/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "cmos.h"
#include "util.h"
#include "video/console.h"
#include "task.h"

task_t* FPUTask;

static void fpu_setcw(uint16_t ctrlword)
{
    // FLDCW = Load FPU Control Word
    __asm__ volatile("fldcw %0;"::"m"(ctrlword));
}

void fpu_install()
{
    if (! (cmos_read(0x14) & BIT(1)) )
    {
        printf("Math Coprozessor not available\n");
        return;
    }

    __asm__ volatile ("finit");

    fpu_setcw(0x37F); // set the FPU Control Word

    // set TS in cr0
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0": "=r"(cr0)); // read cr0
    cr0 |= 0x8; // set the TS bit (no. 3) in CR0 to enable #NM (exception no. 7)
    __asm__ volatile("mov %0, %%cr0":: "r"(cr0)); // write cr0

    // init TaskFPU
    FPUTask = 0;
}

void fpu_test()
{
    double squareroot = sqrt(2.0);
    squareroot = fabs(squareroot);
    squareroot /= sqrt(2.0);
    if (squareroot == 1.00) 
    {
        textColor(0x0A);
        printf("\nFPU-test OK\n");
        textColor(0x0F);
    }
    else
    {
       textColor(0x0C); 
       printf("\nFPU-test ERROR\n");
       textColor(0x0F);
    }
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
