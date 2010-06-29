/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "syscall.h"
#include "util.h"
#include "task.h"
#include "fat12.h"
#include "sys_speaker.h"
#include "timer.h"
#include "irq.h"
#include "devicemanager.h"

DEFN_SYSCALL1(puts,                       0, const char*)
DEFN_SYSCALL1(putch,                      1, char)
DEFN_SYSCALL1(textColor,                  2, uint8_t)
DEFN_SYSCALL0(getpid,                     3)
DEFN_SYSCALL0(nop,                        4)
DEFN_SYSCALL0(switch_context,             5)
DEFN_SYSCALL0(keyboard_getChar,           6)
DEFN_SYSCALL0(flpydsk_read_directory,     7)
DEFN_SYSCALL3(cprintf,                    8, const char*, uint32_t, uint8_t)
DEFN_SYSCALL0(getCurrentSeconds,          9)
DEFN_SYSCALL0(getCurrentMilliseconds,    10) // substitute
DEFN_SYSCALL1(flpydsk_format,            11, char*)
DEFN_SYSCALL2(flpydsk_load,              12, const char*, const char*) // substitute
DEFN_SYSCALL0(exit,                      13)
DEFN_SYSCALL1(keyPressed,                14, uint8_t) 
DEFN_SYSCALL2(beep,                      15, uint32_t, uint32_t)
DEFN_SYSCALL1(execute,                   16, const char*)
DEFN_SYSCALL0(systemControl,             17)
DEFN_SYSCALL1(clear_console,             18, uint8_t)
DEFN_SYSCALL2(set_cursor,                19, uint8_t, uint8_t)
DEFN_SYSCALL1(task_grow_userheap,        20, uint32_t)
DEFN_SYSCALL2(setScrollField,            21, uint8_t, uint8_t)

static void* syscalls[] =
{
    &puts,
    &putch,
    &textColor,
    &getpid,
    &nop,
    &switch_context,
    &keyboard_getChar,
    &flpydsk_read_directory,
    &cprintf,
    &getCurrentSeconds,
    &nop, // substitute
    &flpydsk_format,
    &nop, // substitute
    &exit,
    &keyPressed, 
    &beep,
    &executeFile,
    &systemControl,
    &clear_console,
    &set_cursor,
    &task_grow_userheap,
    &setScrollField
};

void syscall_install()
{
    irq_install_handler(127, syscall_handler);
}

void syscall_handler(registers_t* r)
{
    // Firstly, check if the requested syscall number is valid. The syscall number is found in EAX.
    if (r->eax >= sizeof(syscalls)/sizeof(*syscalls))
        return;

    void* addr = syscalls[r->eax];

    // We don't know how many parameters the function wants.
    // Therefore, we push them all onto the stack in the correct order.
    // The function will use the number of parameters it wants.
    int32_t ret;
    __asm__ volatile (" \
      push %1; \
      push %2; \
      push %3; \
      push %4; \
      push %5; \
      call *%6; \
      add $20, %%esp; \
    " : "=a" (ret) : "r" (r->edi), "r" (r->esi), "r" (r->edx), "r" (r->ecx), "r" (r->ebx), "r" (addr));
    r->eax = ret;
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
