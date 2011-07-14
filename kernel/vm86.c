/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// code derived on basic proposal at http://osdev.berlios.de/v86.html
// cf. "The Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volumes 3A", 17.2 VIRTUAL-8086 MODE
/*
17.2.7 Sensitive Instructions
When an IA-32 processor is running in virtual-8086 mode, the CLI, STI, PUSHF, POPF, INT n,
and IRET instructions are sensitive to IOPL. The IN, INS, OUT, and OUTS instructions,
which are sensitive to IOPL in protected mode, are not sensitive in virtual-8086 mode.
The CPL is always 3 while running in virtual-8086 mode; if the IOPL is less than 3, an
attempt to use the IOPL-sensitive instructions listed above triggers a general-protection
exception (#GP). These instructions are sensitive to IOPL to give the virtual-8086 monitor
a chance to emulate the facilities they affect.
*/

#include "video/console.h"
#include "vm86.h"
#include "util.h"
#include "task.h"
#include "serial.h"


static volatile uint32_t v86_if;

bool vm86_sensitiveOpcodehandler(registers_t* ctx)
{
    uint8_t*  ip      = FP_TO_LINEAR(ctx->cs, ctx->eip);
    uint16_t* ivt     = 0;
    uint16_t* stack   = (uint16_t*)FP_TO_LINEAR(ctx->ss, ctx->useresp);
    uint32_t* stack32 = (uint32_t*)stack;
    bool isOperand32 = false;

    // regarding opcodes, cf. "The Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volumes 2A & 2B"

    while (true)
    {
        switch (ip[0])
        {
        case 0x66: // O32
          #ifdef _VM_DIAGNOSIS_
            // printf("o32 "); // vm86 critical
            serial_log(1, "o32 ");
          #endif
            isOperand32 = true;
            ip++;
            ctx->eip++;
            break;

        case 0x67: // A32
          #ifdef _VM_DIAGNOSIS_
            // printf("a32 "); // vm86 critical
            serial_log(1, "a32 ");
          #endif
            //isAddress32 = true;
            ip++;
            ctx->eip++;
            break;

        case 0x9C: // PUSHF
          #ifdef _VM_DIAGNOSIS_
            // printf("pushf\r\n"); // vm86 critical
            serial_log(1, "pushf\r\r\n");
          #endif
            if (isOperand32)
            {
                ctx->useresp = ((ctx->useresp & 0xFFFF) - 4) & 0xFFFF;
                stack32--;
                stack32[0] = ctx->eflags & VALID_FLAGS;

                if (v86_if)
                {
                    stack32[0] |= EFLAG_IF;
                }
                else
                {
                    stack32[0] &= ~EFLAG_IF;
                }
            }
            else
            {
                ctx->useresp = ((ctx->useresp & 0xFFFF) - 2) & 0xFFFF;
                stack--;
                stack[0] = (uint16_t) ctx->eflags;

                if (v86_if)
                {
                    stack[0] |= EFLAG_IF;
                }
                else
                {
                    stack[0] &= ~EFLAG_IF;
                }
            }
            ctx->eip++;
            ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
            return true;

        case 0x9D: // POPF
          #ifdef _VM_DIAGNOSIS_
            // printf("popf\r\r\n"); // vm86 critical
            serial_log(1, "popf\r\r\n");
          #endif

            if (isOperand32)
            {
                ctx->eflags = EFLAG_IF | EFLAG_VM | (stack32[0] & VALID_FLAGS);
                v86_if = (stack32[0] & EFLAG_IF) != 0;
                ctx->useresp = ((ctx->useresp & 0xFFFF) + 4) & 0xFFFF;
            }
            else
            {
                ctx->eflags = EFLAG_IF | EFLAG_VM | stack[0];
                v86_if = (stack[0] & EFLAG_IF) != 0;
                ctx->useresp = ((ctx->useresp & 0xFFFF) + 2) & 0xFFFF;
            }
            ctx->eip++;
            ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
            return true;

        case 0xEF: // OUT DX, AX and OUT DX, EAX
            if (!isOperand32)
            {
             #ifdef _VM_DIAGNOSIS_
                // printf("outw\r\r\n"); // vm86 critical
                serial_log(1, "outw\r\r\n");
              #endif
                outportw(ctx->edx, ctx->eax);
            }
            else
            {
              #ifdef _VM_DIAGNOSIS_
                 // printf("outl(\r\r\n"); // vm86 critical
                 serial_log(1, "outl\r\r\n");
              #endif
                outportl(ctx->edx, ctx->eax);
            }
            ctx->eip++;
            ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
            return true;

        case 0xEE: // OUT DX, AL
          #ifdef _VM_DIAGNOSIS_
            // printf("outportb(edx, eax)\r\r\n"); // vm86 critical
            serial_log(1, "outportb(...)\r\n");
          #endif
            outportb(ctx->edx, ctx->eax);
            ctx->eip++;
            ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
            return true;

        case 0xED: // IN AX,DX and IN EAX,DX
            if (!isOperand32)
            {
              #ifdef _VM_DIAGNOSIS_
                 // printf("inw\r\n"); // vm86 critical
                 serial_log(1, "inw\r\n");
              #endif
                ctx->eax = (ctx->eax & 0xFFFF0000) + inportw(ctx->edx);
            }
            else
            {
              #ifdef _VM_DIAGNOSIS_
                 // printf("inl\r\n"); // vm86 critical
                 serial_log(1, "inl\r\n");
              #endif
                ctx->eax = inportl(ctx->edx);
            }
            ctx->eip++;
            ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
            return true;

        case 0xEC: // IN AL,DX
          #ifdef _VM_DIAGNOSIS_
            // printf("inportw(edx)\r\n"); // vm86 critical
            serial_log(1, "inportw(...)\r\n");
          #endif
            ctx->eax = (ctx->eax & 0xFF00) + inportb(ctx->edx);
            ctx->eip++;
            ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
            return true;

        case 0xCD: // INT imm8
          #ifdef _VM_DIAGNOSIS_
            // printf("interrupt %X => ", ip[1]); // vm86 critical
            serial_log(1, "interrupt ...\r\n");
          #endif
            switch (ip[1])
            {
            case 0x30:
              #ifdef _VM_DIAGNOSIS_
                // printf("syscall\r\n"); // vm86 critical
                serial_log(1, "syscall\r\n");
              #endif
                return true;

            case 0x20:
            case 0x21:
                return false;

            default:
                stack -= 3;
                ctx->useresp = ((ctx->useresp & 0xFFFF) - 6) & 0xFFFF;
                stack[2] = (uint16_t) (ctx->eip + 2);
                stack[1] = ctx->cs;
                stack[0] = (uint16_t) ctx->eflags;

                if (v86_if)
                {
                    stack[0] |= EFLAG_IF;
                }
                else
                {
                    stack[0] &= ~EFLAG_IF;
                }
                ctx->eip = ivt[2 * ip[1]    ];
                ctx->cs  = ivt[2 * ip[1] + 1];
                ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
              #ifdef _VM_DIAGNOSIS_
                // printf("%x:%x\r\n", ctx->cs, ctx->eip); // vm86 critical
                // ...
              #endif
                return true;
            }
            break;

        case 0xCF: // IRET
          #ifdef _VM_DIAGNOSIS_
            // printf("iret => "); // vm86 critical
            serial_log(1, "iret\r\n");
          #endif
            ctx->eip    = stack[2];
            ctx->cs     = stack[1];
            ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
            ctx->eflags = EFLAG_IF | EFLAG_VM | stack[0];
            ctx->useresp    = ((ctx->useresp & 0xFFFF) + 6) & 0xFFFF;

            v86_if = (stack[0] & EFLAG_IF) != 0;

          #ifdef _VM_DIAGNOSIS_
            // printf("%x:%x\r\n", ctx->cs, ctx->eip); // vm86 critical
            // ...
          #endif
            return true;

        case 0xFA: // CLI
          #ifdef _VM_DIAGNOSIS_
            // printf("cli\r\n"); // vm86 critical
            serial_log(1, "cli\r\n");
          #endif
            v86_if = false;
            ctx->eip++;
            ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
            return true;

        case 0xFB: // STI
          #ifdef _VM_DIAGNOSIS_
            // printf("sti\r\n"); // vm86 critical
            serial_log(1, "sti\r\n");
          #endif
            v86_if = true;
            ctx->eip++;
            ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
            return true;

        case 0xF4: // HLT
          #ifdef _VM_DIAGNOSIS_
            serial_log(1, "hlt\r\n");
          #endif
            exit();
            return true;

        default: // should not happen!
          #ifdef _VM_DIAGNOSIS_
            printf("error: unhandled opcode %X\r\n", ip[0]);
            serial_log(1, "error: unhandled opcode\r\n");
          #endif
            return false;
        }
    }
    return false;
}

void vm86_initPageDirectory(pageDirectory_t* pd, void* address, void* data, size_t size)
{
    pd->codes[0] |= MEM_USER | MEM_WRITE;
    for (uint16_t i=0; i<256; ++i) // Make first 1 MiB accessible
    {
        pd->tables[0]->pages[i] |= MEM_USER; // address = i*0x1000
    }

    // Allocate memory to store vm86 data
    void* paddress = (void*)alignDown((uintptr_t)address, PAGESIZE);
    size_t psize = alignUp(size, PAGESIZE);
    paging_alloc(pd, paddress, psize, MEM_USER | MEM_WRITE);
    // Copy vm86 data
    cli();
    paging_switch(pd);
    memcpy(address, data, size);
    paging_switch(currentTask->pageDirectory);
    sti();
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
