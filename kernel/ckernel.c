/*
 *  license and disclaimer for the use of this source code as per statement below
 *  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
 */

// Utilities
#include "util.h"               // sti, memset, strcmp, strlen, rdtsc, ...
#include "todo_list.h"          // todoList_execute

// Internal devices
#include "cpu.h"                // cpu_analyze
#include "cmos.h"               // cmos_read
#include "timer.h"              // timer_install, timer_getSeconds, sleepMilliSeconds
#include "time.h"               // getCurrentDateAndTime
#include "descriptor_tables.h"  // idt_install, gdt_install
#include "irq.h"                // isr_install
#include "power_management.h"   // pm_install, pm_log

// Base system
#include "kheap.h"              // heap_install, malloc, free, logHeapRegions
#include "task.h"               // tasking_install & others
#include "elf.h"                // elf_prepare
#include "syscall.h"            // syscall_install

// External devices
#include "cdi.h"                // cdi_init
#include "keyboard.h"           // keyboard_install, KEY_...
#include "mouse.h"              // mouse_install
#include "serial.h"             // serial_init
#include "video/videomanager.h" // video_install, video_test
#include "filesystem/initrd.h"  // initrd_install, ramdisk_install, readdir_fs, read_fs, finddir_fs
#include "storage/flpydsk.h"    // flpydsk_install

// Network. Only for temporary tests. TODO: implement as user applications
#include "netprotocol/icmp.h"   // send echo request (PING)
#include "netprotocol/udp.h"    // udp_send (TEST)
#include "netprotocol/tcp.h"    // passive opened connection (LISTEN)


const char* const version = "0.0.2.272 - Rev: 1122";

// .bss
extern uintptr_t _bss_start; // linker script
extern uintptr_t _bss_end;   // linker script

// Information about the system
system_t system;

bool fpu_install(); // fpu.c
void fpu_test();    // fpu.c

extern diskType_t* ScreenDest; // HACK for screenshots

// APIC
bool apic_install()
{
    // TODO: implement APIC functionality
    return(false);
}

todoList_t* kernel_idleTasks;

typedef struct // http://www.lowlevel.eu/wiki/Multiboot
{
    uint32_t flags;
    uint32_t memLower;
    uint32_t memUpper;
    uint32_t bootdevice;
    uint32_t cmdline;
    uint32_t modsCount;
    void*    mods;
    uint32_t syms[4];
    uint32_t mmapLength;
    void*    mmap;
    uint32_t drivesLength;
    void*    drives;
    uint32_t configTable;
    char*    bootloaderName;
    uint32_t apmTable;
    uint32_t vbe_controlInfo;
    uint32_t vbe_modeInfo;
    uint16_t vbe_mode;
    uint16_t vbe_interfaceSeg;
    uint16_t vbe_interfaceOff;
    uint16_t vbe_interfaceLen;
} __attribute__((packed)) multiboot_t;

static void useMultibootInformation(multiboot_t* mb_struct)
{
    memoryMapAdress = mb_struct->mmap;
    memoryMapEnd = (void*)((uintptr_t)mb_struct->mmap + mb_struct->mmapLength);
}

static void log(const char* str)
{
    textColor(SUCCESS);
    printf("[DONE]");
    textColor(LIGHT_GRAY);
    printf("\t%s\n", str);
    textColor(TEXT);
}

static void init(multiboot_t* mb_struct)
{
    // set .bss to zero
    memset(&_bss_start, 0x0, (uintptr_t)&_bss_end - (uintptr_t)&_bss_start);

    useMultibootInformation(mb_struct);

    // video
    kernel_console_init();
    clear_screen();

    // GDT
    gdt_install();

    // Interrupts
    isr_install();
    if(apic_install()) log("APIC");
    else // PIC as fallback
    {
        idt_install(); // cf. interrupts.asm
        log("PIC");
    }

    // internal devices
    timer_install(SYSTEMFREQUENCY); log("Timer"); // Sets system frequency to ... Hz
    if(fpu_install()) log("FPU");

    // memory
    system.Memory_Size = paging_install(); log("Paging");
    heap_install(); log("Heap");

    vga_install(); log("Video");

    tasking_install(); log("Multitasking");

    // external devices
    keyboard_install(); log("Keyboard");
    mouse_install(); log("Mouse");

    pm_install(); log("Power Management");

    // system calls
    syscall_install(); log("Syscalls");

    cdi_init(); log("CDI");

    deviceManager_install(); log("Devicemanager"); // device management for mass storage devices

    kernel_idleTasks = todolist_create();

    sti();
}

static void showMemorySize()
{
    textColor(HEADLINE);
    printf("\nMemory: ");
    textColor(TEXT);
    if (system.Memory_Size >= 0x40000000) // More than 1 GiB
    {
        printf("%u GiB  (%u MiB, %u Bytes)\n", system.Memory_Size>>30, system.Memory_Size>>20, system.Memory_Size);
    }
    else
    {
        printf("%u MiB  (%u Bytes)\n", system.Memory_Size>>20, system.Memory_Size);
    }
}

void main(multiboot_t* mb_struct)
{
    init(mb_struct);
    srand(cmos_read(0x00));

    scheduler_insertTask(create_cthread(&bootscreen, "Booting ..."));

    showMemorySize();
    cpu_analyze();

    fpu_test();
    pm_log();

    serial_init();

    pci_scan(); // Scan of PCI bus to detect PCI devices. (cf. pci.h)

    flpydsk_install(); // detect FDDs
    #ifdef _RAMDISK_DIAGNOSIS_
    void* ramdisk_start = initrd_install(ramdisk_install(), 0, 0x200000);
    #else
    initrd_install(ramdisk_install(), 0, 0x200000);
    #endif

    #ifdef _DEVMGR_DIAGNOSIS_
    showPortList();
    showDiskList();
    #endif

    // search and load shell
    bool shell_found = false;
    struct dirent* node = 0;
    for (size_t i = 0; (node = readdir_fs(fs_root, i)) != 0; ++i)
    {
        fs_node_t* fsnode = finddir_fs(fs_root, node->name);

        if ((fsnode->flags & 0x7) == FS_DIRECTORY)
        {
            #ifdef _RAMDISK_DIAGNOSIS_
            printf("\n<RAMdisk (%Xh) - Root Directory>\n", ramdisk_start);
            #endif
        }
        else
        {
            #ifdef _RAMDISK_DIAGNOSIS_
            printf("%u \t%s\n", fsnode->length, node->name);
            #endif

            if (strcmp(node->name, "shell") == 0)
            {
                shell_found = true;

                uint8_t* buf = malloc(fsnode->length, 0, "shell buffer");
                uint32_t sz = read_fs(fsnode, 0, fsnode->length, buf);

                pageDirectory_t* pd = paging_createUserPageDirectory();
                void* entry = elf_prepare(buf, sz, pd);
                if(entry == 0)
                {
                    textColor(ERROR);
                    printf("Cannot start Shell.\n");
                    paging_destroyUserPageDirectory(pd);
                }
                else
                    scheduler_insertTask(create_process(pd, entry, 3, 0, 0));
                free(buf);
            }
        }
    }
    if (!shell_found)
    {
        textColor(ERROR);
        printf("\nProgram not found.\n");
        textColor(TEXT);
    }

    scheduler_insertTask(create_cthread(&video_test, "VBE"));

    textColor(SUCCESS);
    printf("\n\n--------------------------------------------------------------------------------");
    printf("                                PrettyOS Booted\n");
    printf("--------------------------------------------------------------------------------");
    textColor(TEXT);

    const char* progress    = "|/-\\";    // rotating asterisk
    uint64_t LastRdtscValue = 0;          // rdtsc: read time-stamp counter
    uint32_t CurrentSeconds = 0xFFFFFFFF; // Set on a high value to force a refresh of the statusbar at the beginning.
    char     DateAndTime[50];             // String for Date&Time

    bool ESC   = false;
    bool CTRL  = false;
    bool PRINT = false;

    tcpConnection_t* connection = 0;

    while (true) // start of kernel idle loop
    {
        // show rotating asterisk
        vga_setPixel(79, 49, (FOOTNOTE<<8) | *progress); // Write the character on the screen. (color|character)
        if (! *++progress) { progress = "|/-\\"; }

        // Handle events. TODO: Many of the shortcuts can be moved to the shell later.
        char buffer[4];
        EVENT_t ev = event_poll(buffer, 4, EVENT_NONE); // Take one event from the event queue
        while(ev != EVENT_NONE)
        {
            switch(ev)
            {
                case EVENT_KEY_DOWN:
                    // Detect CTRL and ESC
                    switch(*(KEY_t*)buffer)
                    {
                        case KEY_ESC:
                            ESC = true;
                            break;
                        case KEY_LCTRL: case KEY_RCTRL:
                            CTRL = true;
                            break;
                        case KEY_PRINT: case KEY_F12: // Because of special behaviour of the PRINT key in emulators, we handle it different: After PRINT was pressed, next text entered event is taken as argument to PRINT. F12 is alias for PrintScreen due to problems in some emulators
                            PRINT = true;
                            break;
                        default:
                            break;
                    }
                    break;
                case EVENT_KEY_UP:
                    // Detect CTRL and ESC
                    switch(*(KEY_t*)buffer)
                    {
                        case KEY_ESC:
                            ESC = false;
                            break;
                        case KEY_LCTRL: case KEY_RCTRL:
                            CTRL = false;
                            break;
                        default:
                            break;
                    }
                    break;
                case EVENT_TEXT_ENTERED:
                    if(PRINT)
                    {
                        PRINT = false;
                        switch(*(char*)buffer)
                        {
                            case 'f': // Taking a screenshot (Floppy)
                                ScreenDest = &FLOPPYDISK; // HACK
                                printf("Save screenshot to Floppy.");
                                saveScreenshot();
                                break;
                            case 'u': // Taking a screenshot (USB)
                                ScreenDest = &USB_MSD; // HACK
                                printf("Save screenshot to USB.");
                                saveScreenshot();
                                break;
                        }
                    }
                    if(ESC)
                    {
                        switch(*(char*)buffer)
                        {
                            case 'h':
                                heap_logRegions();
                                break;
                            case 'p':
                                paging_analyzeBitTable();
                                break;
                        }
                    }
                    else if(CTRL)
                    {
                        switch(*(char*)buffer)
                        {
                            case 'd':
                                showPortList();
                                showDiskList();
                                break;
                            case 'a':
                                network_displayArpTables();
                                break;
                            case 't':
                                scheduler_log();
                                break;
                            case 'n':
                            {
                                network_adapter_t* adapter = network_getFirstAdapter();

                                if (adapter)
                                {
                                    // parameters for UDPSend(...)
                                    uint16_t srcPort  = 40; // unassigend
                                    uint16_t destPort = 40;
                                    IP_t IP = {.iIP = 0xFFFFFFFF};
                                    udp_send(adapter, "PrettyOS says hello", strlen("PrettyOS says hello"), srcPort, adapter->IP, destPort, IP);
                                }
                                break;
                            }
                            case 'f':
                            {
                                network_adapter_t* adapter = network_getFirstAdapter();

                                if (adapter)
                                {
                                    DHCP_Release(adapter);
                                }
                                break;
                            }
                            case 'b':
                            {
                                connection = tcp_createConnection();

                                network_adapter_t* adapter = network_getFirstAdapter();

                                if(adapter)
                                    tcp_bind(connection, adapter);
                                break;
                            }
                            case 'c': // show tcp connections
                            {
                                textColor(HEADLINE);
                                printf("\ntcp connections:");
                                textColor(TEXT);
                                tcp_showConnections();
                                break;
                            }
                            case 'p':
                            {
                                network_adapter_t* adapter = network_getFirstAdapter();
                                IP_t destIP; // qemu subnet

                                for (uint8_t i=0; i<255; i++)
                                {
                                    destIP.IP[0] =      10;
                                    destIP.IP[1] =       0;
                                    destIP.IP[2] =       2;
                                    destIP.IP[3] =       i;
                                    if (adapter)
                                        icmp_sendEchoRequest(adapter, destIP);
                                }
                                break;
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
            ev = event_poll(buffer, 4, EVENT_NONE);
        }

        if (timer_getSeconds() != CurrentSeconds)
        {
            CurrentSeconds = timer_getSeconds();

            // all values 64 bit
            uint64_t Rdtsc = rdtsc();
            uint64_t RdtscKCounts   = (Rdtsc - LastRdtscValue);  // Build difference
            uint32_t RdtscKCountsHi = RdtscKCounts >> 32;        // high dword
            uint32_t RdtscKCountsLo = RdtscKCounts & 0xFFFFFFFF; // low dword
            LastRdtscValue = Rdtsc;

            if (RdtscKCountsHi == 0)
                system.CPU_Frequency_kHz = RdtscKCountsLo/1000;

            // draw status bar with date, time and frequency
            getCurrentDateAndTime(DateAndTime);
            kprintf("%s   %u s runtime. CPU: %u MHz    ", 49, FOOTNOTE, DateAndTime, CurrentSeconds, system.CPU_Frequency_kHz/1000); // output in status bar

            deviceManager_checkDrives(); // switch off motors if they are not neccessary
        }


        if (serial_received(1) != 0)
        {
            textColor(HEADLINE);
            printf("\nSerial message:\n");
            textColor(TEXT);
            do
            {
                uint8_t sbyt=serial_read(1);
                printf("%y ",sbyt);
                sleepMilliSeconds(8);
            } while (serial_received(1) != 0);
            serial_write(1,'P');
            serial_write(1,'r');
            serial_write(1,'e');
            serial_write(1,'t');
            serial_write(1,'t');
            serial_write(1,'y');
            printf("\nAnswered with 'Pretty'!\n");
        }

        todoList_execute(kernel_idleTasks);

        switch_context(); // Switch to another task
    } // end of kernel idle loop
}

/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
