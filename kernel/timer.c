/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "timer.h"
#include "util.h"
#include "pit.h"
#include "irq.h"
#include "task.h"
#include "cpu.h"


static uint16_t systemfrequency; // system frequency
static volatile uint64_t timer_ticks = 0;

void timer_install(uint16_t sysfreq)
{
    // Installs 'timer_handler' to IRQ_TIMER
    irq_installHandler(IRQ_TIMER, timer_handler);

    timer_setFrequency(sysfreq); // x Hz, meaning a tick every 1000/x milliseconds
}

uint32_t timer_getSeconds()
{
    return((uint32_t)timer_ticks/systemfrequency);
}
uint32_t timer_getMilliseconds()
{
    return(((uint32_t)timer_ticks*1000)/systemfrequency);
}
uint64_t timer_getTicks()
{
    return(timer_ticks);
}

uint32_t timer_millisecondsToTicks(uint32_t milliseconds)
{
    return((milliseconds*systemfrequency)/1000);
}

void timer_handler(registers_t* r)
{
    ++timer_ticks;
}

void sleepSeconds(uint32_t seconds)
{
    scheduler_blockCurrentTask(0, 0, max(1, 1000*seconds)); // "abuse" timeout function
}

void sleepMilliSeconds(uint32_t ms)
{
    scheduler_blockCurrentTask(0, 0, max(1, ms)); // "abuse" timeout function
}

void timer_setFrequency(uint32_t freq)
{
    systemfrequency  = freq;
    uint16_t divisor = TIMECOUNTER_i8254_FREQU / systemfrequency; //divisor must fit into 16 bits; PIT (programable interrupt timer)

    // Send the command byte
    outportb(COMMANDREGISTER, COUNTER_0 | RW_HI_LO_MODE | RATEGENERATOR);

    // cf. http://wiki.osdev.org/Programmable_Interval_Timer
    // Typically, OSes and BIOSes use mode 3 for PIT channel 0 to generate IRQ 0 timer ticks,
    // but some use mode 2 (rate generator with frequency divider) instead, to gain frequency accuracy.

    // Send divisor
    outportb(COUNTER_0_DATAPORT, BYTE1(divisor));
    outportb(COUNTER_0_DATAPORT, BYTE2(divisor));
}

uint16_t timer_getFrequency()
{
    return(systemfrequency);
}

// delay in microseconds independent of timer interrupt but on rdtsc
void delay(uint32_t microsec)
{
    uint64_t timeout = rdtsc() + (uint64_t)(((uint32_t)(microsec/1000)) * *cpu_frequency);

    while (rdtsc()<timeout) {}
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
