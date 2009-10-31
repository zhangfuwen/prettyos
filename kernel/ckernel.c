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
//#include "fat12.h" //TEST

// RAM Detection by Second Stage Bootloader
#define ADDR_MEM_ENTRIES 0x0FFE
#define ADDR_MEM_INFO    0x1000

// Buffer for User-Space Program
#define FILEBUFFERSIZE   0x2000

extern page_directory_t* kernel_directory;
extern page_directory_t* current_directory;
extern uint32_t file_data_start;
extern uint32_t file_data_end;
uint32_t address_user;
uint8_t address_TEST[FILEBUFFERSIZE];
uint8_t buf[FILEBUFFERSIZE];
uint8_t flag1 = 0; // status of user-space-program
Mem_Chunk_t Mem_Chunk[10]; // contiguous parts of memory detected by int 15h eax = 820h

extern uint32_t placement_address;
heap_t* kheap = 0;

static void init()
{
    kheap = 0;
    kernel_directory  = 0;
    current_directory = 0;
    placement_address = 0x200000;

    k_clear_screen(); settextcolor(14,0);
    printformat("PrettyOS [Version 0.1.0111]  ");
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

    // get physical memory which is usable RAM
    uint16_t num_of_entries = *( (uint16_t*)(ADDR_MEM_ENTRIES) );

    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("\nNUM of RAM-Entries: %d", num_of_entries);
    settextcolor(15,0);
    #endif

    pODA->Memory_Size = 0;
    uint32_t i,j;
    for(i=0; i<num_of_entries; ++i)
    {
        for(j=0; j<24; j+=4)
        {
            if(j== 0) Mem_Chunk[i].base_lo   = *( (uint32_t*)(ADDR_MEM_INFO+i*24+j) );
            if(j== 4) Mem_Chunk[i].base_hi   = *( (uint32_t*)(ADDR_MEM_INFO+i*24+j) );
            if(j== 8) Mem_Chunk[i].length_lo = *( (uint32_t*)(ADDR_MEM_INFO+i*24+j) );
            if(j==12) Mem_Chunk[i].length_hi = *( (uint32_t*)(ADDR_MEM_INFO+i*24+j) );
            if(j==16) Mem_Chunk[i].type      = *( (uint32_t*)(ADDR_MEM_INFO+i*24+j) );
            if(j==20) Mem_Chunk[i].extended  = *( (uint32_t*)(ADDR_MEM_INFO+i*24+j) );
        }
        if((Mem_Chunk[i].type)==1) pODA->Memory_Size += Mem_Chunk[i].length_lo;
    }
    if( (pODA->Memory_Size>0)&&(pODA->Memory_Size<=0xFFFFFFFF) )
    {
        printformat("Usable RAM: %d KB", (pODA->Memory_Size)/1024);
    }
    else
    {
        if(pODA->Memory_Size==0)
           pODA->Memory_Size = 0x2000000; // 32 MB

        printformat("\nNo memory detection. Estimated usable RAM: %d KB", (pODA->Memory_Size)/1024);
    }
    printformat("\n\n");


    /// TEST PCI-SCAN BEGIN
    settextcolor(15,0);
    uint16_t pciDevice          = 0; // device
    uint16_t pciVendor          = 0; // vendor
    uint8_t  pciClass           = 0; // device class
    uint8_t  pciSubclass        = 0; // device subclass
    uint8_t  pciProgrInterface  = 0; // program interface
    uint8_t  pciIRQLine         = 0; // IRQ line (interrupt line)
    uint8_t  bus                = 0; // max. 256
    uint8_t  device             = 0; // max.  32
    uint8_t  func               = 0; // max.   8

    struct pciBasicAddressRegister pciBAR[6]; // base address, size, type of BAR0 - BAR5
    uint32_t pciBar             = 0;
    uint32_t EHCI_data          = 0;

    for(bus=0;bus<8;++bus)
    {
        for(device=0;device<32;++device)
        {
            for(func=0;func<8;++func)
            {
                pciVendor                 = pci_config_read( bus, device, func, PCI_VENDOR_ID );
                pciDevice                 = pci_config_read( bus, device, func, PCI_DEVICE_ID );
                pciClass                  = pci_config_read( bus, device, func, PCI_CLASS     );
                pciSubclass               = pci_config_read( bus, device, func, PCI_SUBCLASS  );
                pciProgrInterface         = pci_config_read( bus, device, func, PCI_INTERFACE );
                pciIRQLine                = pci_config_read( bus, device, func, PCI_IRQLINE   );
                pciBAR[0].baseAddress     = pci_config_read( bus, device, func, PCI_BAR0      );
                pciBAR[1].baseAddress     = pci_config_read( bus, device, func, PCI_BAR1      );
                pciBAR[2].baseAddress     = pci_config_read( bus, device, func, PCI_BAR2      );
                pciBAR[3].baseAddress     = pci_config_read( bus, device, func, PCI_BAR3      );
                pciBAR[4].baseAddress     = pci_config_read( bus, device, func, PCI_BAR4      );
                pciBAR[5].baseAddress     = pci_config_read( bus, device, func, PCI_BAR5      );

                if( pciVendor != 0xFFFF )
                {
                    if(pciIRQLine!=255)
                    {
                        printformat("%d:%d.%d\t dev:%x vend:%x IRQ:%d ",
                                     bus, device, func, pciDevice, pciVendor, pciIRQLine);
                    }
                    else // "255 is defined as meaning "unknown" or "no connection" to the interrupt controller"
                    {
                        printformat("%d:%d.%d\t dev:%x vend:%x IRQ:-- ",
                                     bus, device, func, pciDevice, pciVendor);

                    }

                    if( (pciClass==0x0C) && (pciSubclass==0x03) ) // USB
                    {
                        printformat(" USB ");
                        if( pciProgrInterface==0x00 ) { printformat("UHCI ");   }
                        if( pciProgrInterface==0x10 ) { printformat("OHCI ");   }
                        if( pciProgrInterface==0x20 ) { printformat("EHCI ");   }
                        if( pciProgrInterface==0x80 ) { printformat("no HCI "); }
                        if( pciProgrInterface==0xFE ) { printformat("any ");    }

                        for(i=0;i<6;++i) // check USB BARs
                        {
                            pciBAR[i].memoryType = pciBAR[i].baseAddress & 0x01;

                            if(pciBAR[i].baseAddress)
                            {
                                if(pciBAR[i].memoryType == 0)
                                {
                                    printformat("%d:%X MEM ", i, pciBAR[i].baseAddress & 0xFFFFFFF0 );
                                }
                                if(pciBAR[i].memoryType == 1)
                                {
                                    printformat("%d:%X I/O ", i, pciBAR[i].baseAddress & 0xFFFFFFFC );
                                }

                                /// TEST Memory Size Begin
                                cli();
                                pci_config_write_dword  ( bus, device, func, PCI_BAR0 + 4*i, 0xFFFFFFFF );
                                pciBar = pci_config_read( bus, device, func, PCI_BAR0 + 4*i             );
                                pci_config_write_dword  ( bus, device, func, PCI_BAR0 + 4*i, pciBAR[i].baseAddress  );
                                sti();
                                pciBAR[i].memorySize = (~pciBar | 0x0F) + 1;
                                printformat("sz:%d ", pciBAR[i].memorySize );

                                    /// TEST EHCI Data Begin
                                    if( (pciProgrInterface==0x20) && pciBAR[i].baseAddress )
                                    {
                                        for(j=0;j<24;++j)
                                        {
                                             EHCI_data = *((volatile uint8_t*)( (pciBAR[i].baseAddress & 0xFFFFFFF0) + j ));
                                             if(!j)
                                             {
                                                 printformat("\n");
                                             }
                                             else
                                             {
                                                 printformat("\t");
                                             }
                                             printformat("BAR%d+%d: %x ",i, j, EHCI_data);
                                        }
                                    }
                                    /// TEST EHCI Data Begin

                                /// TEST Memory Size End
                            }
                        }
                    }
                    printformat("\n");
                }

                // Bit 7 in header type (Bit 23-16) --> multifunctional
                if( !(pci_config_read(bus, device, 0, PCI_HEADERTYPE) & 0x80) )
                {
                    break; // --> not multifunctional
                }
            }
        }
    }
    printformat("\n");
    /// TEST PCI-SCAN END

    // paging, kernel heap
    paging_install();
    kheap = create_heap(KHEAP_START, KHEAP_START+KHEAP_INITIAL_SIZE, KHEAP_MAX, 1, 0); // SV and RW

    // RAM Disk
    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("rd_start: ");
    settextcolor(15,0);
    #endif
    ///
    uint32_t ramdisk_start = k_malloc(0x200000, 0, 0);
    settextcolor(15,0);

    // multitasking
    tasking_install(); // ends with sti()

    // test with data and program from data.asm
    k_memcpy((void*)ramdisk_start, &file_data_start, (uint32_t)&file_data_end - (uint32_t)&file_data_start);
    fs_root = install_initrd(ramdisk_start);

    // search the content of files <- data from outside "loaded" via incbin ...
    settextcolor(15,0);
    i=0;
    struct dirent* node = 0;
    while( (node = readdir_fs(fs_root, i)) != 0)
    {
        fs_node_t* fsnode = finddir_fs(fs_root, node->name);

        if((fsnode->flags & 0x7) == FS_DIRECTORY)
        {
            printformat("<RAM Disk at %X DIR> %s\n", ramdisk_start, node->name);
        }
        else
        {
            uint32_t sz = read_fs(fsnode, 0, fsnode->length, buf);


            int8_t name[40];
            k_memset((void*)name, 0, 40);
            k_memcpy((void*)name, node->name, 35); // protection against wrong / too long filename
            printformat("%d \t%s\n",sz,name);

            uint32_t j;
            if( k_strcmp((const int8_t*)node->name,((const int8_t*)"shell")==0 ))
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
        ++i;
    }
    printformat("\n\n");

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
