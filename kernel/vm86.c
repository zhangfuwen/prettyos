// code derived on basic proposal at http://osdev.berlios.de/v86.html
// cf. "The Intel� 64 and IA-32 Architectures Software Developer�s Manual, Volumes 3A", 17.2 VIRTUAL-8086 MODE
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
#include "task.h"

current_t Current;
current_t* current = &Current;

context_v86_t context;

bool vm86sensitiveOpcodehandler(context_v86_t* ctx)
{
    uint8_t*  ip      = FP_TO_LINEAR(ctx->cs, ctx->eip);
    uint16_t* ivt     = NULL;
    uint16_t* stack   = (uint16_t*) FP_TO_LINEAR(ctx->ss, ctx->useresp);
    uint32_t* stack32 = (uint32_t*) stack;
    bool isOperand32 = false;
    bool isAddress32 = false;

  #ifdef _VM_DIAGNOSIS_
    printf("\nvm86sensitiveOpcodehandler: cs:eip = %x:%X ss:sp = %x:%x: ", ctx->cs, ctx->eip, ctx->ss, ctx->useresp);
  #endif
    // regarding opcodes, cf. "The Intel� 64 and IA-32 Architectures Software Developer�s Manual, Volumes 2A & 2B"

    while (true)
    {
        switch (ip[0])
        {
        case 0x66: // O32
          #ifdef _VM_DIAGNOSIS_
            printf("o32 ");
          #endif
            isOperand32 = true;
            ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip); 
            ctx->eip = (uintptr_t)ip;
            /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/
            break;

        case 0x67: // A32
          #ifdef _VM_DIAGNOSIS_
            printf("a32 ");
          #endif
            isAddress32 = true;
            ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip); 
            ctx->eip = (uintptr_t)ip;
            /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/
            break;

        case 0x9C: // PUSHF
          #ifdef _VM_DIAGNOSIS_  
            printf("pushf\n");
          #endif
            if (isOperand32)
            {
                ctx->useresp = ((ctx->useresp & 0xFFFF) - 4) & 0xFFFF;
                stack32--;
                stack32[0] = ctx->eflags & VALID_FLAGS;

                if (current->v86_if)
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

                if (current->v86_if)
                {
                    stack[0] |= EFLAG_IF;
                }
                else
                {
                    stack[0] &= ~EFLAG_IF;
                }
            }
            ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip); 
            ctx->eip = (uintptr_t)ip;
            /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/
            return true;

        case 0x9D: // POPF
          #ifdef _VM_DIAGNOSIS_
            printf("popf\n");
          #endif

            if (isOperand32)
            {
                ctx->eflags = EFLAG_IF | EFLAG_VM | (stack32[0] & VALID_FLAGS);
                current->v86_if = (stack32[0] & EFLAG_IF) != 0;
                ctx->useresp = ((ctx->useresp & 0xFFFF) + 4) & 0xFFFF;
            }
            else
            {
                ctx->eflags = EFLAG_IF | EFLAG_VM | stack[0];
                current->v86_if = (stack[0] & EFLAG_IF) != 0;
                ctx->useresp = ((ctx->useresp & 0xFFFF) + 2) & 0xFFFF;
            }
            ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip); 
            ctx->eip = (uintptr_t)ip;
            /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/
            return true;

        case 0xEF: // OUT DX, AX and OUT DX, EAX
 
            if (!isOperand32)
            {
              #ifdef _VM_DIAGNOSIS_
                printf("outportw(edx, ax)\n");
              #endif
                outportw(ctx->edx, ctx->eax);
            }
            else
            {
              #ifdef _VM_DIAGNOSIS_
                printf("outportl(edx, eax)\n");
              #endif
                outportl(ctx->edx, ctx->eax);
            }
            ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip); 
            ctx->eip = (uintptr_t)ip;
            /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/
            return true;

        case 0xEE: // OUT DX, AL
          #ifdef _VM_DIAGNOSIS_
            printf("outportb(edx, al)\n");
          #endif
            outportb(ctx->edx, ctx->eax);
            ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip); 
            ctx->eip = (uintptr_t)ip;
            /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/
            return true;

        case 0xED: // IN AX,DX and IN EAX,DX
          #ifdef _VM_DIAGNOSIS_
            printf("inportw(edx)\n");
          #endif
            if (!isOperand32)
            {
              #ifdef _VM_DIAGNOSIS_
                printf("inportw(edx)\n");
              #endif
                ctx->eax = (ctx->eax & 0xFFFF0000) + inportw(ctx->edx);
            }
            else
            {
              #ifdef _VM_DIAGNOSIS_
                printf("inportl(edx)\n");
              #endif
                ctx->eax = inportl(ctx->edx);
            }
            ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip); 
            ctx->eip = (uintptr_t)ip;
            /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/
            return true;

        case 0xEC: // IN AL,DX
          #ifdef _VM_DIAGNOSIS_
            printf("inportb(edx)\n");
          #endif
            ctx->eax = (ctx->eax & 0xFFFFFF00) + inportb(ctx->edx);
            ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip); 
            ctx->eip = (uintptr_t)ip;
            /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/
            return true;

        case 0xCD: // INT imm8
          #ifdef _VM_DIAGNOSIS_
            printf("interrupt %X => ", ip[1]);
          #endif
            switch (ip[1])
            {
            case 0x30:
              #ifdef _VM_DIAGNOSIS_
                printf("syscall\n");
              #endif
	            ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
                ctx->eip = (uintptr_t)ip; // !!!                
                /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/ // ??
                return true;

            case 0x20:
            case 0x21:
                ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
                ctx->eip = (uintptr_t)ip; // !!!
                /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/ // ??
                return false;

            default:
                stack -= 3;
                ctx->useresp = ((ctx->useresp & 0xFFFF) - 6) & 0xFFFF;
                stack[2] = ctx->eip + 2;
                stack[1] = ctx->cs;
                stack[0] = ctx->eflags;

                if (current->v86_if)
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
                ctx->eip = (uintptr_t)ip; // !!!
              #ifdef _VM_DIAGNOSIS_
                printf("%x:%x\n", ctx->cs, ctx->eip);
              #endif
                return true;
            }
            break;

        case 0xCF: // IRET
          #ifdef _VM_DIAGNOSIS_
            printf("iret => ");
          #endif
            ctx->eip    = stack[2]; 
            ctx->cs     = stack[1];
            ip = FP_TO_LINEAR(ctx->cs, ctx->eip);
            ctx->eip = (uintptr_t)ip; // !!!
            ctx->eflags = EFLAG_IF | EFLAG_VM | stack[0];
            ctx->useresp    = ((ctx->useresp & 0xFFFF) + 6) & 0xFFFF;

            current->v86_if = (stack[0] & EFLAG_IF) != 0;

          #ifdef _VM_DIAGNOSIS_
            printf("%x:%x\n", ctx->cs, ctx->eip);
          #endif
            return true;

        case 0xFA: // CLI
            printf("cli\n");
            current->v86_if = false;
            ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip); 
            ctx->eip = (uintptr_t)ip;
            /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/            
            return true;

        case 0xFB: // STI
          #ifdef _VM_DIAGNOSIS_
            printf("sti\n");
          #endif
            current->v86_if = true;
            ctx->eip++; ip = FP_TO_LINEAR(ctx->cs, ctx->eip); 
            ctx->eip = (uintptr_t)ip;
            /*printf("eip: %X ip: %X\n",ctx->eip,ip);*/
            return true;

        case 0xF4: // HLT
            exit();
            return true;

        default: // should not happen!
          #ifdef _VM_DIAGNOSIS_
            printf("error: unhandled opcode %X\n", ip[0]);
          #endif
            return false;
        }
    }
    return false;
}
