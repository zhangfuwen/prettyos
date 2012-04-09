/*
 *  license and disclaimer for the use of this source code as per statement below
 *  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
 */

// Utilities
#include "util/util.h"          // sti, memset, strcmp, strlen, rdtsc, ...
#include "util/todo_list.h"     // todoList_execute

// Internal devices
#include "cpu.h"                // cpu_analyze
#include "cmos.h"               // cmos_read
#include "timer.h"              // timer_install, timer_getSeconds, sleepMilliSeconds
#include "time.h"               // getCurrentDateAndTime
#include "descriptor_tables.h"  // idt_install, gdt_install
#include "irq.h"                // isr_install
#include "power_management.h"   // powmgmt_install, powmgmt_log
#include "apic.h"               // apic_install, apic_available

// Base system
#include "kheap.h"              // heap_install, malloc, free, logHeapRegions
#include "tasking/task.h"       // tasking_install & others
#include "syscall.h"            // syscall_install
#include "ipc.h"                // ipc_print

// External devices
#include "cdi.h"                // cdi_init
#include "keyboard.h"           // keyboard_install, KEY_...
#include "mouse.h"              // mouse_install
#include "serial.h"             // serial_init
#include "video/videomanager.h" // video_install, video_test
#include "video/textgui.h"      // TextGUI_ShowMSG, TextGUI_AskYN
#include "filesystem/initrd.h"  // initrd_install, ramdisk_install, readdir_fs, read_fs, finddir_fs
#include "storage/flpydsk.h"    // flpydsk_install

// Network
#include "netprotocol/tcp.h"    // tcp_showConnections, network_displayArpTables


const char* const version = "0.0.4.1 - Rev: 1384";

// .bss
extern uintptr_t _bss_start; // Linker script
extern uintptr_t _bss_end;   // Linker script

extern diskType_t* ScreenDest; // HACK for screenshots

todoList_t* kernel_idleTasks; // List of functions that are executed in kernel idle loop


static void logText(const char* str)
{
    textColor(LIGHT_GRAY);
    printf("   => %s: ", str);

    uint16_t len = strlen(str);

    for (uint16_t i = len; i < 20;i++)
    {
        putch(' ');
    }
}

static void logExec(bool b)
{
    putch('[');
    if(b)
    {
        textColor(SUCCESS);
        printf("OK");
    }
    else
    {
        textColor(ERROR);
        printf("ERROR");
    }
    textColor(LIGHT_GRAY);
    printf("]\n");
    textColor(TEXT);
}

#define log(Text, Func) {logText(Text); logExec(Func);} // For functions returning bool. Writes [ERROR] if false is returned. [OK] otherwise.
#define simpleLog(Text, Func) {logText(Text); Func; logExec(true);} // For functions returning void. Writes [OK]


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

static void init(multiboot_t* mb_struct)
{
    // Set .bss to zero
    memset(&_bss_start, 0, (uintptr_t)&_bss_end - (uintptr_t)&_bss_start);

    // Video
    kernel_console_init();
    vga_clearScreen();

    textColor(HEADLINE);
    printf(" => Initializing PrettyOS:\n\n");

    // GDT
    gdt_install();

    // Interrupts
    isr_install();

    cpu_install();

    if (apic_available())
        log("APIC", apic_install())
    //else // PIC as fallback // Use APIC+PIC until our APIC driver is ready to replace PIC
        simpleLog("PIC", idt_install()); // Cf. interrupts.asm

    // Internal devices
    simpleLog("Timer", timer_install(SYSTEMFREQUENCY)); // Sets system frequency to ... Hz
    log("FPU", fpu_install());

    // Memory
    int64_t memsize = paging_install(mb_struct->mmap, (void*)mb_struct->mmap + mb_struct->mmapLength);
    ipc_setInt("PrettyOS/RAM (Bytes)", &memsize);
    log("Paging", memsize != 0);
    simpleLog("Heap", heap_install());

    // Video
    log("Video", vga_install());

    // Tasks
    simpleLog("Multitasking", tasking_install());

  #ifdef _BOOTSCREEN_
    scheduler_insertTask(create_cthread(&bootscreen, "Booting ..."));
  #endif

    // External devices
    simpleLog("Keyboard", keyboard_install());
    simpleLog("Mouse", mouse_install());

    // Power management
    simpleLog("Power Management", powmgmt_install());

    // System calls
    simpleLog("Syscalls", syscall_install());

    // Mass storage devices
    simpleLog("Devicemanager", deviceManager_install(0));

    kernel_idleTasks = todolist_create();

    puts("\n\n");
    sti();
}

static void showMemorySize()
{
    textColor(LIGHT_GRAY);
    printf("   => Memory: ");
    textColor(TEXT);
    int64_t ramsize;
    ipc_getInt("PrettyOS/RAM (Bytes)", &ramsize);

    printf("%Sa (%u Bytes)\n", (size_t)ramsize, (uint32_t)ramsize);
    textColor(LIGHT_GRAY);
}

void main(multiboot_t* mb_struct)
{
    init(mb_struct);
    srand(cmos_read(CMOS_SECOND));

    textColor(HEADLINE);
    printf(" => System analysis:\n");

    showMemorySize();

    cpu_analyze();
    fpu_test();

    powmgmt_log();

    serial_init();

    pci_scan(); // Scan of PCI bus to detect PCI devices. (cf. pci.h)

    cdi_init(); // http://www.lowlevel.eu/wiki/CDI

    flpydsk_install(); // Detect FDDs

    initrd_install(ramdisk_install(), 0);

  #ifdef _DEVMGR_DIAGNOSIS_
    showPortList();
    showDiskList();
  #endif


    /*TextGUI_ShowMSG("W e l c o m e   t o   P r e t t y O S !", "This is an educational OS!");

    uint16_t result = TextGUI_AskYN("Question", "Are you a software developer?",TEXTGUI_YES);

    switch (result)
    {
        case TEXTGUI_ABORTED:
            TextGUI_ShowMSG("Result", "You have cancelled the MessageBox!");
            break;
        case TEXTGUI_YES:
            TextGUI_ShowMSG("Result", "Hello developer!");
            break;
        case TEXTGUI_NO:
            TextGUI_ShowMSG("Result", "Hello user!");
            break;
        default:
            TextGUI_ShowMSG("Result", "This should not happen.");
            break;
    }*/

    initrd_loadShell();

    textColor(SUCCESS);
    printf("\n\n--------------------------------------------------------------------------------");
    printf("                                PrettyOS Booted\n");
    printf("--------------------------------------------------------------------------------");
    textColor(TEXT);


    const char* progress    = "|/-\\";    // Rotating asterisk
    uint32_t CurrentSeconds = 0xFFFFFFFF; // Set on a high value to force a refresh of the statusbar at the beginning.

    bool ESC   = false;
    bool CTRL  = false;
    bool PRINT = false;

    while (true) /// Start of kernel idle loop
    {
        // Show rotating asterisk
        if(!(console_displayed->properties & CONSOLE_FULLSCREEN))
        {
            vga_setPixel(79, 49, (FOOTNOTE<<8) | *progress); // Write the character on the screen. (color|character)
        }
        if (*++progress == 0)
        {
            progress-=5; // Reset asterisk
        }

        // Handle events. TODO: Many of the shortcuts can be moved to the shell later.
        char buffer[4];
        EVENT_t ev = event_poll(buffer, 4, EVENT_NONE); // Take one event from the event queue

        while (ev != EVENT_NONE)
        {
            switch (ev)
            {
                case EVENT_KEY_DOWN:
                    // Detect CTRL and ESC
                    switch (*(KEY_t*)buffer)
                    {
                        case KEY_ESC:
                            ESC = true;
                            break;
                        case KEY_LCTRL: case KEY_RCTRL:
                            CTRL = true;
                            break;
                        case KEY_PRINT: // Because of special behavior of the PRINT key in emulators, we handle it in a special way:
                                        // After PRINT was pressed, next text-entered event is taken as argument to PRINT.
                        case KEY_F12:   // F12 is used as alias for PrintScreen due to problems in some emulators
                            PRINT = true;
                            break;
                        default:
                            break;
                    }
                    break;
                case EVENT_KEY_UP:
                    // Detect CTRL and ESC
                    switch (*(KEY_t*)buffer)
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
                    if (PRINT)
                    {
                        PRINT = false;
                        switch (*(char*)buffer)
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
                    if (ESC)
                    {
                        switch (*(char*)buffer)
                        {
                            case 'h':
                                heap_logRegions();
                                break;
                            case 'p':
                                paging_analyzeBitTable();
                                break;
                        }
                    }
                    else if (CTRL)
                    {
                        switch (*(char*)buffer)
                        {
                            case 'a':
                                network_displayArpTables();
                                break;
                            case 'c':
                                tcp_showConnections();
                                break;
                            case 'd':
                                showPortList();
                                showDiskList();
                                break;
                            case 'i':
                                ipc_print();
                                break;
                            case 's':
                                initrd_loadShell();
                                break;
                            case 't':
                                scheduler_log();
                                break;
                            case 'v':
                                scheduler_insertTask(create_cthread(&video_test, "VBE"));
                                break;
                        }
                    }
                    break;
                default:
                    break;
            }
            ev = event_poll(buffer, 4, EVENT_NONE);
        }

        if (timer_getSeconds() != CurrentSeconds) // Execute one time per second
        {
            CurrentSeconds = timer_getSeconds();

            cpu_calculateFrequency();

            if(!(console_displayed->properties & CONSOLE_FULLSCREEN))
            {
                // Draw status bar with date, time and frequency
                char DateAndTime[50];
                getCurrentDateAndTime(DateAndTime, 50);
                kprintf("%s   %u s runtime. CPU: %u MHz    ", 49, FOOTNOTE, DateAndTime, CurrentSeconds, ((uint32_t)*cpu_frequency)/1000); // Output in status bar
            }

            deviceManager_checkDrives(); // Switch off motors if they are not neccessary
        }

        if (serial_received(1) != 0)
        {
            textColor(HEADLINE);
            printf("\nSerial message: ");
            textColor(DATA);

            do
            {
                printf("%y ", serial_read(1));
                sleepMilliSeconds(8);
            } while (serial_received(1) != 0);
        }

        todoList_execute(kernel_idleTasks);
        switch_context(); // Kernel idle loop has finished so far. Provide time to next waiting task
    }
}

/*
* Copyright (c) 2009-2012 The PrettyOS Project. All rights reserved.
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
