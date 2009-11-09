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

// TODO: use a dynamic list; TODO: create structs/functions list_t
pciDev_t pciDev_Array[50];

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
    uint8_t  bus                = 0; // max. 256
    uint8_t  device             = 0; // max.  32
    uint8_t  func               = 0; // max.   8

    uint32_t pciBar             = 0; // helper variable for memory size
    uint32_t EHCI_data          = 0; // helper variable for EHCI_data


    // list of devices, 50 for first tests
    for(i=0;i<50;++i)
    {
        pciDev_Array[i].number = i;
    }

    int number=0;
    for(bus=0;bus<8;++bus)
    {
        for(device=0;device<32;++device)
        {
            for(func=0;func<8;++func)
            {
                pciDev_Array[number].vendorID     = pci_config_read( bus, device, func, PCI_VENDOR_ID  );
                pciDev_Array[number].deviceID     = pci_config_read( bus, device, func, PCI_DEVICE_ID  );
                pciDev_Array[number].classID      = pci_config_read( bus, device, func, PCI_CLASS      );
                pciDev_Array[number].subclassID   = pci_config_read( bus, device, func, PCI_SUBCLASS   );
                pciDev_Array[number].interfaceID  = pci_config_read( bus, device, func, PCI_INTERFACE  );
                pciDev_Array[number].revID        = pci_config_read( bus, device, func, PCI_REVISION   );
                pciDev_Array[number].irq          = pci_config_read( bus, device, func, PCI_IRQLINE    );
                pciDev_Array[number].bar[0].baseAddress = pci_config_read( bus, device, func, PCI_BAR0 );
                pciDev_Array[number].bar[1].baseAddress = pci_config_read( bus, device, func, PCI_BAR1 );
                pciDev_Array[number].bar[2].baseAddress = pci_config_read( bus, device, func, PCI_BAR2 );
                pciDev_Array[number].bar[3].baseAddress = pci_config_read( bus, device, func, PCI_BAR3 );
                pciDev_Array[number].bar[4].baseAddress = pci_config_read( bus, device, func, PCI_BAR4 );
                pciDev_Array[number].bar[5].baseAddress = pci_config_read( bus, device, func, PCI_BAR5 );

                if( pciDev_Array[number].vendorID != 0xFFFF )
                {
                    // Valid Device
                    pciDev_Array[number].bus    = bus;
                    pciDev_Array[number].device = device;
                    pciDev_Array[number].func   = func;

                    // output to screen
                    printformat("%d:%d.%d\t dev:%x vend:%x",
                         pciDev_Array[number].bus, pciDev_Array[number].device, pciDev_Array[number].func,
                         pciDev_Array[number].deviceID, pciDev_Array[number].vendorID );

                    if(pciDev_Array[number].irq!=255)
                    {
                        printformat(" IRQ:%d ", pciDev_Array[number].irq );
                    }
                    else // "255 means "unknown" or "no connection" to the interrupt controller"
                    {
                        printformat(" IRQ:-- ");
                    }

                    // test on USB
                    if( (pciDev_Array[number].classID==0x0C) && (pciDev_Array[number].subclassID==0x03) )
                    {
                        printformat(" USB ");
                        if( pciDev_Array[number].interfaceID==0x00 ) { printformat("UHCI ");   }
                        if( pciDev_Array[number].interfaceID==0x10 ) { printformat("OHCI ");   }
                        if( pciDev_Array[number].interfaceID==0x20 ) { printformat("EHCI ");   }
                        if( pciDev_Array[number].interfaceID==0x80 ) { printformat("no HCI "); }
                        if( pciDev_Array[number].interfaceID==0xFE ) { printformat("any ");    }

                        for(i=0;i<6;++i) // check USB BARs
                        {
                            pciDev_Array[number].bar[i].memoryType = pciDev_Array[number].bar[i].baseAddress & 0x01;

                            if(pciDev_Array[number].bar[i].baseAddress) // check valid BAR
                            {
                                if(pciDev_Array[number].bar[i].memoryType == 0)
                                {
                                    printformat("%d:%X MEM ", i, pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFF0 );
                                }
                                if(pciDev_Array[number].bar[i].memoryType == 1)
                                {
                                    printformat("%d:%X I/O ", i, pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFFC );
                                }

                                /// TEST Memory Size Begin
                                cli();
                                pci_config_write_dword  ( bus, device, func, PCI_BAR0 + 4*i, 0xFFFFFFFF );
                                pciBar = pci_config_read( bus, device, func, PCI_BAR0 + 4*i             );
                                pci_config_write_dword  ( bus, device, func, PCI_BAR0 + 4*i,
                                                          pciDev_Array[number].bar[i].baseAddress       );
                                sti();
                                pciDev_Array[number].bar[i].memorySize = (~pciBar | 0x0F) + 1;
                                printformat("sz:%d ", pciDev_Array[number].bar[i].memorySize );
                                /// TEST Memory Size End

                                /// TEST EHCI Data Begin
                                if(  (pciDev_Array[number].interfaceID==0x20)   // EHCI
                                   && pciDev_Array[number].bar[i].baseAddress ) // valid BAR
                                {
                                    /*
                                    Offset Size Mnemonic    Power Well   Register Name
                                    00h     1   CAPLENGTH      Core      Capability Register Length
                                    01h     1   Reserved       Core      N/A
                                    02h     2   HCIVERSION     Core      Interface Version Number
                                    04h     4   HCSPARAMS      Core      Structural Parameters
                                    08h     4   HCCPARAMS      Core      Capability Parameters
                                    0Ch     8   HCSP-PORTROUTE Core      Companion Port Route Description
                                    */

                                    EHCI_data = *((volatile uint8_t* )((pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFF0) + 0x00 ));
                                    printformat("\nBAR%d CAPLENGTH:  %x \t\t",i, EHCI_data);

                                    EHCI_data = *((volatile uint16_t*)((pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFF0) + 0x02 ));
                                    printformat(  "BAR%d HCIVERSION: %x \n",i, EHCI_data);

                                    EHCI_data = *((volatile uint32_t*)((pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFF0) + 0x04 ));
                                    printformat(  "BAR%d HCSPARAMS:  %X \t",i, EHCI_data);

                                    EHCI_data = *((volatile uint32_t*)((pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFF0) + 0x08 ));
                                    printformat(  "BAR%d HCCPARAMS:  %X \n",i, EHCI_data);
                                }
                                /// TEST EHCI Data End
                            } // if
                        } // for
                    } // if
                    printformat("\n");
                    ++number;
                } // if pciVendor

                // Bit 7 in header type (Bit 23-16) --> multifunctional
                if( !(pci_config_read(bus, device, 0, PCI_HEADERTYPE) & 0x80) )
                {
                    break; // --> not multifunctional
                }
            } // for function
        } // for device
    } // for bus
    printformat("\n");
    /// TEST PCI-SCAN END

    // paging, kernel heap
    paging_install();
    kheap = create_heap(KHEAP_START, KHEAP_START+KHEAP_INITIAL_SIZE, KHEAP_MAX, 1, 0); // SV and RW
    tasking_install(); // ends with sti()

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
    uint32_t ramdisk_start = k_malloc(0x200000, 0, 0);
    settextcolor(15,0);

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
