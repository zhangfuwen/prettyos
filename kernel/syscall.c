/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "syscall.h"
#include "util.h"
#include "task.h"
#include "filesystem/fat12.h"
#include "audio/sys_speaker.h"
#include "timer.h"
#include "irq.h"
#include "storage/devicemanager.h"
#include "executable.h"

// Overwiew to all syscalls in documentation/Syscalls.odt

static void* syscalls[] =
{
/*  0  */    &executeFile,
/*  1  */    &nop, // createThread
/*  2  */    &exit,
/*  3  */    &nop, // semaphoreLock
/*  4  */    &sleepMilliSeconds,
/*  5  */    &nop, // waitForTask
/*  6  */    &getpid,
/*  7  */    &nop,
/*  8  */    &nop,
/*  9  */    &nop,

/*  10 */    &task_grow_userheap,
/*  11 */    &nop, // userheapFree
/*  12 */    &nop,
/*  13 */    &nop,
/*  14 */    &nop,

/*  15 */    &fopen,
/*  16 */    &fgetc,
/*  17 */    &fputc,
/*  18 */    &fseek,
/*  19 */    &fflush,
/*  20 */    &nop, // fmove
/*  21 */    &fclose,
/*  22 */    &formatPartition,
/*  23 */    &nop,
/*  24 */    &nop,

/*  25 */    &nop, // ipc_getString
/*  26 */    &nop, // ipc_setString
/*  27 */    &nop, // ipc_getInt
/*  28 */    &nop, // ipc_setInt
/*  29 */    &nop, // ipc_getDouble
/*  30 */    &nop, // ipc_setDouble
/*  31 */    &nop, // ipc_deleteKey
/*  32 */    &nop, // ipc_allowAccess
/*  33 */    &nop,
/*  34 */    &nop,
/*  35 */    &nop,
/*  36 */    &nop,
/*  37 */    &nop,
/*  38 */    &nop,
/*  39 */    &nop,

/*  40 */    &timer_getMilliseconds,
/*  41 */    &nop,
/*  42 */    &nop,
/*  43 */    &nop,
/*  44 */    &nop,
/*  45 */    &nop,
/*  46 */    &nop,
/*  47 */    &nop,
/*  48 */    &nop,
/*  49 */    &nop,

/*  50 */    &systemControl,
/*  51 */    &nop, // systemRefresh
/*  52 */    &nop,
/*  53 */    &nop,
/*  54 */    &nop,

/*  55 */    &putch,
/*  56 */    &textColor,
/*  57 */    &setScrollField,
/*  58 */    &setCursor,
/*  59 */    &getCursor,
/*  60 */    &nop, // readChar
/*  61 */    &clear_console,
/*  62 */    &nop,
/*  63 */    &nop,
/*  64 */    &nop,
/*  65 */    &nop,
/*  66 */    &nop,
/*  67 */    &nop,
/*  68 */    &nop,
/*  69 */    &nop,

/*  70 */    &getch,
/*  71 */    &keyPressed,
/*  72 */    &nop, // mousePressed
/*  73 */    &nop, // getMousePosition
/*  74 */    &nop, // setMousePosition
/*  75 */    &nop,
/*  76 */    &nop,
/*  77 */    &nop,
/*  78 */    &nop,
/*  79 */    &nop,

/*  80 */    &beep,
/*  81 */    &nop,
/*  82 */    &nop,
/*  83 */    &nop,
/*  84 */    &nop,

/*  85 */    &nop, // connect
/*  86 */    &nop, // receive
/*  87 */    &nop, // send
/*  88 */    &nop, // disconnect
/*  89 */    &nop,

// COMPATIBILITY (90-92); should be removed
    &flpydsk_read_directory,
    &cprintf
};

static void syscall_handler(registers_t* r);

void syscall_install()
{
    irq_installHandler(IRQ_SYSCALL, syscall_handler);
}

static void syscall_handler(registers_t* r)
{
    // Firstly, check if the requested syscall number is valid. The syscall number is found in EAX.
    if (r->eax >= sizeof(syscalls)/sizeof(*syscalls))
        return;

    currentConsole = currentTask->console; // Syscall output should appear in the console of the task that caused the syscall
    void* addr = syscalls[r->eax];

    // We don't know how many parameters the function wants.
    // Therefore, we push them all onto the stack in the correct order.
    // The function will use the number of parameters it wants.
    __asm__ volatile (
     "push %1; \
      push %2; \
      push %3; \
      push %4; \
      push %5; \
      call *%6; \
      add $20, %%esp;"
       : "=a" (r->eax) : "r" (r->edi), "r" (r->esi), "r" (r->edx), "r" (r->ecx), "r" (r->ebx), "r" (addr)); /*Ruinieren wir uns hier nicht den Stack? (Endloses Wachstum)*/

    currentConsole = kernelTask.console;
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
