/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "timer.h"
#include "time.h"
#include "kheap.h"
#include "filesystem/initrd.h"
#include "storage/flpydsk.h"
#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "todo_list.h"
#include "syscall.h"
#include "pci.h"
#include "cdi.h"
#include "video/vbe.h"
#include "irq.h"
#include "serial.h"
#include "cpu.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "audio/sys_speaker.h"
#include "power_management.h"

const char* const version = "0.0.2.13 - Rev: 850";

// .bss
extern uintptr_t _bss_start;  // linker script
extern uintptr_t _kernel_end; // linker script

// Information about the system
system_t system;

void fpu_install(); // fpu.c
void fpu_test();    // fpu.c

// APIC
void apic_install()
{
    // ...  // TODO: implement APIC functionality
}

todoList_t* delayedInitTasks; // HACK! Why is it needed? (RTL8139 generates interrupts (endless) if its not used for EHCI)

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
    textColor(0x02);
    printf("[DONE]");
    textColor(0x07);
    printf("\t%s\n",str);
    textColor(0x0F);
}

static void init(multiboot_t* mb_struct)
{
    // set .bss to zero
    memset(&_bss_start, 0x0, (uintptr_t)&_kernel_end - (uintptr_t)&_bss_start);

    useMultibootInformation(mb_struct);

    isr_install();

    // descriptors
    gdt_install();
    idt_install(); // cf. interrupts.asm

    // video
    kernel_console_init();
    clear_screen();

    // internal devices
    timer_install(100); log("Timer"); // Sets system frequency to ... Hz
    if (cpu_supports(CF_FPU)) fpu_install(); log("FPU");
    if (cpu_supports(CF_APIC)) apic_install(); log("APIC");

    // memory
    system.Memory_Size = paging_install(); log("Paging");
    heap_install(); log("Heap");

    tasking_install(); log("Multitasking");

    // external devices
    keyboard_install(); log("Keyboard");
    mouse_install(); log("Mouse");

    pm_install(); log("Power Management");

    // system calls
    syscall_install(); log("Syscalls");

    cdi_init(); log("CDI");

    deviceManager_install(); log("Devicemanager"); // device management for mass storage devices
    fsmanager_install(); log("Filesystemmanager\n");

    delayedInitTasks = todoList_create();

    sti();
}

void showMemorySize()
{
    if (system.Memory_Size >= 0x40000000) // More than 1 GiB
    {
        printf("\nMemory size: %u GiB / %u GB  (%u MiB / %u MB, %u Bytes)\n", system.Memory_Size>>30, system.Memory_Size/1000000000, system.Memory_Size>>20, system.Memory_Size/1000000, system.Memory_Size);
    }
    else
    {
        printf("\nMemory size: %u MiB / %u MB  (%u Bytes)\n", system.Memory_Size>>20, system.Memory_Size/1000000, system.Memory_Size);
    }
}

void main(multiboot_t* mb_struct)
{
    init(mb_struct);

    create_cthread(&bootscreen, "Booting ...");

    showMemorySize();
    cpu_analyze();

    if (cpu_supports(CF_FPU)) fpu_test();
    pm_log();

    serial_init();

    pciScan();         // scan of pci bus; results go to: pciDev_t pciDev_Array[PCIARRAYSIZE]; (cf. pci.h)

    flpydsk_install(); // detect FDDs
    #ifdef _RAMDISK_DIAGNOSIS_
    void* ramdisk_start = initrd_install(ramdisk_install(), 0, 0x200000);
    #else
    initrd_install(ramdisk_install(), 0, 0x200000);
    #endif

    showPortList();
    showDiskList();

    // search and load shell
    textColor(0x0F);
    bool shell_found = false;
    struct dirent* node = 0;
    for (size_t i = 0; (node = readdir_fs(fs_root, i)) != 0; ++i)
    {
        fs_node_t* fsnode = finddir_fs(fs_root, node->name);

        if ((fsnode->flags & 0x7) == FS_DIRECTORY)
        {
            #ifdef _RAMDISK_DIAGNOSIS_
            printf("\n<RAMdisk (%X) - Root Directory>\n", ramdisk_start);
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
                if (!elf_exec(buf, sz, "Shell"))
                {
                    textColor(0x04);
                    printf("\nCannot start shell!\n");
                    textColor(0x0F);
                }
                free(buf);
            }
        }
    }
    if (!shell_found)
    {
        textColor(0x04);
        printf("\nProgram not found.\n");
        textColor(0x0F);
    }

    create_cthread(&vbe_bootscreen, "VBE");

    textColor(0x05);
    printf("--------------------------------------------------------------------------------");
    printf(  "                                PrettyOS Booted");
    printf("\n--------------------------------------------------------------------------------\n");
    textColor(0x0F);

    const char* progress    = "|/-\\";    // rotating asterisk
    uint64_t LastRdtscValue = 0;          // rdtsc: read time-stamp counter
    uint32_t CurrentSeconds = 0xFFFFFFFF; // Set on a high value to force a refresh of the statusbar at the beginning.
    char     DateAndTime[81];             // String for Date&Time

    while (true) // start of kernel idle loop
    {
        // show rotating asterisk
        ((uint16_t*)0xB8000)[49*80 + 79] = 0x0C00 | *progress; // Write the character directly to the video-memory. 0x0C00 is the color (red)
        if (! *++progress) { progress = "|/-\\"; }

        if (timer_getSeconds() != CurrentSeconds)
        {
            CurrentSeconds = timer_getSeconds();

            // all values 64 bit
            uint64_t RdtscDiffValue = rdtsc() - LastRdtscValue;
            LastRdtscValue = rdtsc();
            uint64_t RdtscKCounts   = RdtscDiffValue >> 10;         // division by 1024
            uint32_t RdtscKCountsHi = RdtscKCounts   >> 32;         // high dword
            uint32_t RdtscKCountsLo = RdtscKCounts   &  0xFFFFFFFF; // low dword

            if (RdtscKCountsHi == 0)
            {
                system.CPU_Frequency_kHz = (RdtscKCountsLo/1000) << 10;
            }

            // draw status bar with date & time and frequency
            getCurrentDateAndTime(DateAndTime);
            kprintf("%s   %u s runtime. CPU: %u MHz    ", 49, 0x0C, DateAndTime, CurrentSeconds, system.CPU_Frequency_kHz/1000); // output in status bar

            deviceManager_checkDrives(); // switch off motors if they are not neccessary
        }


        if (serial_received(1) != 0)
        {
            printf("Serial message: \n");
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
            printf("\nAnswered with 'Pretty'!\n\n");
        }

        todoList_execute(delayedInitTasks);

        if (keyPressed(VK_ESCAPE) && keyPressed(VK_H)) // kernel heap
        {
            logHeapRegions();
        }
        if (keyPressed(VK_ESCAPE) && keyPressed(VK_P)) // physical memory
        {
            paging_analyzeBitTable();
        }

        switch_context(); // Switch to another task
    } // end of kernel idle loop
}

/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
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
