#include "os.h"
#include "kheap.h"
#include "paging.h"
#include "task.h"
#include "initrd.h"
#include "syscall.h"
#include "pci.h"
#include "cmos.h"
#include "flpydsk.h"
#include "list.h"
//#include "fat12.h" //TEST

// RAM Detection by Second Stage Bootloader
#define ADDR_MEM_INFO    0x1000

// Buffer for User-Space Program
#define FILEBUFFERSIZE   0x2000


// RAM disk and user program
extern uint32_t file_data_start;
extern uint32_t file_data_end;


static void init()
{
    k_clear_screen(); settextcolor(14,0);
    printformat("PrettyOS [Version 0.0.0.27]   ");
    cmos_time(); printformat("\n\n");
    gdt_install();
    idt_install();
    timer_install();
    keyboard_install();
    syscall_install();
}


int main()
{
    init();

    settextcolor(15,0);

    pciScan(); // scan of pci bus; results go to: pciDev_t pciDev_Array[50]; (cf. pci.h)
               // TODO: we need calculation of virtual address from physical address
               //       that we can carry out this routine after paging_install()

    pODA->Memory_Size = paging_install();
    printformat( "\nMemory size: %d KB\n", pODA->Memory_Size/1024 );
    heap_install();
    tasking_install();
    sti();

    /// direct 1st floppy disk
    if( (cmos_read(0x10)>>4) == 4 ) // 1st floppy 1,44 MB: 0100....b
    {
        printformat("\n1.44 MB floppy disk is installed as floppy device 0\n\n");

        flpydsk_set_working_drive(0); // set drive 0 as current drive
	    flpydsk_install(32+6);        // floppy disk uses IRQ 6 // 32+6
	    k_memset((void*)DMA_BUFFER, 0x0, 0x2400);
    }
    else
    {
        printformat("\n1.44 MB 1st floppy not shown by CMOS\n\n");
    }
    /// direct 1st floppy disk

    /// TEST list BEGIN
    // link valid devices from pciDev_t pciDev_Array[50] to a dynamic list
    listHead_t* pciDevList = listCreate();
    for(int i=0;i<PCIARRAYSIZE;++i)
    {
        if( pciDev_Array[i].vendorID && (pciDev_Array[i].vendorID != 0xFFFF) && (pciDev_Array[i].vendorID != 0xEE00) )   // there is no vendor EE00h
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
            printformat("%X dev: %x vend: %x\t",
                       ( pciDev_t*)element,
                       ((pciDev_t*)element)->deviceID,
                       ((pciDev_t*)element)->vendorID);
        }
    }
    printformat("\n\n");
    /// TEST list END

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
    bool shell_found = false;
    uint8_t* buf = k_malloc( FILEBUFFERSIZE, 0 );
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

            if ( k_strcmp( (const char*)node->name, "shell" ) == 0 )
            {
                shell_found = true;

                if ( ! elf_exec( buf, sz ) )
                    printformat( "Cannot start shell!\n" );
            }
        }
    }
    k_free( buf );
    printformat("\n\n");

    if ( ! shell_found )
    {
        settextcolor(4,0);
        printformat("Program not found.\n");
        settextcolor(15,0);
    }

    pODA->ts_flag = 1;

    const char* progress = "|/-\\";
    const char* p = progress;
    while ( true )
    {
        *((uint16_t*)(0xB8000+158)) = 0x0F00 | *p;
        if ( ! *++p )
            p = progress;
    }
    return 0;
}
