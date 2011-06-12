/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "cpu.h"
#include "video/console.h"


// http://www.lowlevel.eu/wiki/Cpuid

static bool cpuid_available = true;

char cpu_vendor[13];

void cpu_analyze()
{
    printf("CPU information:");

    // Test if the CPU supports the CPUID-Command
    __asm__ volatile ("pushfl\n\t"
                      "pop %ecx\n\t"
                      "mov %ecx, %eax\n\t"
                      "xor %eax, 0x200000\n\t"
                      "push %eax\n\t"
                      "popfl\n\t"
                      "pushfl\n\t"
                      "pop %eax\n\t");
    register uint32_t eax __asm__("%eax");
    register uint32_t ecx __asm__("%ecx");
    cpuid_available = (eax==ecx);
    if(!cpuid_available)
    {
        printf(" CPU does not support cpuid instruction.\n");
        return;
    }

    // Read out VendorID
    ((uint32_t*)cpu_vendor)[0] = cpu_idGetRegister(0, CR_EBX);
    ((uint32_t*)cpu_vendor)[1] = cpu_idGetRegister(0, CR_EDX);
    ((uint32_t*)cpu_vendor)[2] = cpu_idGetRegister(0, CR_ECX);
    cpu_vendor[12] = 0;

    printf(" VendorID: %s\n", cpu_vendor);
}

bool cpu_supports(CPU_FEATURE feature)
{
    if(feature == CF_CPUID) return(cpuid_available);
    if(!cpuid_available) return(false);

    CPU_REGISTER r = feature&~31;
    return(cpu_idGetRegister(0x00000001, r) & (BIT(feature-r)));
}

uint32_t cpu_idGetRegister(uint32_t function, CPU_REGISTER reg)
{
    if(!cpuid_available) return(0);

    __asm__ ("movl %0, %%eax" : : "r"(function) : "%eax");
    __asm__ ("cpuid");
    switch(reg)
    {
        case CR_EAX:
        {
            register uint32_t temp __asm__("%eax");
            return(temp);
        }
        case CR_EBX:
        {
            register uint32_t temp __asm__("%ebx");
            return(temp);
        }
        case CR_ECX:
        {
            register uint32_t temp __asm__("%ecx");
            return(temp);
        }
        case CR_EDX:
        {
            register uint32_t temp __asm__("%edx");
            return(temp);
        }
        default:
            return(0);
    }
}

uint64_t cpu_MSRread(uint32_t msr)
{
    uint32_t low, high;

    __asm__ volatile ("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));

    return ((uint64_t)high << 32) | low;
}

void cpu_MSRwrite(uint32_t msr, uint64_t value)
{
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;

    __asm__ __volatile__ ("wrmsr" :: "a"(low), "c"(msr), "d"(high));
}


/*
* Copyright (c) 2010-2011 The PrettyOS Project. All rights reserved.
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
