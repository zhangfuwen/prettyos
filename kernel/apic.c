/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "apic.h"
#include "cpu.h"
#include "video/console.h"
#include "util/util.h"


static volatile uint32_t* apic_base = 0;


// Some APIC registers
enum {
    APIC_TASKPRIORITY = 0x20,
    APIC_SPURIOUSINTERRUPT = 0x3C,
    APIC_TIMER = 0xC8,
    APIC_THERMALSENSOR = 0xCC,
    APIC_PERFORMANCECOUNTER = 0xD0,
    APIC_LINT0 = 0xD4,
    APIC_LINT1 = 0xD8,
    APIC_ERROR = 0xDC
};


bool apic_available(void)
{
    return(cpu_supports(CF_MSR) && cpu_supports(CF_APIC)); // We need MSR (to initialize APIC) and (obviously) APIC.;
}

bool apic_install(void)
{
    apic_base = (uint32_t*)(uintptr_t)(cpu_MSRread(0x1B) & (~0xFFF)); // APIC base address can be read from MSR 0x1B
    printf("\nAPIC base: %X", apic_base);

    apic_base[APIC_SPURIOUSINTERRUPT] = 0x10F; // Enable APIC. Spurious Vector is 15
    apic_base[APIC_TASKPRIORITY] = 0x20; // Inhibit software interrupt delivery
    apic_base[APIC_TIMER] = 0x10000; // Disable timer interrupts
    apic_base[APIC_THERMALSENSOR] = 0x10000; // Disable thermal sensor interrupts
    apic_base[APIC_PERFORMANCECOUNTER] = 0x10000; // Disable performance counter interrupts
    apic_base[APIC_LINT0] = 0x8700; // Enable normal external Interrupts
    apic_base[APIC_LINT1] = 0x400; // Enable NMI Processing
    apic_base[APIC_ERROR] = 0x10000; // Disable Error Interrupts

    return(true); // Successful
}


/*
* Copyright (c) 2012-2013 The PrettyOS Project. All rights reserved.
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

