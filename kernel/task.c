/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "task.h"
#include "scheduler.h"
#include "paging.h"
#include "descriptor_tables.h"
#include "kheap.h"

// Count of running user tasks
int32_t userTaskCounter;

// The currently running and currently displayed task.
task_t* current_task; /// TODO: clarify, whether volatile is necessary
console_t* current_console;

// Some externs are needed
extern tss_entry_t tss;
extern void irq_tail();

uint32_t next_pid = 0; // The next available process ID.

int32_t getpid()
{
    return current_task->pid;
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

    current_task = initTaskQueue();
    current_task->pid = next_pid++;
    current_task->esp = 0;
    current_task->eip = 0;
    current_task->page_directory = kernel_pd;
    current_task->privilege = 0;
    pODA->curTask = current_task;
    current_task->FPU_ptr = (uintptr_t)NULL;
    setNextTask(current_task, NULL); // last task in queue
    current_task->console = current_console;
    current_task->ownConsole = true;
    refreshUserScreen();

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("1st_ks: ");
    settextcolor(15,0);
    #endif
    ///

    current_task->kernel_stack = malloc(KERNEL_STACK_SIZE,PAGESIZE)+KERNEL_STACK_SIZE;
    userTaskCounter = 0;
    sti();
}

task_t* create_ctask(page_directory_t* directory, void* entry, uint8_t privilege, const char* programName)
{
    task_t* new_task = create_task(directory, entry, privilege);
    new_task->ownConsole = true;
    new_task->console = malloc(sizeof(console_t), PAGESIZE);
    console_init(new_task->console, programName);
    for (uint8_t i = 0; i < 10; i++)
    { // The next free place in our console-list will be filled with the new console
        if (reachableConsoles[i] == 0)
        {
            reachableConsoles[i] = new_task->console;
            changeDisplayedConsole(i); //Switching to the new console
            break;
        }
    }
    return(new_task);
}

task_t* create_task(page_directory_t* directory, void* entry, uint8_t privilege)
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
    new_task->pid  = next_pid++;
    new_task->page_directory = directory;
    new_task->privilege = privilege;
    new_task->threadFlag = false;

    if (privilege == 3)
    {
        new_task->heap_top = (uint8_t*)USER_HEAP_START;

        #ifdef _DIAGNOSIS_
        printf("task: %X. Alloc user-stack: \n",new_task);
        #endif

        paging_alloc(new_task->page_directory, (void*)(USER_STACK-10*PAGESIZE), 10*PAGESIZE, MEM_USER|MEM_WRITE);
    }

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("cr_task_ks: ");
    settextcolor(15,0);
    #endif
    ///

    new_task->kernel_stack = (void*)((uintptr_t)malloc(KERNEL_STACK_SIZE,PAGESIZE)+KERNEL_STACK_SIZE);
    pODA->curTask = new_task;
    new_task->FPU_ptr = (uintptr_t)NULL;
    setNextTask(new_task, NULL); // last task in queue

    new_task->ownConsole = false;
    new_task->console = reachableConsoles[10]; // task uses the same console as the kernel

    setNextTask(getLastTask(), new_task); // new _task is inserted as last task in queue

    uint32_t* kernel_stack = (uint32_t*) new_task->kernel_stack;
    uint32_t code_segment=0x08, data_segment=0x10;

    if (privilege == 3)
    {
        // general information: Intel 3A Chapter 5.12
        *(--kernel_stack) = new_task->ss = 0x23;    // ss
        *(--kernel_stack) = USER_STACK; // esp
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
    tss.esp   = current_task->esp = USER_STACK;
    tss.esp0  = (uintptr_t)new_task->kernel_stack;
    tss.ss    = data_segment;

    //setup task_t
    new_task->esp = (uint32_t)kernel_stack;
    new_task->eip = (uint32_t)irq_tail;
    new_task->ss  = data_segment;

    #ifdef _DIAGNOSIS_
    task_log(new_task);
    #endif

    sti();
    return new_task;
}

task_t* create_cthread(void* entry, const char* consoleName)
{
    task_t* new_task = create_thread(entry);
    new_task->ownConsole = true;
    new_task->console = malloc(sizeof(console_t), PAGESIZE);
    console_init(new_task->console, consoleName);
    for (uint8_t i = 0; i < 10; i++)
    { // The next free place in our console-list will be filled with the new console
        if (reachableConsoles[i] == 0)
        {
            reachableConsoles[i] = new_task->console;
            changeDisplayedConsole(i); //Switching to the new console
            break;
        }
    }
    return(new_task);
}

task_t* create_thread(void* entry)
{
    cli();

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("cr_thread: ");
    settextcolor(15,0);
    #endif
    ///

    task_t* new_task = malloc(sizeof(task_t),0);
    new_task->pid  = current_task->pid;
    new_task->page_directory = current_task->page_directory;
    new_task->privilege = current_task->privilege;
    // new_task->privilege = 0; /// TEST
    new_task->threadFlag = true;

    if (new_task->privilege == 3)
    {
        new_task->heap_top = (uint8_t*)USER_HEAP_START;

        #ifdef _DIAGNOSIS_
        printf("task: %X. Alloc user-stack: \n",new_task);
        #endif

        paging_alloc(new_task->page_directory, (void*)(USER_STACK-10*PAGESIZE), 10*PAGESIZE, MEM_USER|MEM_WRITE);
    }

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("cr_thread_ks: ");
    settextcolor(15,0);
    #endif
    ///

    new_task->kernel_stack = malloc(KERNEL_STACK_SIZE,PAGESIZE)+KERNEL_STACK_SIZE;
    pODA->curTask = new_task;
    new_task->FPU_ptr = (uintptr_t)NULL;
    setNextTask(new_task, NULL); // last task in queue

    new_task->ownConsole = false;
    new_task->console = current_task->console; // The thread uses the same console as the parent Task

    setNextTask(getLastTask(), new_task); // new _task is inserted as last task in queue

    uint32_t* kernel_stack = (uint32_t*) new_task->kernel_stack;
    uint32_t code_segment=0x08, data_segment=0x10;

    /// TEST
    *(--kernel_stack) = (uintptr_t)&exit;
    /// TEST

    if (new_task->privilege == 3)
    {
        // general information: Intel 3A Chapter 5.12
        *(--kernel_stack) = new_task->ss = 0x23;    // ss
        *(--kernel_stack) = USER_STACK; // esp
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

    if (new_task->privilege == 3) data_segment = 0x23; // 0x20|0x3=0x23

    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;

    //setup TSS
    tss.ss0   = 0x10;
    tss.esp0  = (uintptr_t)new_task->kernel_stack;
    tss.ss    = data_segment;

    //setup task_t
    new_task->esp = (uint32_t)kernel_stack;
    new_task->eip = (uint32_t)irq_tail;
    new_task->ss  = data_segment;

    #ifdef _DIAGNOSIS_
    task_log(new_task);
    #endif

    sti();
    return new_task;
}

uint32_t task_switch (uint32_t esp)
{
    if (!current_task) return esp;
    current_task->esp = esp;   // save esp

    // Dispatcher - task switch
    current_task = current_task->next; // take the next task
    if (!current_task)
    {
        current_task = getReadyTask();
    }

    // write active task struct address to ODA
    pODA->curTask = current_task;

    current_console = current_task->console;

    // new_task
    paging_switch (current_task->page_directory);
    //tss.cr3 = ... TODO: Really unnecessary?

    tss.esp  = current_task->esp;
    tss.esp0 = (uintptr_t)current_task->kernel_stack;
    tss.ss   = current_task->ss;

    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("%d ",getpid());
    settextcolor(15,0);
    #endif

    // set TS
    if (pODA->curTask == pODA->TaskFPU)
    {
        __asm__ ("CLTS"); // CLearTS: reset the TS bit (no. 3) in CR0 to disable #NM
    }
    else
    {
        uint32_t cr0;
        __asm__ volatile("mov %%cr0, %0": "=r"(cr0)); // read cr0
        cr0 |= 0x8; // set the TS bit (no. 3) in CR0 to enable #NM (exception no. 7)
        __asm__ volatile("mov %0, %%cr0":: "r"(cr0)); // write cr0
    }
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

    if ( (current_task->privilege == 3) && !(current_task->threadFlag) )
    {
        userTaskCounter--; // a user-program is going to stop
    }

    // Cleanup, delete current tasks console from list of our reachable consoles, if it is in that list and free memory
    if(current_task->ownConsole) {
        for (int i = 0; i < 10; i++)
        {
            if (current_task->console == reachableConsoles[i])
            {
                if (i == displayedConsole)
                {
                    changeDisplayedConsole(10);
                }
                reachableConsoles[i] = 0;
                break;
            }
        }
        console_exit(current_task->console);
        free(current_task->console);
    }

    clearTask((task_t*)current_task);

    // free memory at heap
    free(pkernelstack);
    free(ptask);

    #ifdef _DIAGNOSIS_
    log_task_list();
    printf("exit finished.\n");
    #endif

    sti();
    switch_context(); // switch to next task
}

void* task_grow_userheap(uint32_t increase)
{
    uint8_t* old_heap_top = current_task->heap_top;
    increase = alignUp(increase, PAGESIZE);

    if (((uintptr_t)old_heap_top + increase) > (uintptr_t)USER_HEAP_END)
        return NULL;

    if (! paging_alloc(current_task->page_directory, (void*)old_heap_top, increase, MEM_USER | MEM_WRITE))
        return NULL;

    current_task->heap_top += increase; // correct???
    return old_heap_top;
}

void task_log(task_t* t)
{
    settextcolor(5,0);
    printf("\npid: %d ", t->pid);             // Process ID
    printf("esp: %X ",t->esp);              // Stack pointer
    printf("eip: %X ",t->eip);              // Instruction pointer
    printf("PD: %X ", t->page_directory);   // Page directory.
    printf("k_stack: %X ",t->kernel_stack); // Kernel stack location.
    printf("next: %X\n",  t->next);         // The next task in a linked list.
    settextcolor(15,0);
}

void TSS_log(tss_entry_t* tssEntry)
{
    settextcolor(6,0);
    //printf("\nprev_tss: %x ", tssEntry->prev_tss);
    printf("esp0: %X ", tssEntry->esp0);
    printf("ss0: %X ", tssEntry->ss0);
    //printf("esp1: %X ", tssEntry->esp1);
    //printf("ss1: %X ", tssEntry->ss1);
    //printf("esp2: %X ", tssEntry->esp2);
    //printf("ss2: %X ", tssEntry->ss2);
    printf("cr3: %X ", tssEntry->cr3);
    printf("eip: %X ", tssEntry->eip);
    printf("eflags: %X ", tssEntry->eflags);
    printf("eax: %X ", tssEntry->eax);
    printf("ecx: %X ", tssEntry->ecx);
    printf("edx: %X ", tssEntry->edx);
    printf("ebx: %X ", tssEntry->ebx);
    printf("esp: %X ", tssEntry->esp);
    // printf("ebp: %X ", tssEntry->ebp);
    printf("esi: %X ", tssEntry->esi);
    printf("edi: %X ", tssEntry->edi);
    printf("es: %X ", tssEntry->es);
    printf("cs: %X ", tssEntry->cs);
    printf("ss: %X ", tssEntry->ss);
    printf("ds: %X ", tssEntry->ds);
    printf("fs: %X ", tssEntry->fs);
    printf("gs: %X ", tssEntry->gs);
    //printf("ldt: %X ", tssEntry->ldt);
    //printf("trap: %X ", tssEntry->trap); //only 0 or 1
    //printf("iomap_base: %X ", tssEntry->iomap_base);
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
