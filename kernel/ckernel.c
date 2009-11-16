#include "os.h"
#include "kheap.h"
#include "paging.h"
#include "task.h"
#include "initrd.h"
#include "syscall.h"
#include "shared_pages.h"
#include "pci.h"
#include "cmos.h"
#include "flpydsk.h"
#include "list.h"
//#include "fat12.h" //TEST

// RAM Detection by Second Stage Bootloader
#define ADDR_MEM_ENTRIES 0x0FFE
#define ADDR_MEM_INFO    0x1000

// Buffer for User-Space Program
#define FILEBUFFERSIZE   0x2000


// RAM disk and user program
extern uint32_t file_data_start;
extern uint32_t file_data_end;
uint32_t address_user;
uint8_t address_TEST[FILEBUFFERSIZE];
uint8_t buf[FILEBUFFERSIZE];
uint8_t flag1 = 0; // status of user-space-program


static void init()
{
    k_clear_screen(); settextcolor(14,0);
    printformat("PrettyOS [Version 0.0.0.20]\n");
    gdt_install();
    idt_install();
    isrs_install();
    irq_install();
    initODA();
    timer_install();
    keyboard_install();
}

int main()
{
    init();

    settextcolor(15,0);

    pciScan(); // scan of pci bus; results go to: pciDev_t pciDev_Array[50]; (cf. pci.h)
               // TODO: we need calculation of virtual address from physical address
               //       that we can carry out this routine after paging_install()

    pODA->Memory_Size = paging_install();
    printformat( "Memory size is %X\n", pODA->Memory_Size );
    heap_install();
    tasking_install();
    sti();

    /// TEST list BEGIN
    // link valid devices from pciDev_t pciDev_Array[50] to a dynamic list
    listHead_t* pciDevList = listCreate();
    for(int i=0;i<PCIARRAYSIZE;++i)
    {
        if( pciDev_Array[i].vendorID && (pciDev_Array[i].vendorID != 0xFFFF) )
        {
            listAppend(pciDevList, (void*)(pciDev_Array+i));
            // printformat("%X\t",pciDev_Array+i);
        }
    }
    //printformat("\n");
    // listShow(pciDevList); // shows addresses of list elements (not data)

    printformat("\n");
    for(int i=0;i<PCIARRAYSIZE;++i)
    {
        void* element = listShowElement(pciDevList,i);
        if(element)
        {
            printformat("%X dev: %x vend: %x\t",(pciDev_t*)element,((pciDev_t*)element)->deviceID,((pciDev_t*)element)->vendorID);
        }
    }
    printformat("\n");
    /// TEST list END

    /// direct 1st floppy disk
    if( (cmos_read(0x10)>>4) == 4 ) // 1st floppy 1,44 MB: 0100....b
    {
        printformat("1.44 MB 1st floppy is installed\n\n");

        flpydsk_set_working_drive(0); // set drive 0 as current drive
	    flpydsk_install(6);           // floppy disk uses IRQ 6
	    k_memset((void*)DMA_BUFFER, 0x0, 0x2400);
    }
    else
    {
        printformat("1.44 MB 1st floppy not shown by CMOS\n\n");
    }
    /// direct 1st floppy disk

    // RAM Disk
    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("rd_start: ");
    settextcolor(15,0);
    #endif
    ///
    uint32_t ramdisk_start = (uint32_t)k_malloc(0x200000, PAGESIZE);
    settextcolor(15,0);

    // test with data and program from data.asm
    k_memcpy((void*)ramdisk_start, &file_data_start, (uint32_t)&file_data_end - (uint32_t)&file_data_start);
    fs_root = install_initrd(ramdisk_start);

    // search the content of files <- data from outside "loaded" via incbin ...
    settextcolor(15,0);
    struct dirent* node = 0;
    for ( int i=0; (node = readdir_fs(fs_root, i))!=0; ++i )
    {
        fs_node_t* fsnode = finddir_fs(fs_root, node->name);

        if((fsnode->flags & 0x7) == FS_DIRECTORY)
        {
            printformat("<RAM Disk at %X DIR> %s\n", ramdisk_start, node->name);
        }
        else
        {
            uint32_t sz = read_fs(fsnode, 0, fsnode->length, buf);

            char name[40];
            k_memset(name, 0, 40);
            k_memcpy(name, node->name, 35); // protection against wrong / too long filename
            printformat("%d \t%s\n",sz,name);

            uint32_t j;
            if( k_strcmp((const char*)node->name,((const char*)"shell")==0 ))
            {
                flag1=1;

                if(sz>=FILEBUFFERSIZE)
                {
                    sz = 0;
                    settextcolor(4,0);
                    printformat("Buffer Overflow prohibited!\n");
                    settextcolor(15,0);
                }

                for(j=0; j<sz; ++j)
                    address_TEST[j] = buf[j];
            }
        }
    }
    printformat("\n\n");

    /// shell in elf-executable-format provided by data.asm
    uint32_t elf_vaddr     = *( (uint32_t*)( address_TEST + 0x3C ) );
    uint32_t elf_offset    = *( (uint32_t*)( address_TEST + 0x38 ) );
    uint32_t elf_filesize  = *( (uint32_t*)( address_TEST + 0x44 ) );

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(5,0);
    printformat("virtual address: %X offset: %x size: %x", elf_vaddr, elf_offset, elf_filesize);
    printformat("\n\n");
    settextcolor(15,0);
    #endif
    ///

    if( (elf_vaddr>0x5FF000) || (elf_vaddr<0x400000) || (elf_offset>0x130) || (elf_filesize>0x1000) )
    {
        flag1=2;
    }

    // Test-Programm ==> user space
    if(flag1==1)
    {
        address_user = elf_vaddr;
        k_memcpy((void*)address_user, address_TEST + elf_offset, elf_filesize); // ELF LOAD:
                                                                                // Offset VADDR FileSize
    }

    pODA->ts_flag = 1; // enable task_switching

    if(flag1==1)
    {
      create_task ((void*)getAddressUserProgram(),3); // program in user space (ring 3) takes over
    }
    else
    {
        if(flag1==0)
        {
            settextcolor(4,0);
            printformat("Program not found.\n");
            settextcolor(15,0);
        }
        else
        {
            settextcolor(4,0);
            printformat("Program corrupt.\n");
            settextcolor(15,0);
        }
    }

    while(true){/*  */}
    return 0;
}
