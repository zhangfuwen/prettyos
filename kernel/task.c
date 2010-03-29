/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "list.h"
#include "task.h"
#include "paging.h"
#include "kheap.h"

// Count of running tasks
int32_t userTaskCounter;

// The currently running and currently displayed task.
volatile task_t* current_task;
console_t* current_console;

// The start of the task linked list.
volatile task_t* ready_queue;

// Some externs are needed
extern tss_entry_t tss;
extern void irq_tail();
extern uint32_t read_eip();

uint32_t next_pid = 1; // The next available process ID.


int32_t getpid()
{
    return current_task->id;
}

void settaskflag(int32_t i)
{
    pODA->ts_flag = i;
}

int32_t getUserTaskNumber()
{
    return userTaskCounter;
}

void tasking_install()
{
    cli();

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("1st_task: ");
    settextcolor(15,0);
    #endif
    ///
    current_task = ready_queue = (task_t*)malloc(sizeof(task_t),0); // first task (kernel task)
    current_task->id = next_pid++;
    current_task->esp = current_task->ebp = 0;
    current_task->eip = 0;
    current_task->page_directory = kernel_pd;
    current_task->next = 0;
    current_task->console = malloc(sizeof(console_t), PAGESIZE); // Reserving space for the kernels console
    reachableConsoles[10] = current_task->console; // reachableConsoles[10] is reserved for kernels console
    current_console = current_task->console; // Kernels console is currently active (does not mean that it is visible)
    console_init(reachableConsoles[10], "");
    current_console->SCROLL_END = 39;
    refreshUserScreen();
    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("1st_ks: ");
    settextcolor(15,0);
    #endif
    ///

    current_task->kernel_stack = (uint32_t)malloc(KERNEL_STACK_SIZE,PAGESIZE)+KERNEL_STACK_SIZE;
    sti();

    userTaskCounter = 0;
}

task_t* create_task(page_directory_t* directory, void* entry, uint8_t privilege, const char* programName)
{
    cli();

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("cr_task: ");
    settextcolor(15,0);
    #endif
    ///

    task_t* new_task = (task_t*)malloc(sizeof(task_t),0);
    new_task->id  = next_pid++;
    new_task->page_directory = directory;
    new_task->heap_top = USER_HEAP_START;

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("cr_task_ks: ");
    settextcolor(15,0);
    #endif
    ///

    new_task->kernel_stack = (uint32_t) malloc(KERNEL_STACK_SIZE,PAGESIZE)+KERNEL_STACK_SIZE;
    new_task->next = 0;

    if (strcmp(programName, "Shell") == 0)
    {
        new_task->console = reachableConsoles[10]; // The Shell uses the same console as the kernel
    }
    else
    {
        new_task->console = malloc(sizeof(console_t), PAGESIZE);
        console_init(new_task->console, programName);
        for (uint8_t i = 0; i < 10; i++) { // The next free place in our console-list will be filled with the new console
            if (reachableConsoles[i] == 0) {
                reachableConsoles[i] = new_task->console;
                changeDisplayedConsole(i); //Switching to the new console
                break;
            }
        }
    }

    task_t* tmp_task = (task_t*)ready_queue;
    while (tmp_task->next)
    {
        tmp_task = tmp_task->next;
    }
    tmp_task->next = new_task;

    uint32_t* kernel_stack = (uint32_t*) new_task->kernel_stack;

    uint32_t code_segment=0x08, data_segment=0x10;


///TEST///
    *(--kernel_stack) = 0x0;  // return address dummy
///TEST///

    if (privilege == 3)
    {
        // general information: Intel 3A Chapter 5.12
        *(--kernel_stack) = new_task->ss = 0x23;    // ss
        *(--kernel_stack) = new_task->kernel_stack; // esp
        code_segment = 0x1B; // 0x18|0x3=0x1B
    }

    *(--kernel_stack) = 0x0202; // eflags = interrupts activated and iopl = 0
    *(--kernel_stack) = code_segment; // cs
    *(--kernel_stack) = (uint32_t)entry; // eip
    *(--kernel_stack) = 0; // error code

    *(--kernel_stack) = 0; // interrupt nummer

    // general purpose registers w/o esp
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;

    if (privilege == 3) data_segment = 0x23; // 0x20|0x3=0x23

    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;



    //setup TSS
    tss.ss0   = 0x10;
    tss.esp0  = new_task->kernel_stack;
    tss.ss    = data_segment;


    //setup task_t
    new_task->ebp = 0xd00fc0de; // test value
    new_task->esp = (uint32_t)kernel_stack;
    new_task->eip = (uint32_t)irq_tail;
    new_task->ss  = data_segment;

    sti();
    return new_task;
}

uint32_t task_switch (uint32_t esp)
{
    if (!current_task) return esp;
    current_task->esp = esp;   // save esp

    // Dispatcher
    // task switch
    current_task = current_task->next; // take the next task
    if (!current_task)
    {
        current_task = ready_queue;    // start at the beginning of the queue
    }

    // new_task
    paging_switch (current_task->page_directory);
    //tss.cr3 = ... TODO: Really unnecessary?
    tss.esp  = current_task->esp;
    tss.esp0 = (current_task->kernel_stack)+KERNEL_STACK_SIZE;
    tss.ebp  = current_task->ebp;
    tss.ss   = current_task->ss;

    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("%d ",getpid());
    settextcolor(15,0);
    #endif

    current_console = current_task->console;
    return current_task->esp;  // return new task's esp
}

void switch_context() // switch to next task
{
    __asm__ volatile("int $0x7E");
}

void exit()
{
    cli();
    #ifdef _DIAGNOSIS_
    log_task_list();
    #endif

    // finish current task and free occupied heap
    void* pkernelstack = (void*) ((uint32_t) current_task->kernel_stack - KERNEL_STACK_SIZE);
    void* ptask        = (void*) current_task;

    // Cleanup, delete current tasks console from list of our reachable consoles, if it is in that list and free memory
    for (int i = 0; i < 10; i++)
    {
        if (current_task->console == reachableConsoles[i]) {
            if (i == displayedConsole) {
                changeDisplayedConsole(10);
            }
            reachableConsoles[i] = 0;
        }
    }
    console_exit(current_task->console);
    if (current_task->console != reachableConsoles[10]) {
        free(current_task->console);
    }


    // delete task in linked list
    task_t* tmp_task = (task_t*)ready_queue;
    do
    {
        if (tmp_task->next == current_task)
        {
            tmp_task->next = current_task->next;
        }
        if (tmp_task->next)
        {
            tmp_task = tmp_task->next;
        }
    }
    while (tmp_task->next);
    // printf("after delete task in linked list.\n");

    // free memory at heap
    free(pkernelstack);
    free(ptask);
    // printf("pid: %d t: %x ks: %x <---heap freed.\n",getpid(),ptask,pkernelstack);

    #ifdef _DIAGNOSIS_
    log_task_list();
    printf("exit finished.\n");
    #endif

    userTaskCounter--; // a user-program has been deleted
    sti();
    switch_context(); // switch to next task
}


void* task_grow_userheap(uint32_t increase)
{
    increase = alignUp(increase, PAGESIZE);

    if (current_task->heap_top + increase > USER_HEAP_END)
        return false;

    if (! paging_alloc(current_task->page_directory, current_task->heap_top, increase, MEM_USER | MEM_WRITE))
        return NULL;

    void* before = current_task->heap_top;
    current_task->heap_top += increase;
    return before;
}


void log_task_list()
{
    task_t* tmp_task = (task_t*)ready_queue;
    do
    {
        task_log(tmp_task);
        tmp_task = tmp_task->next;
    }
    while (tmp_task->next);
    task_log(tmp_task);
}

void task_log(task_t* t)
{
    settextcolor(5,0);
    printf("\nid: %d ", t->id);               // Process ID
    printf("ebp: %x ",t->ebp);              // Base pointer
    printf("esp: %x ",t->esp);              // Stack pointer
    printf("eip: %x ",t->eip);              // Instruction pointer
    printf("PD: %x ", t->page_directory);   // Page directory.
    printf("k_stack: %x ",t->kernel_stack); // Kernel stack location.
    printf("next: %x\n",  t->next);         // The next task in a linked list.
    settextcolor(15,0);
}

void TSS_log(tss_entry_t* tssEntry)
{
    settextcolor(6,0);
    //printf("\nprev_tss: %x ", tssEntry->prev_tss);
    printf("esp0: %x ", tssEntry->esp0);
    printf("ss0: %x ", tssEntry->ss0);
    //printf("esp1: %x ", tssEntry->esp1);
    //printf("ss1: %x ", tssEntry->ss1);
    //printf("esp2: %x ", tssEntry->esp2);
    //printf("ss2: %x ", tssEntry->ss2);
    printf("cr3: %x ", tssEntry->cr3);
    printf("eip: %x ", tssEntry->eip);
    printf("eflags: %x ", tssEntry->eflags);
    printf("eax: %x ", tssEntry->eax);
    printf("ecx: %x ", tssEntry->ecx);
    printf("edx: %x ", tssEntry->edx);
    printf("ebx: %x ", tssEntry->ebx);
    printf("esp: %x ", tssEntry->esp);
    printf("ebp: %x ", tssEntry->ebp);
    printf("esi: %x ", tssEntry->esi);
    printf("edi: %x ", tssEntry->edi);
    printf("es: %x ", tssEntry->es);
    printf("cs: %x ", tssEntry->cs);
    printf("ss: %x ", tssEntry->ss);
    printf("ds: %x ", tssEntry->ds);
    printf("fs: %x ", tssEntry->fs);
    printf("gs: %x ", tssEntry->gs);
    //printf("ldt: %x ", tssEntry->ldt);
    //printf("trap: %x ", tssEntry->trap); //only 0 or 1
    //printf("iomap_base: %x ", tssEntry->iomap_base);
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
