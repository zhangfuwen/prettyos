/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "task.h"
#include "util.h"
#include "paging.h"
#include "descriptor_tables.h"
#include "kheap.h"
#include "video/video.h"
#include "scheduler.h"

bool task_switching;

task_t* currentTask;

// The console of the active task
console_t* currentConsole;

// Some externs are needed
extern tss_entry_t tss;
extern void irq_tail();

static uint32_t next_pid = 0; // The next available process ID.


void set_fpu_cw(const uint16_t ctrlword); // fpu.c

int32_t getpid()
{
    return(currentTask->pid);
}

void waitForTask(task_t* blockingTask)
{
    scheduler_blockCurrentTask(&BL_TASK, blockingTask);
}

void tasking_install()
{
    #ifdef _TASKING_DIAGNOSIS_
      textColor(0x03);
      printf("Install tasking\n");
    #endif

    cli();
    scheduler_install();
    currentTask                 = malloc(sizeof(task_t), 0, "task-currtask");
    currentTask->pid            = next_pid++;
    currentTask->esp            = 0;
    currentTask->eip            = 0;
    currentTask->page_directory = kernel_pd;
    currentTask->privilege      = 0;
    currentTask->FPU_ptr        = 0;
    currentTask->console        = currentConsole;
    currentTask->ownConsole     = true;
    currentTask->attrib         = 0x0F;
    currentTask->blocker.type   = 0;

    currentTask->kernel_stack = malloc(KERNEL_STACK_SIZE,4, "task-currtask-kst")+KERNEL_STACK_SIZE;

    scheduler_insertTask(currentTask);

    sti();
    task_switching = true;
}

static void addConsole(task_t* task, const char* consoleName)
{
    task->ownConsole = true;
    task->console = malloc(sizeof(console_t), 0, "task-taskconsole");
    console_init(task->console, consoleName);
    for (uint8_t i = 0; i < 10; i++)
    { // The next free place in our console-list will be filled with the new console
        if (reachableConsoles[i] == 0)
        {
            reachableConsoles[i] = task->console;
            changeDisplayedConsole(i); //Switching to the new console
            break;
        }
    }
}

static void createThreadTaskBase(task_t* new_task, page_directory_t* directory, void* entry, uint8_t privilege)
{
    new_task->pid            = next_pid++;
    new_task->page_directory = directory;
    new_task->privilege      = privilege;
    new_task->FPU_ptr        = 0;
    new_task->attrib         = 0x0F;
    new_task->blocker.type   = 0;

    if (new_task->privilege == 3)
    {
        new_task->heap_top = USER_HEAP_START;

        paging_alloc(new_task->page_directory, (void*)(USER_STACK-10*PAGESIZE), 10*PAGESIZE, MEM_USER|MEM_WRITE);
    }

    new_task->kernel_stack = malloc(KERNEL_STACK_SIZE,4, "task-kernelstack")+KERNEL_STACK_SIZE;


    uint32_t* kernel_stack = (uint32_t*)new_task->kernel_stack;
    uint32_t code_segment = 0x08, data_segment = 0x10;

    if(new_task->thread)
        *(--kernel_stack) = (uintptr_t)&exit;

    if (new_task->privilege == 3)
    {
        // general information: Intel 3A Chapter 5.12
        *(--kernel_stack) = new_task->ss = 0x23; // ss
        *(--kernel_stack) = USER_STACK;          // esp
        code_segment = 0x1B; // 0x18|0x3=0x1B
    }

    *(--kernel_stack) = 0x0202;          // eflags = interrupts activated and iopl = 0
    *(--kernel_stack) = code_segment;    // cs
    *(--kernel_stack) = (uint32_t)entry; // eip
    *(--kernel_stack) = 0;               // error code

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

    //setup task_t
    new_task->esp = (uint32_t)kernel_stack;
    new_task->eip = (uint32_t)irq_tail;
    new_task->ss  = data_segment;


    scheduler_insertTask(new_task); // new _task is inserted as last task in queue

    #ifdef _TASKING_DIAGNOSIS_
    task_log(new_task);
    #endif
}

task_t* create_ctask(page_directory_t* directory, void* entry, uint8_t privilege, const char* consoleName)
{
    task_t* new_task = create_task(directory, entry, privilege);
    addConsole(new_task, consoleName);
    return(new_task);
}

task_t* create_task(page_directory_t* directory, void* entry, uint8_t privilege)
{
    #ifdef _TASKING_DIAGNOSIS_
      textColor(0x03);
      printf("create task");
    #endif

    cli();
    task_t* new_task = malloc(sizeof(task_t),0, "task-newtask");
    new_task->thread = false;

    createThreadTaskBase(new_task, directory, entry, privilege);

    new_task->ownConsole = false;
    new_task->console = reachableConsoles[KERNELCONSOLE_ID]; // task uses the same console as the kernel

    sti();
    return new_task;
}

task_t* create_cthread(void(*entry)(), const char* consoleName)
{
    task_t* new_task = create_thread(entry);
    addConsole(new_task, consoleName);
    return(new_task);
}

task_t* create_thread(void(*entry)())
{
    #ifdef _TASKING_DIAGNOSIS_
      textColor(0x03);
      printf("create thread");
    #endif

    cli();
    task_t* new_task = malloc(sizeof(task_t),0, "task-newthread");
    new_task->thread = true;

    createThreadTaskBase(new_task, currentTask->page_directory, entry, currentTask->privilege);

    new_task->ownConsole = false;
    new_task->console = reachableConsoles[KERNELCONSOLE_ID]; // task uses the same console as the kernel

    sti();
    return new_task;
}

uint32_t task_switch(uint32_t esp)
{
    if (!currentTask) return esp;
    currentTask->esp = esp;   // save esp
    
    task_t* oldTask = currentTask; // Save old task to check if its the same than the new one
    currentTask = scheduler_getNextTask();

    if(oldTask == currentTask) return esp; // No task switch because old==new

    currentConsole = currentTask->console;

    // new_task
    paging_switch (currentTask->page_directory);

    tss.esp  = currentTask->esp;
    tss.esp0 = (uintptr_t)currentTask->kernel_stack;
    tss.ss   = currentTask->ss;

    #ifdef _TASKING_DIAGNOSIS_
      textColor(0x03);
      printf("%u ", currentTask->pid);
    #endif

    // set TS
    if (currentTask == FPUTask)
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
    return currentTask->esp;  // return new task's esp
}

void switch_context() // switch to next task
{
    __asm__ volatile("int $0x7E");
}

void exit()
{
    cli();

    #ifdef _TASKING_DIAGNOSIS_
      scheduler_log();
    #endif

    // finish current task and free occupied heap
    void* pkernelstack = (void*)((uintptr_t)currentTask->kernel_stack - KERNEL_STACK_SIZE);
    task_t* ptask      = currentTask;

    // Cleanup, delete current tasks console from list of our reachable consoles, if it is in that list and free memory
    if(currentTask->ownConsole) {
        for (uint8_t i = 0; i < 10; i++)
        {
            if (currentTask->console == reachableConsoles[i])
            {
                if (i == displayedConsole)
                {
                    changeDisplayedConsole(10);
                }
                reachableConsoles[i] = 0;
                break;
            }
        }
        console_exit(currentTask->console);
        free(currentTask->console);
    }


    // free memory at heap
    free(pkernelstack);
    free(ptask);

    scheduler_deleteTask(currentTask);

    sti();

    #ifdef _TASKING_DIAGNOSIS_
      textColor(0x03);
      printf("exit finished.\n");
      scheduler_log();
    #endif

    switch_context(); // switch to next task
}

void* task_grow_userheap(uint32_t increase)
{
    uint8_t* old_heap_top = currentTask->heap_top;
    increase = alignUp(increase, PAGESIZE);

    if (((uintptr_t)old_heap_top + increase > (uintptr_t)USER_HEAP_END) ||
        !paging_alloc(currentTask->page_directory, old_heap_top, increase, MEM_USER | MEM_WRITE))
    {
        return NULL;
    }

    currentTask->heap_top += increase;
    return old_heap_top;
}

void task_log(task_t* t)
{
    textColor(0x05);
    printf("\npid: %d  ", t->pid);            // Process ID
    printf("esp: %X  ", t->esp);            // Stack pointer
    printf("eip: %X  ", t->eip);            // Instruction pointer
    printf("PD: %X  ", t->page_directory);  // Page directory
    printf("k_stack: %X", t->kernel_stack); // Kernel stack location
    textColor(0x0F);
}

void TSS_log(tss_entry_t* tssEntry)
{
    textColor(0x06);
    printf("esp0: %X ", tssEntry->esp0);
    printf("ss0: %X ", tssEntry->ss0);
    printf("cr3: %X ", tssEntry->cr3);
    printf("eip: %X ", tssEntry->eip);
    printf("eflags: %X ", tssEntry->eflags);
    printf("eax: %X ", tssEntry->eax);
    printf("ecx: %X ", tssEntry->ecx);
    printf("edx: %X ", tssEntry->edx);
    printf("ebx: %X ", tssEntry->ebx);
    printf("esp: %X ", tssEntry->esp);
    printf("esi: %X ", tssEntry->esi);
    printf("edi: %X ", tssEntry->edi);
    printf("es: %X ", tssEntry->es);
    printf("cs: %X ", tssEntry->cs);
    printf("ss: %X ", tssEntry->ss);
    printf("ds: %X ", tssEntry->ds);
    printf("fs: %X ", tssEntry->fs);
    printf("gs: %X\n", tssEntry->gs);
    textColor(0x0F);
}


// --------------------- VM86 -------------------------------------------------------------------------------

static void create_vm86_ThreadTaskBase(task_t* new_task, void* entry)
{
    new_task->pid            = next_pid++;
    new_task->privilege      = 3;
    new_task->FPU_ptr        = 0;
    new_task->attrib         = 0x0F;
    new_task->blocker.type   = 0;

    new_task->heap_top = USER_HEAP_START;
    new_task->kernel_stack = malloc(KERNEL_STACK_SIZE,4, "task-kernelstack")+KERNEL_STACK_SIZE;
    uint32_t* kernel_stack = (uint32_t*)new_task->kernel_stack;

    if(new_task->thread)
    {
        *(--kernel_stack) = (uintptr_t)&exit;
    }

    *(--kernel_stack) = 0x23; // gs
    *(--kernel_stack) = 0x23; // fs
    *(--kernel_stack) = 0x23; // es 
    *(--kernel_stack) = 0x23; // ds 
    *(--kernel_stack) = new_task->ss = 0x0000; // ss
    *(--kernel_stack) = 4090;                  // USER_STACK; 
    *(--kernel_stack) = 0x20202;               // eflags = vm86 (bit17), interrupts (bit9), iopl=0
    *(--kernel_stack) = 0x00;                  // cs  
    *(--kernel_stack) = (uint32_t)entry;       // eip 

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
    
    uint32_t data_segment = 0x23; // 0x20|0x3=0x23

    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;

    //setup task_t
    new_task->esp = (uint32_t)kernel_stack;
    new_task->eip = (uint32_t)irq_tail;
    new_task->ss  = data_segment;

    scheduler_insertTask(new_task); // new _task is inserted as last task in queue

    #ifdef _TASKING_DIAGNOSIS_
    task_log(new_task);
    #endif
}

task_t* create_vm86_task(void* entry)
{
    #ifdef _TASKING_DIAGNOSIS_
      textColor(0x03);
      printf("create task");
    #endif

    cli();
    task_t* new_task = malloc(sizeof(task_t),0, "vm86-task-newtask");
    new_task->thread = false;

    create_vm86_ThreadTaskBase(new_task, entry);

    new_task->ownConsole = false;
    new_task->console = reachableConsoles[KERNELCONSOLE_ID]; // task uses the same console as the kernel

    sti();
    return new_task;
}

task_t* create_vm86_ctask(void* entry, const char* consoleName)
{
    task_t* new_task = create_vm86_task(entry);
    addConsole(new_task, consoleName);
    return(new_task);
}


// --------------------- VM86 -------------------------------------------------------------------------------


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
