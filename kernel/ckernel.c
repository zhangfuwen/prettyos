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
#include "list.h"
#include "sys_speaker.h"
#include "ehci.h"
#include "file.h"
#include "console.h"

/// PrettyOS Version string
const char* version = "0.0.0.319";

// RAM Detection by Second Stage Bootloader
#define ADDR_MEM_INFO    0x1000

// Buffer for User-Space Program
#define FILEBUFFERSIZE   0x2000

// RAM disk and user program
extern uint32_t file_data_start;
extern uint32_t file_data_end;

// pci devices list
extern pciDev_t pciDev_Array[PCIARRAYSIZE];

void set_fpu_control_word(const uint16_t cw)
{
    // FLDCW = Load FPU Control Word
    __asm__ volatile("fldcw %0;"::"m"(cw)); // sets the FPU control word to "cw"

}

// 9th bit (OSFXSR) in the CR4 tells the CPU
// that we intend on using the FXSAVE, FXRSTOR, and SSEx instructions.
void setup_x87_fpu()
{
    size_t cr4; // backup of CR4

    // place CR4 into our variable
    __asm__ volatile("mov %%cr4, %0;" : "=r" (cr4));

    // set the OSFXSR bit
    cr4 |= 0x200;

    // reload CR4 and INIT the FPU (FINIT)
    __asm__ volatile("mov %0, %%cr4; finit;" : : "r"(cr4));

    // set the FPU Control Word
    set_fpu_control_word(0x37F);

    // set TS in cr0
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0": "=r"(cr0)); // read cr0
    cr0 |= 0x8; // set the TS bit (no. 3) in CR0 to enable #NM (exception no. 7)
    __asm__ volatile("mov %0, %%cr0":: "r"(cr0)); // write cr0

   // init TaskFPU in ODA
   pODA->TaskFPU = (uintptr_t)NULL;
}

static void init()
{
    clear_screen();
    settextcolor(14,0);
    gdt_install();
    idt_install();
    timer_install();
    keyboard_install();
	mouse_install();
    syscall_install();
    setup_x87_fpu();
    settextcolor(15,0);
}

int main()
{
    init();
    pODA->Memory_Size = paging_install();
    heap_install();
    tasking_install();

	// Show Startup Screen
	bootscreen();


    if (pODA->Memory_Size > 1073741824)
    {
        printf("Memory size: %u GiB / %u GB  (%u Bytes)\n", pODA->Memory_Size/1073741824, pODA->Memory_Size/1000000000, pODA->Memory_Size);
    }
    else if (pODA->Memory_Size > 1048576)
    {
        printf("Memory size: %u MiB / %u MB  (%u Bytes)\n", pODA->Memory_Size/1048576, pODA->Memory_Size/1000000, pODA->Memory_Size);
    }
    else
    {
        printf("Memory size: %u KiB / %u KB  (%u Bytes)\n", pODA->Memory_Size/1024, pODA->Memory_Size/1000, pODA->Memory_Size);
    }

    EHCIflag     = false;  // first EHCI device found?
    initEHCIFlag = false;  //   any EHCI device found?
    pciScan(); // scan of pci bus; results go to: pciDev_t pciDev_Array[50]; (cf. pci.h)
    sti();

    // direct 1st floppy disk
    if ((cmos_read(0x10)>>4) == 4)   // 1st floppy 1,44 MB: 0100....b
    {
        printf("1.44 MB FDD device 0\n\n");
        pODA->flpy_motor[0] = false;        // first floppy motor is off
        flpydsk_set_working_drive(0); // set drive 0 as current drive
        flpydsk_install(32+6);        // floppy disk uses IRQ 6 // 32+6
        memset((void*)DMA_BUFFER, 0x0, 0x2400); // set DMA memory buffer to zero
    }
    else
    {
        printf("\n1.44 MB 1st floppy not shown by CMOS\n\n");
    }
    /// direct 1st floppy disk


    /// PCI list BEGIN
    // link valid devices from pciDev_t pciDev_Array[50] to a dynamic list
    listHead_t* pciDevList = listCreate();
    for (int i=0;i<PCIARRAYSIZE;++i)
    {
        if (pciDev_Array[i].vendorID && (pciDev_Array[i].vendorID != 0xFFFF) && (pciDev_Array[i].vendorID != 0xEE00))   // there is no vendor EE00h
        {
            listAppend(pciDevList, (void*)(pciDev_Array+i));
            ///
            #ifdef _DIAGNOSIS_
            settextcolor(2,0);
            printf("%X\t",pciDev_Array+i);
            #endif
            ///
        }
    }
    //printf("\n");
    // listShow(pciDevList); // shows addresses of list elements (not data)
    putch('\n');
    for (int i=0;i<PCIARRAYSIZE;++i)
    {
        void* element = listShowElement(pciDevList,i);
        if (element)
        {

            ///
            #ifdef _DIAGNOSIS_
            settextcolor(2,0);
            printf("%X dev: %x vend: %x\t",
                       (pciDev_t*)element,
                       ((pciDev_t*)element)->deviceID,
                       ((pciDev_t*)element)->vendorID);
            settextcolor(15,0);
            #endif
            ///
        }
    }
    ///
    #ifdef _DIAGNOSIS_
    puts("\n\n");
    #endif
    ///
    /// PCI list END

    // RAM Disk
    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("rd_start: ");
    settextcolor(15,0);
    #endif
    ///
    uint32_t ramdisk_start = (uint32_t)malloc(0x200000, PAGESIZE);
    settextcolor(15,0);

    // test with data and program from data.asm
    memcpy((void*)ramdisk_start, &file_data_start, (uint32_t)&file_data_end - (uint32_t)&file_data_start);
    fs_root = install_initrd(ramdisk_start);

    // search the content of files <- data from outside "loaded" via incbin ...
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

    pODA->ts_flag = 1;

    const char* progress = "|/-\\";
    const char* p = progress;
    uint32_t CurrentSeconds=0xFFFFFFFF; // Set on a high value to force a refresh of the display at the beginning.

    uint64_t CurrentRdtscValue=0, OldRdtscValue, RdtscDiffValue;


    // String for Date&Time
    char DateAndTime[81];
    char timeBuffer[20];

    while (true)
    {
        // Show Rotating Asterisk
        *((uint16_t*)(0xB8000 + 49*160+ 158)) = 0x0C00 | *p;
        if (! *++p)
        {
            p = progress;
        }

        // Show Date & Time at Status bar

        OldRdtscValue = CurrentRdtscValue;

        if (getCurrentSeconds() != CurrentSeconds)
        {
            kprintf("--------------------------------------------------------------------------------", 48, 7); // Separation
            CurrentSeconds = getCurrentSeconds();
            // all values 64 bit
            CurrentRdtscValue = rdtsc();
            RdtscDiffValue = CurrentRdtscValue - OldRdtscValue;
            uint64_t RdtscKCounts = RdtscDiffValue>>10; // divide by 1024

            uint32_t RdtscKCountsHi    = RdtscKCounts >> 32;
            uint32_t RdtscKCountsLo    = RdtscKCounts & 0xFFFFFFFF;

            if (RdtscKCountsHi==0)
            {
                uint32_t CPU_Frequency_kHz = (RdtscKCountsLo/1000)<<10;
                pODA->CPU_Frequency_kHz = CPU_Frequency_kHz;
            }
            else
            {
                // not to be expected
                // printf("\nRdtscKCountsHi: %d RdtscKCountsLo: %d\n",RdtscKCountsHi,RdtscKCountsLo);
            }

            itoa(CurrentSeconds, timeBuffer);
            getCurrentDateAndTime(DateAndTime);
            kprintf("%s   %i s runtime. CPU: %i MHz    ", 49, 0x0C, DateAndTime, CurrentSeconds, pODA->CPU_Frequency_kHz/1000); // output in status bar

            if (CurrentSeconds%120==3)
            {
                char timeStr[10];
                sprintf(timeStr, "TIME%s", timeBuffer);
                // screenshot(timeStr); ==> iDon't want a screenshot!
            }

            /// FPU-TEST
            if ((CurrentSeconds%5)==0)
            {
                float number1 = 2.5;
                float number2 = 2.5;
                float number3 = number1 * number2;
                char str[40];
                float2string(number3,3,str);
                printf("float result: %s\n",str);
            }
            /// TEST

            if ((initEHCIFlag == true) && (CurrentSeconds >= 3) && pciEHCINumber)
            {
                initEHCIFlag = false;
                initEHCIHostController(pciEHCINumber);
            }
        }
        __asm__ volatile ("hlt");
    }
    return 0;
}

/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
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
