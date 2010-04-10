/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "kheap.h"
#include "paging.h"
#include "task.h"
#include "initrd.h"
#include "syscall.h"
#include "pci.h"
#include "cmos.h"
#include "time.h"
#include "flpydsk.h"
#include "sys_speaker.h"
#include "ehci.h"
#include "file.h"

/// PrettyOS Version string
const char* version = "0.0.0.347";

// RAM Detection by Second Stage Bootloader
#define ADDR_MEM_INFO    0x1000

// Buffer for User-Space Program
#define FILEBUFFERSIZE   0x4000 // shell

// RAM disk and user program
extern uint32_t file_data_start;
extern uint32_t file_data_end;

// pci devices list
extern pciDev_t pciDev_Array[PCIARRAYSIZE];

static void init()
{
    clear_screen();
    settextcolor(14,0);
    kernel_console_init();
    gdt_install();
    idt_install();         // cf. interrupts.asm
    timer_install(1000);   // Sets system frequency to ... Hz
    keyboard_install();
    mouse_install();
    syscall_install();
    fpu_install();
    pODA->Memory_Size = paging_install();
    heap_install();
    tasking_install();
    sti();
    settextcolor(15,0);
}

void showMemorySize()
{
    if (pODA->Memory_Size > 1073741824)
    {
        printf("Memory size: %u GiB / %u GB  (%u Bytes)\n",
        pODA->Memory_Size/1073741824, pODA->Memory_Size/1000000000, pODA->Memory_Size);
    }
    else if (pODA->Memory_Size > 1048576)
    {
        printf("Memory size: %u MiB / %u MB  (%u Bytes)\n",
        pODA->Memory_Size/1048576, pODA->Memory_Size/1000000, pODA->Memory_Size);
    }
    else
    {
        printf("Memory size: %u KiB / %u KB  (%u Bytes)\n",
        pODA->Memory_Size/1024, pODA->Memory_Size/1000, pODA->Memory_Size);
    }
}

void* ramdisk_install(size_t size)
{
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("rd_start: ");
    settextcolor(15,0);
    #endif

    void* ramdisk_start = malloc(size, PAGESIZE);
    // shell via incbin in data.asm
    memcpy(ramdisk_start, &file_data_start, (uintptr_t)&file_data_end - (uintptr_t)&file_data_start);
    fs_root = install_initrd(ramdisk_start);
    return(ramdisk_start);
}

int main()
{
    init();
    screenshot_Flag = false;
    EHCIflag        = false;  // first EHCI device found?
    initEHCIFlag    = false;  // any EHCI device found?
    portCheckFlag   = false;  // EHCI port change
    pODA->pciEHCInumber = 0;  // pci number of first EHCI device

    // Create Startup Screen
    create_cthread((task_t*)pODA->curTask, &bootscreen, "Booting ...");
    pODA->ts_flag = true;

    showMemorySize();

    floppy_install();// detect FDDs

    pciScan(); // scan of pci bus; results go to: pciDev_t pciDev_Array[PCIARRAYSIZE]; (cf. pci.h)
    listPCI();

    void* ramdisk_start = ramdisk_install(0x200000);

    // search and load shell
    bool shell_found = false;
    uint8_t* buf = malloc(FILEBUFFERSIZE, 0);
    struct dirent* node = 0;
    for (int i=0; (node = readdir_fs(fs_root, i))!=0; ++i)
    {
        fs_node_t* fsnode = finddir_fs(fs_root, node->name);

        if ((fsnode->flags & 0x7) == FS_DIRECTORY)
        {
            printf("<RAM Disk at %X DIR> %s\n", ramdisk_start, node->name);
        }
        else
        {
            uint32_t sz = read_fs(fsnode, 0, fsnode->length, buf);

            char name[40];
            memset(name, 0, 40);
            memcpy(name, node->name, 35); // protection against wrong / too long filename
            printf("%d \t%s\n",sz,name);

            if (strcmp((const char*)node->name, "shell") == 0)
            {
                shell_found = true;

                if (!elf_exec(buf, sz, "Shell"))
                    printf("Cannot start shell!\n");
            }
        }
    }
    free(buf);
    putch('\n');

    if (! shell_found)
    {
        settextcolor(4,0);
        printf("Program not found.\n");
        settextcolor(15,0);
    }

    const char* progress = "|/-\\";

    uint64_t LastRdtscValue = 0;

    uint32_t CurrentSeconds = 0xFFFFFFFF; // Set on a high value to force a refresh of the statusbar at the beginning.
    char DateAndTime[81]; // String for Date&Time

    while (true) // start of kernel idle loop
    {
        // Show Rotating Asterisk
        *((uint16_t*)(0xB8000 + sizeof(uint16_t)*(49*80 + 79))) = 0x0C00 | *progress;
        if (! *++progress){ progress = "|/-\\"; }

        if (getCurrentSeconds() != CurrentSeconds)
        {
            CurrentSeconds = getCurrentSeconds();

            // all values 64 bit
            uint64_t RdtscDiffValue = rdtsc() - LastRdtscValue;
            LastRdtscValue = rdtsc();
            uint64_t RdtscKCounts = RdtscDiffValue>>10; // divide by 1024
            uint32_t RdtscKCountsHi = RdtscKCounts >> 32;
            uint32_t RdtscKCountsLo = RdtscKCounts & 0xFFFFFFFF;

            if (RdtscKCountsHi==0){ pODA->CPU_Frequency_kHz = (RdtscKCountsLo/1000)<<10; }

            // draw separation line
            kprintf("--------------------------------------------------------------------------------", 48, 7); // Separation

            // draw status bar with date & time and frequency
            getCurrentDateAndTime(DateAndTime);
            kprintf("%s   %i s runtime. CPU: %i MHz    ", 49, 0x0C, // output in status bar
                    DateAndTime, CurrentSeconds, pODA->CPU_Frequency_kHz/1000);


            /////////////////////////////////////////////////////////////////////////////////////////
            // actions in kernel idle loop due to flags                                            //
            /////////////////////////////////////////////////////////////////////////////////////////

            if (screenshot_Flag == true)
            {
                screenshot_Flag = false;
                printf("Screenshot (Thread)\n");
                create_thread((task_t*)pODA->curTask, &screenshot_thread);
            }

            if ((initEHCIFlag == true) && (CurrentSeconds >= 3) && pODA->pciEHCInumber)
            {
                initEHCIFlag = false;
                create_cthread((task_t*)pODA->curTask, &startEHCI, "EHCI");
            }

            if ((portCheckFlag == true) && (CurrentSeconds >= 3) && pODA->pciEHCInumber)
            {
                portCheckFlag = false;
                create_cthread((task_t*)pODA->curTask, &portCheck, "EHCI Ports");
            }
        }

    __asm__ volatile ("hlt"); // HLT halts the CPU until the next external interrupt is fired.

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
