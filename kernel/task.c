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

bool task_switching = false;

task_t* kernelTask = 0; // Needed to find out when the kernel task is exited

task_t* currentTask = 0;
listHead_t* tasks; // List of all tasks. Not sorted by pid

static uint32_t next_pid = 0; // The next available process ID. TODO: Reuse old pid

// The console of the active task
console_t* currentConsole;

// Some externs are needed
extern tss_entry_t tss;
void irq_tail();
void fpu_setcw(uint16_t ctrlword); // fpu.c

void tasking_install()
{
    #ifdef _TASKING_DIAGNOSIS_
    textColor(0x03);
    printf("Install tasking\n");
    textColor(0x0F);
    #endif

    tasks = list_Create();

    scheduler_install();
    kernelTask                 = malloc(sizeof(task_t), 0, "task-kernelTask");
    kernelTask->pid            = next_pid++;
    kernelTask->esp            = 0;
    kernelTask->eip            = 0;
    kernelTask->page_directory = kernel_pd;
    kernelTask->privilege      = 0;
    kernelTask->FPU_ptr        = 0;
    kernelTask->console        = currentConsole;
    kernelTask->ownConsole     = true;
    kernelTask->attrib         = 0x0F;
    kernelTask->blocker.type   = 0; // The task is not blocked (scheduler.h/c)
    kernelTask->kernel_stack   = 0; // The kerneltask does not need a kernel-stack because it does not call his own functions by syscall

    list_Append(tasks, kernelTask);
    scheduler_insertTask(kernelTask);
    currentTask = kernelTask;

    task_switching = true;
}

int32_t getpid()
{
    return(currentTask->pid);
}

void waitForTask(task_t* blockingTask)
{
    scheduler_blockCurrentTask(&BL_TASK, blockingTask);
}


/// Functions to create tasks

static void addConsole(task_t* task, const char* consoleName)
{
    task->ownConsole = true;
    task->console = malloc(sizeof(console_t), 0, "task-console");
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

static void createThreadTaskBase(task_t* newTask, page_directory_t* directory, void(*entry)(), uint8_t privilege)
{
    newTask->pid            = next_pid++;
    newTask->page_directory = directory;
    newTask->privilege      = privilege;
    newTask->FPU_ptr        = 0;
    newTask->attrib         = 0x0F;
    newTask->blocker.type   = 0;
    newTask->entry          = entry;
    newTask->threads        = 0; // created later if necessary

    if (newTask->privilege == 3)
    {
        newTask->heap_top = USER_HEAP_START;

        if(newTask->type != VM86)
            paging_alloc(newTask->page_directory, (void*)(USER_STACK-10*PAGESIZE), 10*PAGESIZE, MEM_USER|MEM_WRITE);
    }

    newTask->kernel_stack = malloc(KERNEL_STACK_SIZE,4, "task-kernelstack")+KERNEL_STACK_SIZE;
    uint32_t* kernel_stack = newTask->kernel_stack;

    uint32_t code_segment = 0x08;

    if(newTask->type != VM86)
    {
        if(newTask->type == THREAD)
            *(--kernel_stack) = (uintptr_t)&exit;

        if (newTask->privilege == 3)
        {
            // general information: Intel 3A Chapter 5.12
            *(--kernel_stack) = newTask->ss = 0x23; // ss
            *(--kernel_stack) = USER_STACK;          // esp
            code_segment = 0x1B; // 0x18|0x3=0x1B
        }

        *(--kernel_stack) = 0x0202; // eflags = interrupts activated and iopl = 0
    }
    else
    {
        code_segment = 0;
        *(--kernel_stack) = 0x23; // gs
        *(--kernel_stack) = 0x23; // fs
        *(--kernel_stack) = 0x23; // es
        *(--kernel_stack) = 0x23; // ds
        *(--kernel_stack) = newTask->ss = 0x0000; // ss
        *(--kernel_stack) = 4090;                 // USER_STACK;
        *(--kernel_stack) = 0x20202;              // eflags = vm86 (bit17), interrupts (bit9), iopl=0
    }

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

    uint32_t data_segment = 0x10;
    if (newTask->privilege == 3) data_segment = 0x23; // 0x20|0x3=0x23

    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;

    //setup task_t
    newTask->esp = (uint32_t)kernel_stack;
    newTask->eip = (uint32_t)irq_tail;
    newTask->ss  = data_segment;

    list_Append(tasks, newTask);
    scheduler_insertTask(newTask); // newTask is inserted as last task in queue

    #ifdef _TASKING_DIAGNOSIS_
    task_log(newTask);
    #endif
}

task_t* create_ctask(page_directory_t* directory, void(*entry)(), uint8_t privilege, const char* consoleName)
{
    task_t* newTask = create_task(directory, entry, privilege);
    addConsole(newTask, consoleName);
    return(newTask);
}

task_t* create_task(page_directory_t* directory, void(*entry)(), uint8_t privilege)
{
    #ifdef _TASKING_DIAGNOSIS_
    textColor(0x03);
    printf("create task");
    textColor(0x0F);
    #endif

    task_t* newTask = malloc(sizeof(task_t),0, "task-newtask");
    newTask->type = TASK;

    createThreadTaskBase(newTask, directory, entry, privilege);

    newTask->ownConsole = false;
    newTask->console = reachableConsoles[KERNELCONSOLE_ID]; // task uses the same console as the kernel

    sti();
    return newTask;
}

task_t* create_cthread(void(*entry)(), const char* consoleName)
{
    task_t* newTask = create_thread(entry);
    addConsole(newTask, consoleName);
    return(newTask);
}

task_t* create_thread(void(*entry)())
{
    #ifdef _TASKING_DIAGNOSIS_
    textColor(0x03);
    printf("create thread");
    textColor(0x0F);
    #endif

    task_t* newTask = malloc(sizeof(task_t),0, "task-newthread");
    newTask->type = THREAD;

    createThreadTaskBase(newTask, currentTask->page_directory, entry, currentTask->privilege);

    // attach the thread with its parent
    newTask->parent = currentTask;
    if(currentTask->threads == 0)
        currentTask->threads = list_Create();
    list_Append(currentTask->threads, newTask);

    newTask->ownConsole = false;
    newTask->console = reachableConsoles[KERNELCONSOLE_ID]; // task uses the same console as the kernel

    return newTask;
}

task_t* create_vm86_task(void(*entry)())
{
    #ifdef _TASKING_DIAGNOSIS_
    textColor(0x03);
    printf("create task");
    textColor(0x0F);
    #endif

    task_t* newTask = malloc(sizeof(task_t),0, "vm86-task");
    newTask->type = VM86;

    createThreadTaskBase(newTask, kernel_pd, entry, 3);

    newTask->ownConsole = false;
    newTask->console = reachableConsoles[KERNELCONSOLE_ID]; // task uses the same console as the kernel

    return newTask;
}


/// Functions to switch the task

uint32_t task_switch(uint32_t esp)
{
    if (!currentTask) return esp;

    task_switching = false;

    task_t* oldTask = currentTask; // Save old task to check if its the same than the new one
    oldTask->esp = esp; // save esp

    currentTask = scheduler_getNextTask();

    if(oldTask == currentTask)
    {
        task_switching = true;
        return esp; // No task switch because old==new
    }

    currentConsole = currentTask->console;

    paging_switch(currentTask->page_directory);

    tss.esp  = currentTask->esp;
    tss.esp0 = (uintptr_t)currentTask->kernel_stack;
    tss.ss   = currentTask->ss;

    #ifdef _TASKING_DIAGNOSIS_
    textColor(0x03);
    printf("%u ", currentTask->pid);
    textColor(0x0F);
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

    task_switching = true;
    return currentTask->esp; // return new task's esp
}

void switch_context() // switch to next task (by interrupt)
{
    if(scheduler_shouldSwitchTask()) // only switch task if the scheduler wants to switch ...
        __asm__ volatile("int $0x7E");
    else // ... otherwise save CPU-time with hlt and switch then (to avoid problems if the next interrupt is not a task-switching interrupt)
    {
        hlt();
        __asm__ volatile("int $0x7E");
    }
}


/// Functions to kill a task

static void kill(task_t* task)
{
    if(task == currentTask)
        cli();

    #ifdef _TASKING_DIAGNOSIS_
    scheduler_log();
    #endif

    // Cleanup, delete current tasks console from list of our reachable consoles, if it is in that list and free memory
    if(task->ownConsole)
    {
        for (uint8_t i = 0; i < 10; i++)
        {
            if (task->console == reachableConsoles[i])
            {
                if (i == displayedConsole)
                {
                    changeDisplayedConsole(10);
                }
                reachableConsoles[i] = 0;
                break;
            }
        }
        console_exit(task->console);
        free(task->console);
    }

    // signalize the parent task that this task is exited
    if(task->type == THREAD)
        list_Delete(task->parent->threads, task);

    // kill all child-threads of this task
    if(task->threads)
    {
        for(element_t* e = task->threads->head; e != 0; e = e->next)
        {
           //kill(e->data);  /// HACK -> #PF on some systems
        }
        // list_DeleteAll(task->threads);
    }

    // list_Delete(tasks, task);
    scheduler_deleteTask(task);

    #ifdef _TASKING_DIAGNOSIS_
    textColor(0x03);
    printf("exit finished.\n");
    scheduler_log();
    #endif

    if(task == kernelTask) // TODO: Handle termination of shellTask
    {
        systemControl(REBOOT);
    }

    free(task->kernel_stack - KERNEL_STACK_SIZE); // free kernelstack
    free(task);

    if(task == currentTask) // tasks adress is still saved, although its no longer valid so we can use it here
    {
        sti();
        switch_context(); // switch to next task
    }
}

void task_kill(uint32_t pid)
{
    // Find out task by looking for the pid in the tasks-list
    for(element_t* e = tasks->head; e != 0; e = e->next)
    {
        if(((task_t*)e->data)->pid == pid)
        {
            kill(e->data);
            return;
        }
    }
}

void exit()
{
    kill(currentTask);
}

void task_restart(uint32_t pid)
{
    task_t* task = 0; // Find out task by looking for the pid in the tasks-list
    for(element_t* e = tasks->head; e != 0; e = e->next)
    {
        if(((task_t*)e->data)->pid == pid)
        {
            task = e->data;
            break;
        }
    }
    if(task == 0) return;

    // Safe old properties
    page_directory_t* directory  = task->page_directory;
    void            (*entry)()   = task->entry;
    uint8_t           privilege  = task->privilege;
    bool              ownConsole = task->ownConsole;

    kill(task); // Kill old task

    // create a new one reusing old properties
    if(ownConsole)
        create_ctask(directory, entry, privilege, "restarted task"); // TODO: Safe old name or maybe reuse old console
    else
        create_task(directory, entry, privilege);
}

void* task_grow_userheap(uint32_t increase)
{
    uint8_t* old_heap_top = currentTask->heap_top;
    increase = alignUp(increase, PAGESIZE);

    if (((uintptr_t)old_heap_top + increase > (uintptr_t)USER_HEAP_END) ||
        !paging_alloc(currentTask->page_directory, old_heap_top, increase, MEM_USER | MEM_WRITE))
    {
        return 0;
    }

    currentTask->heap_top += increase;
    return old_heap_top;
}

void task_log(task_t* t)
{
    textColor(0x05);
    printf("\npid: %d\t", t->pid);          // Process ID
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
