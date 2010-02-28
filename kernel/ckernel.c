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

// RAM Detection by Second Stage Bootloader
#define ADDR_MEM_INFO    0x1000

// Buffer for User-Space Program
#define FILEBUFFERSIZE   0x2000

// RAM disk and user program
extern uint32_t file_data_start;
extern uint32_t file_data_end;

// String for Date&Time
char DateAndTime[80];

static void init()
{
    clear_screen();
    settextcolor(14,0);
    printformat("PrettyOS [Version 0.0.0.147]\n\n");
    gdt_install();
    idt_install();
    timer_install();
    keyboard_install();
    syscall_install();
    settextcolor(15,0);
}

int main()
{
    init();
    pODA->Memory_Size = paging_install();
    printformat( "\n\nMemory size: %d KB\n", pODA->Memory_Size/1024 );
    heap_install();
    tasking_install();
    pciScan(); // scan of pci bus; results go to: pciDev_t pciDev_Array[50]; (cf. pci.h)
    sti();

    // direct 1st floppy disk
    if( (cmos_read(0x10)>>4) == 4 )   // 1st floppy 1,44 MB: 0100....b
    {
        printformat("\n1.44 MB floppy disk has been installed as floppy device 0\n\n");
        pODA->flpy_motor[0] = false;        // first floppy motor is off
        flpydsk_set_working_drive(0); // set drive 0 as current drive
	    flpydsk_install(32+6);        // floppy disk uses IRQ 6 // 32+6
	    memset((void*)DMA_BUFFER, 0x0, 0x2400); // set DMA memory buffer to zero
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
            ///
            #ifdef _DIAGNOSIS_
            settextcolor(2,0);
            printformat("%X\t",pciDev_Array+i);
            #endif
            ///
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

            ///
            #ifdef _DIAGNOSIS_
            settextcolor(2,0);
            printformat("%X dev: %x vend: %x\t",
                       ( pciDev_t*)element,
                       ((pciDev_t*)element)->deviceID,
                       ((pciDev_t*)element)->vendorID);
            settextcolor(15,0);
            #endif
            ///
        }
    }
    ///
    #ifdef _DIAGNOSIS_
    printformat("\n\n");
    #endif
    ///
    /// TEST list END

    // RAM Disk
    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("rd_start: ");
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
    uint8_t* buf = malloc( FILEBUFFERSIZE, 0 );
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
            memset(name, 0, 40);
            memcpy(name, node->name, 35); // protection against wrong / too long filename
            printformat("%d \t%s\n",sz,name);

            if ( strcmp( (const char*)node->name, "shell" ) == 0 )
            {
                shell_found = true;

                if ( ! elf_exec( buf, sz ) )
                    printformat( "Cannot start shell!\n" );
            }
        }
    }
    free( buf );
    printformat("\n\n");

    if ( ! shell_found )
    {
        settextcolor(4,0);
        printformat("Program not found.\n");
        settextcolor(15,0);
    }

    msgbeep();
    pODA->ts_flag = 1;

    const char* progress = "|/-\\";
    const char* p = progress;
    uint32_t CurrentSeconds=0, CurrentSecondsOld;
    char timeBuffer[20];
    while( true )
    {
        // SHOW ROTATING ASTERISK
        *((uint16_t*)(0xB8000 + 49*160+ 158)) = 0x0C00 | *p;
        if ( ! *++p )
            p = progress;

        // PRINT TIME IN SECONDS AT STATUS BAR
        CurrentSecondsOld = CurrentSeconds;
        CurrentSeconds = getCurrentSeconds();

        if (CurrentSeconds!=CurrentSecondsOld)
        {
            itoa(CurrentSeconds, timeBuffer);
            getCurrentDateAndTime(DateAndTime);
            strcat(DateAndTime, "     ");
            strcat(DateAndTime, timeBuffer);
            strcat(DateAndTime, " seconds since start.");

            // output in status bar
            printf(DateAndTime, 49, 0xC);
        }
        __asm__ volatile ("hlt");
    }
    return 0;
}

/*
* Copyright (c) 2009 The PrettyOS Project. All rights reserved.
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

