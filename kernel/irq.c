/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "irq.h"
#include "util.h"
#include "task.h"
#include "kheap.h"
#include "vm86.h"

typedef void(*interrupt_handler_t)(registers_t*);

extern context_v86_t context; // vm86.c

// Array of function pointers handling custom ir handlers for a given ir
static interrupt_handler_t interruptRoutines[256];

// Implement a custom ir handler for the given ir
void irq_installHandler(IRQ_NUM_t irq, interrupt_handler_t handler)
{
    interruptRoutines[irq+32] = handler;
}

// Clear the custom ir handler
void irq_uninstallHandler(IRQ_NUM_t irq)
{
    interruptRoutines[irq+32] = 0;
}


void waitForIRQ(IRQ_NUM_t number)
{
    scheduler_blockCurrentTask(&BL_INTERRUPT, (void*)(number+32));
}


// Message string corresponding to the exception number 0-31: exceptionMessages[interrupt_number]
const char* exceptionMessages[] =
{
    "Division By Zero",        "Debug",                         "Non Maskable Interrupt",    "Breakpoint",
    "Into Detected Overflow",  "Out of Bounds",                 "Invalid Opcode",            "No Coprocessor",
    "Double Fault",            "Coprocessor Segment Overrun",   "Bad TSS",                   "Segment Not Present",
    "Stack Fault",             "General Protection Fault",      "Page Fault",                "Unknown Interrupt",
    "Coprocessor Fault",       "Alignment Check",               "Machine Check",             "Reserved",
    "Reserved",                "Reserved",                      "Reserved",                  "Reserved",
    "Reserved",                "Reserved",                      "Reserved",                  "Reserved",
    "Reserved",                "Reserved",                      "Reserved",                  "Reserved"
};

static void quitTask()
{
    printf("| <Severe Failure - Task Halted> Press key for exit! |");
    sti();
    while(!keyboard_getChar());
    exit();
    for (;;);
}

static void defaultError(registers_t* r)
{
    if(r->eflags & 0x20000) return; // VM86
    printf("\nerr_code: %u address(eip): %X\n", r->err_code, r->eip);
    printf("edi: %X esi: %X ebp: %X eax: %X ebx: %X ecx: %X edx: %X\n", r->edi, r->esi, r->ebp, r->eax, r->ebx, r->ecx, r->edx);
    printf("cs: %x ds: %x es: %x fs: %x gs %x ss %x\n", r->cs, r->ds, r->es, r->fs, r->gs, r->ss);
    printf("int_no %u eflags %X useresp %X\n", r->int_no, r->eflags, r->useresp);

    textColor(0x0B);
    printf("\n\n%s!\n", exceptionMessages[r->int_no]);

    quitTask();
}

static void invalidOpcode(registers_t* r)
{
    printf("\nInvalid Opcode err_code: %u address(eip): %X", r->err_code, r->eip);
    /*printf("edi: %X esi: %X ebp: %X eax: %X ebx: %X ecx: %X edx: %X\n", r->edi, r->esi, r->ebp, r->eax, r->ebx, r->ecx, r->edx);
    printf("cs: %x ds: %x es: %x fs: %x gs %x ss %x\n", r->cs, r->ds, r->es, r->fs, r->gs, r->ss);
    printf("int_no %u eflags %X useresp %X\n", r->int_no, r->eflags, r->useresp);*/
}

static void NM(registers_t* r) // -> FPU
{
    // set TS in cr0 to zero
    __asm__ ("CLTS"); // CLearTS: reset the TS bit (no. 3) in CR0 to disable #NM

    kdebug(3, "#NM: FPU is used. pCurrentTask: %X\n", currentTask);

    // save FPU data ...
    if (FPUTask)
    {
        // fsave or fnsave to FPUTask->FPUptr
        __asm__ volatile("fsave %0" :: "m" (*(uint8_t*)(FPUTask->FPUptr)));
    }

    // store the last task using FPU
    FPUTask = currentTask;

    // restore FPU data ...
    if (currentTask->FPUptr)
    {
        // frstor from pCurrentTask->FPUptr
        __asm__ volatile("frstor %0" :: "m" (*(uint8_t*)(currentTask->FPUptr)));
    }
    else
    {
        // allocate memory to pCurrentTask->FPUptr
        currentTask->FPUptr = (uintptr_t)malloc(108,4,"FPUptr"); // http://siyobik.info/index.php?module=x86&id=112
    }
}

static void GPF(registers_t* r) // -> VM86
{
    if (r->eflags & 0x20000) // VM bit - it is a VM86-task
    {
        context_v86_t* ctx = &context;

        ctx->cs      = r->cs;
        ctx->eip     = r->eip;
        ctx->ss      = r->ss;
        ctx->eflags  = r->eflags;
        ctx->ds      = r->ds;
        ctx->es      = r->es;
        ctx->fs      = r->fs;
        ctx->gs      = r->gs;
        ctx->useresp = r->useresp;
        ctx->eax     = r->eax;
        ctx->ebx     = r->ebx;
        ctx->ecx     = r->ecx;
        ctx->edx     = r->edx;
        ctx->edi     = r->edi;
        ctx->esi     = r->esi;
        textColor(0x0C);
        if (vm86sensitiveOpcodehandler(ctx)) // OK
        {
            /*
            textColor(0x03);
            //printf("\nretVal i386V86Gpf: %u\n", retVal); // vm86 critical
            textColor(0x0C);
            */
        }
        else
        {
            printf("\nvm86: sensitive opcode error)\n");
        }

        r->cs      = ctx->cs;
        r->eip     = ctx->eip;
        r->ss      = ctx->ss;
        r->eflags  = ctx->eflags;
        r->ds      = ctx->ds;
        r->es      = ctx->es;
        r->fs      = ctx->fs;
        r->gs      = ctx->gs;
        r->eax     = ctx->eax;
        r->ebx     = ctx->ebx;
        r->ecx     = ctx->ecx;
        r->edx     = ctx->edx;
        r->edi     = ctx->edi;
        r->esi     = ctx->esi;
        r->useresp = ctx->useresp;
    }
    else
        defaultError(r);
}

static void PF(registers_t* r)
{
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address)); // faulting address <== CR2 register

    // The error code gives us details of what happened.
    int32_t present  = !r->err_code & 0x1;  // Page not present
    int32_t rw       =  r->err_code & 0x2;  // Write operation?
    int32_t us       =  r->err_code & 0x4;  // Processor was in user-mode?
    int32_t reserved =  r->err_code & 0x8;  // Overwritten CPU-reserved bits of page entry?
    int32_t id       =  r->err_code & 0x10; // Caused by an instruction fetch?

    // Output an error message.
    printf("\nPage Fault (");
    if (present)  printf("page not present");
    if (rw)       printf(" - read-only - write operation");
    if (us)       printf(" - user-mode");
    if (reserved) printf(" - overwritten CPU-reserved bits of page entry");
    if (id)       printf(" - caused by an instruction fetch");
    printf(") at %X - EIP: %X\n", faulting_address, r->eip);

    quitTask();
}


void isr_install()
{
    for(uint8_t i = 0; i < 32; i++) interruptRoutines[i] = &defaultError; // If nothing else is specified, the default handler is called
    for(uint16_t i = 32; i < 256; i++) interruptRoutines[i] = 0;

    // Installing ISR-Routines
    interruptRoutines[ISR_invalidOpcode] = &invalidOpcode;
    interruptRoutines[ISR_NM] = &NM;
    interruptRoutines[ISR_GPF] = &GPF;
    interruptRoutines[ISR_PF] = &PF;
}

uint32_t irq_handler(uintptr_t esp)
{
    task_t* oldTask = currentTask;       // Save old task to be able to restore attr in case of task_switch
    uint8_t attr = currentTask->attrib;  // Save the attrib so that we do not get color changes after the Interrupt if it changed the attrib
    currentConsole = kernelTask.console; // The output should appear in the kernels console usually

    registers_t* r = (registers_t*)esp;

    if(r->int_no == 0x20 || r->int_no == 0x7E) // timer interrupt or function switch_ctx
    {
        if(task_switching)
            esp = task_switch(esp); // new task's esp
    }

    if (interruptRoutines[r->int_no])
        interruptRoutines[r->int_no](r); // Execute handler

    scheduler_unblockEvent(&BL_INTERRUPT, (void*)r->int_no);

    if (r->int_no >= 40)
        outportb(0xA0, 0x20);
    outportb(0x20, 0x20);

    currentConsole = currentTask->console;
    oldTask->attrib = attr;
    return esp;
}

/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
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
