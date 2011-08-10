/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/


// Enable pc-speaker-simulation in qemu: -soundhw pcspk

#include "sys_speaker.h"
#include "util.h"
#include "timer.h"
#include "pit.h"

void sound(uint32_t frequency)
{
    uint8_t  temp;

    // calculate our divisor
    uint16_t divisor = TIMECOUNTER_i8254_FREQU / frequency; //divisor must fit into 16 bits; PIT (programable interrupt timer)

    // set commandregister to 0xB6 = 10 11 011 0 // Select channel, Access mode, Operating mode, BCD/Binary mode
    outportb(COMMANDREGISTER,
        BIT(1) | BIT(2) | // Mode 2 (rate generator)     |
        BIT(4) | BIT(5) | // Access mode: lobyte/hibyte
        BIT(7));          // Channel 2

    // send divisor
    outportb(CHANNEL_2_DATAPORT, (uint8_t)(divisor       & 0xFF)); // low  byte
    outportb(CHANNEL_2_DATAPORT, (uint8_t)((divisor >> 8) & 0xFF)); // high byte

    // sound on
    temp = inportb(CHANNEL_2_CONTROLPORT);
      if (temp != (temp | 3))
      {
         outportb(CHANNEL_2_CONTROLPORT, temp | 3);
      }
}

void noSound()
{
    outportb(CHANNEL_2_CONTROLPORT, inportb(CHANNEL_2_CONTROLPORT) & ~3);
}

void beep(uint32_t freq, uint32_t duration)
{
    sti();
    sound(freq);
    sleepMilliSeconds(duration);
    noSound();
}

void msgbeep()
{
    beep(440,1000);
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
