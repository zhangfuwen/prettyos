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

#include "console.h"
#include "vm86.h"
#include "util.h"

current_t Current;
current_t* current = &Current;

context_v86_t context;

FARPTR i386LinearToFp(void *ptr)
{
    unsigned seg, off;
    off = (uintptr_t) ptr & 0xF;
    seg = ((uintptr_t) ptr - ((uintptr_t) ptr & 0xF)) / 16;
    return MK_FP(seg, off);
}

bool i386V86Gpf(context_v86_t* ctx)
{
    uint8_t*  ip      = FP_TO_LINEAR(ctx->cs, ctx->eip);
    uint16_t* ivt     = NULL;
    uint16_t* stack   = (uint16_t*) FP_TO_LINEAR(ctx->ss, ctx->esp);
    uint32_t* stack32 = (uint32_t*) stack;
    bool is_operand32 = false;
    bool is_address32 = false;

    printf("\ni386V86Gpf: cs:ip = %x:%x ss:sp = %x:%x: ", ctx->cs, ctx->eip, ctx->ss, ctx->esp);

    // regarding opcodes, cf. "The Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volumes 2A & 2B"

    while (true)
    {
        switch (ip[0])
        {
        case 0x66:            // O32
            printf("o32 ");
            is_operand32 = true;
            ip++;
            ctx->eip = (uint16_t) (ctx->eip + 1);
            break;

        case 0x67:            // A32
            printf("a32 ");
            is_address32 = true;
            ip++;
            ctx->eip = (uint16_t) (ctx->eip + 1);
            break;

        case 0x9C:            // PUSHF
            printf("pushf\n");

            if (is_operand32)
            {
                ctx->esp = ((ctx->esp & 0xFFFF) - 4) & 0xFFFF;
                stack32--;
                stack32[0] = ctx->eflags /*& VALID_FLAGS*/;

                if (current->v86_if)
                {
                    stack32[0] |= EFLAG_IF;
                    printf(" stack32[0]: %X",stack32[0]);
                }
                else
                {
                    stack32[0] &= ~EFLAG_IF;
                    printf(" stack32[0]: %X",stack32[0]);
                }
            }
            else
            {
                ctx->esp = ((ctx->esp & 0xFFFF) - 2) & 0xFFFF;
                stack--;
                stack[0] = (uint16_t) ctx->eflags;

                if (current->v86_if)
                {
                    stack[0] |= EFLAG_IF;
                    printf(" stack[0]: %X",stack[0]);
                }
                else
                {
                    stack[0] &= ~EFLAG_IF;
                    printf(" stack[0]: %X",stack[0]);
                }
            }
            ctx->eip = (uint16_t) (ctx->eip + 1);
            return true;

        case 0x9D:            // POPF
            printf("popf\n");

            if (is_operand32)
            {
                ctx->eflags = EFLAG_IF | EFLAG_VM | (stack32[0] /*& VALID_FLAGS*/);
                current->v86_if = (stack32[0] & EFLAG_IF) != 0;
                ctx->esp = ((ctx->esp & 0xFFFF) + 4) & 0xFFFF;
                printf(" *(ctx->esp): %X  esp: %X if: %u", *((uint8_t*)(ctx->esp)), ctx->esp, current->v86_if);
            }
            else
            {
                ctx->eflags = EFLAG_IF | EFLAG_VM | stack[0];
                current->v86_if = (stack[0] & EFLAG_IF) != 0;
                ctx->esp = ((ctx->esp & 0xFFFF) + 2) & 0xFFFF;
                printf(" *(ctx->esp): %X  esp: %X if: %u", *((uint8_t*)(ctx->esp)), ctx->esp, current->v86_if);
            } 
            ctx->eip = (uint16_t) (ctx->eip + 1);
            return true;


        case 0xEF: // OUT DX, AX and OUT DX, EAX
            printf("outportw(edx, eax)\n");
            outportw(ctx->edx, ctx->eax);
            ctx->eip = (uint16_t) (ctx->eip + 1);
            return true;

        case 0xEE: // OUT DX, AL
            printf("outportb(edx, eax)\n");
            outportb(ctx->edx, ctx->eax);
            ctx->eip = (uint16_t) (ctx->eip + 1);
            return true;

        case 0xED: // inw
            printf("inportb(edx)\n");
            ctx->eax = inportb(ctx->edx);
            ctx->eip = (uint16_t) (ctx->eip + 1);
            return true;

        case 0xEC: // inb
            printf("inportw(edx)\n");
            ctx->eax = (ctx->eax & 0xFF00) + inportb(ctx->edx);
            ctx->eip = (uint16_t) (ctx->eip + 1);
            return true;


        case 0xCD:          // INT imm8
            printf("interrupt %X => ", ip[1]);
            switch (ip[1])
            {
            case 0x30:
                printf("syscall\n");
				/*
                if (ctx->regs.eax == SYS_ThrExitThread)
				{
				}*/
                    // ThrExitThread(0);
                return true;

            case 0x20:
            case 0x21:
                /*i386V86EmulateInt21(ctx);*/
                
                /*
                if (current->v86_in_handler)
                {
                    return false;
                }

                printf("redirect to %X\n", current->v86_handler);
                current->v86_in_handler = true;
                current->v86_context = *ctx;
                current->kernel_esp += sizeof(context_v86_t) - sizeof(context_t);
                ctx->eflags = EFLAG_IF | 2;
                ctx->eip = current->v86_handler;
                ctx->cs = USER_FLAT_CODE | 3;
                ctx->ds = ctx->es = ctx->gs = ctx->ss = USER_FLAT_DATA | 3;
                ctx->fs = USER_THREAD_INFO | 3;
                ctx->esp = current->user_stack_top;
                return true;
                */
                return false;

            default:
                stack -= 3;
                ctx->esp = ((ctx->esp & 0xFFFF) - 6) & 0xFFFF;

                /*
                stack[0] = (uint16_t) (ctx->eip + 2);
                stack[1] = ctx->cs;
                stack[2] = (uint16_t) ctx->eflags;
                */

                // wie bei tyndur:
                stack[2] = (uint16_t) (ctx->eip + 2);
                stack[1] = ctx->cs;
                stack[0] = (uint16_t) ctx->eflags;

                if (current->v86_if)
                {
                    // stack[2] |= EFLAG_IF;
                    stack[0] |= EFLAG_IF;
                }
                else
                {
                    //stack[2] &= ~EFLAG_IF;
                    stack[0] &= ~EFLAG_IF;
                }

                ctx->cs = ivt[ip[1] * 2 + 1];
                ctx->eip = ivt[ip[1] * 2];
                printf("%x:%x\n", ctx->cs, ctx->eip);
                return true;
            }
            break;

        case 0xCF:            // IRET
            printf("iret => ");
            
            /*
            ctx->eip = stack[0];
            ctx->cs = stack[1];
            ctx->eflags = EFLAG_IF | EFLAG_VM | stack[2];
            */
            
            ctx->eip = stack[2];
            ctx->cs = stack[1];
            ctx->eflags = EFLAG_IF | EFLAG_VM | stack[0];
            
            
            // current->v86_if = (stack[2] & EFLAG_IF) != 0;
            current->v86_if = (stack[0] & EFLAG_IF) != 0;

            ctx->esp = ((ctx->esp & 0xFFFF) + 6) & 0xFFFF;
            printf("%x:%x\n", ctx->cs, ctx->eip);
            return true;

        case 0xFA:            // CLI
            printf("cli\n");
            current->v86_if = false;
            ctx->eip = (uint16_t) (ctx->eip + 1);
            return true;

        case 0xFB:            // STI
            printf("sti\n");
            current->v86_if = true;
            ctx->eip = (uint16_t) (ctx->eip + 1);
            return true;

        default: // should not happen!
            printf("error: unhandled opcode %X\n", ip[0]);
            return false;
        }
    }

    return false;
}
