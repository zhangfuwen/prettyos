/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "descriptor_tables.h"

#define NUMBER_GDT_GATES 6 // 0-5: Null, Kernel Code, Kernel Data, User Code, User Data, TSS

/* Our GDT, and finally our special GDT pointer */
gdt_entry_t gdt[NUMBER_GDT_GATES];
gdt_ptr_t   gdt_register;

/* Setup a descriptor in the Global Descriptor Table */
void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    /* Setup the descriptor base address */
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    /* Setup the descriptor limits */
    gdt[num].limit_low   = ( limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    /* Finally, set up the granularity and access flags */
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access      = access;
}

void gdt_install()
{
    /* Setup the GDT pointer and limit */
    gdt_register.limit = (sizeof(struct gdt_entry) * NUMBER_GDT_GATES)-1;
    gdt_register.base  = (uint32_t) &gdt;

    /* GDT GATES -  desriptors with pointers to the linear memory address */
    gdt_set_gate(0,0,0,0,0); // NULL descriptor

    /*          num base limit     access                                               gran */
    gdt_set_gate(1, 0, 0xFFFFFFFF, VALID | RING_0 | CODE_DATA_STACK | CODE_EXEC_READ  , _4KB_ | USE32 | 0xF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, VALID | RING_0 | CODE_DATA_STACK | DATA_READ_WRITE , _4KB_ | USE32 | 0xF);
    gdt_set_gate(3, 0, 0xFFFFFFFF, VALID | RING_3 | CODE_DATA_STACK | CODE_EXEC_READ  , _4KB_ | USE32 | 0xF);
    gdt_set_gate(4, 0, 0xFFFFFFFF, VALID | RING_3 | CODE_DATA_STACK | DATA_READ_WRITE , _4KB_ | USE32 | 0xF);

    write_tss(5, 0x10, 0x0); // num, ss0, esp0
    gdt_flush((uint32_t)&gdt_register);
    tss_flush();
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
