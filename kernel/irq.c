/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "task.h"
#include "kheap.h"
#include "flpydsk.h" // floppy motor off


typedef void(*interrupt_handler_t)(registers_t*);


/* Array of function pointers handling custom ir handlers for a given ir */
interrupt_handler_t irq_routines[256-32];

/* Implement a custom ir handler for the given ir */
void irq_install_handler(int32_t ir, interrupt_handler_t handler) {irq_routines[ir] = handler;}

/* Clear the custom ir handler */
void irq_uninstall_handler(int32_t ir) {irq_routines[ir] = 0;}



/* Message string corresponding to the exception number 0-31: exception_messages[interrupt_number] */
const char* exception_messages[] =
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

uint32_t irq_handler(uint32_t esp)
{
    registers_t* r = (registers_t*)esp;
    task_t* pCurrentTask = (task_t*)(pODA->curTask);

    if (r->int_no == 7) //exception #NM (number 7)
    {
        // set TS in cr0 to zero
        __asm__ ("CLTS"); // CLearTS: reset the TS bit (no. 3) in CR0 to disable #NM

        ///
        #ifdef _DIAGNOSIS_
        settextcolor(12,0);
        printf("#NM: FPU is used. pCurrentTask: %X\n",pCurrentTask);
        settextcolor(15,0);
        #endif
        ///

        // save FPU data ...
        if (pODA->TaskFPU)
        {
            // fsave or fnsave to pODA->TaskFPU->FPU_ptr
            __asm__ volatile("fsave %0" :: "m" (*(uint8_t*)(((task_t*)pODA->TaskFPU)->FPU_ptr)));
        }

        // store the last task using FPU
        pODA->TaskFPU = pCurrentTask;

        // restore FPU data ...
        if (pCurrentTask->FPU_ptr)
        {
            // frstor from pCurrentTask->FPU_ptr
            __asm__ volatile("frstor %0" :: "m" (*(uint8_t*)(pCurrentTask->FPU_ptr)));
        }
        else
        {
            // allocate memory to pCurrentTask->FPU_ptr
            pCurrentTask->FPU_ptr = (uintptr_t)malloc(108,4);
        }
    }

    if ((r->int_no < 32) && (r->int_no != 7)) //exception w/o #NM
    {
        settextcolor(12,0);
        flpydsk_control_motor(false); // floppy motor off

        if (r->int_no == 6 || r->int_no == 1) //Invalid Opcode
        {
            printf("err_code: %X address(eip): %X\n", r->err_code, r->eip);
            printf("edi: %X esi: %X ebp: %X eax: %X ebx: %X ecx: %X edx: %X\n", r->edi, r->esi, r->ebp, r->eax, r->ebx, r->ecx, r->edx);
            printf("cs: %X ds: %X es: %X fs: %X gs %X ss %X\n", r->cs, r->ds, r->es, r->fs, r->gs, r->ss);
            printf("int_no %d eflags %X useresp %X\n", r->int_no, r->eflags, r->useresp);
        }

        if (r->int_no == 14) //Page Fault
        {
            uint32_t faulting_address;
            __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address)); // faulting address <== CR2 register

            // The error code gives us details of what happened.
            int32_t present   = !(r->err_code & 0x1); // Page not present
            int32_t rw        =   r->err_code & 0x2;  // Write operation?
            int32_t us        =   r->err_code & 0x4;  // Processor was in user-mode?
            int32_t reserved  =   r->err_code & 0x8;  // Overwritten CPU-reserved bits of page entry?
            int32_t id        =   r->err_code & 0x10; // Caused by an instruction fetch?

            // Output an error message.
                          printf("\nPage Fault (");
            if (present)  printf("page not present");
            if (rw)       printf(" - read-only - write operation");
            if (us)       printf(" - user-mode");
            if (reserved) printf(" - overwritten CPU-reserved bits of page entry");
            if (id)       printf(" - caused by an instruction fetch");
                          printf(") at %X - EIP: %X\n", faulting_address, r->eip);
        }

        printf("err_code: %X address(eip): %X\n", r->err_code, r->eip);
        printf("edi: %X esi: %X ebp: %X eax: %X ebx: %X ecx: %X edx: %X\n", r->edi, r->esi, r->ebp, r->eax, r->ebx, r->ecx, r->edx);
        printf("cs: %X ds: %X es: %X fs: %X gs %X ss %X\n", r->cs, r->ds, r->es, r->fs, r->gs, r->ss);
        printf("int_no %d eflags %X useresp %X\n", r->int_no, r->eflags, r->useresp);

        printf("\n\n");
        settextcolor(11,0);
        printf("%s!\n", exception_messages[r->int_no]);
        printf("| <Exception - System Halted!> |");
        sleepSeconds(5);
        exit();
        for (;;);
    }

    if (pODA->ts_flag && (r->int_no==0x20 || r->int_no==0x7E)) // timer interrupt or function switch_context
        esp = task_switch (esp); //new task's esp

    interrupt_handler_t handler = irq_routines[r->int_no];
    if (handler) { handler(r); }

    if (r->int_no >= 40)
        outportb(0xA0, 0x20);
    outportb(0x20, 0x20);

    return esp;
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
