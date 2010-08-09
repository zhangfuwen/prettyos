/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "timer.h"
#include "irq.h"
#include "task.h"

static uint16_t systemfrequency; // system frequency
static uint32_t timer_ticks = 0;

void timer_install(uint16_t sysfreq)
{
    // Installs 'timer_handler' to IRQ_TIMER
    irq_installHandler(IRQ_TIMER, timer_handler);

    timer_setFrequency(sysfreq); // x Hz, meaning a tick every 1000/x milliseconds
}

uint32_t timer_getSeconds()
{
    return(timer_ticks/systemfrequency);
}
uint32_t timer_getMilliseconds()
{
    return((timer_ticks*1000)/systemfrequency);
}

void timer_handler(registers_t* r)
{
    ++timer_ticks;
}

void timer_wait(uint32_t ticks)
{
    scheduler_blockCurrentTask(&BL_TIME, (void*)timer_ticks+ticks); // data: Wakeup-time casted to a pointer (32-bit integer)
}

bool timer_unlockTask(task_t* task)
{
    return((uint32_t)task->blocker.data <= timer_ticks);
}

void sleepSeconds(uint32_t seconds)
{
    sleepMilliSeconds(1000*seconds);
}

void sleepMilliSeconds(uint32_t ms)
{
    timer_wait(systemfrequency*ms/1000);
}

void timer_setFrequency(uint32_t freq)
{
    systemfrequency = freq;
    uint32_t divisor = 1193180 / systemfrequency; //divisor must fit into 16 bits; PIT (programable interrupt timer)

    // Send the command byte
    // outportb(0x43, 0x36); // 0x36 -> Mode 3 : Square Wave Generator
    outportb(0x43, 0x34); // 0x34 -> Mode 2 : Rate Generator // idea of +gjm+

    // Send divisor
    outportb(0x40, (uint8_t)(divisor      & 0xFF)); // low  byte
    outportb(0x40, (uint8_t)((divisor>>8) & 0xFF)); // high byte
}

uint16_t timer_getFrequency()
{
    return(systemfrequency);
}

void timer_uninstall()
{
    // Uninstalls IRQ_TIMER
    irq_uninstallHandler(IRQ_TIMER);
}

// delay in microseconds independent of timer interrupt but on rdtsc
void delay(uint32_t microsec)
{
    uint64_t timeout = rdtsc() + (uint64_t)(((uint32_t)(microsec/1000)) * system.CPU_Frequency_kHz);

    while (rdtsc()<timeout)
    {
       // nop();
    }
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
