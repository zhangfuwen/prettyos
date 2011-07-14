/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "task.h"
#include "util.h"
#include "descriptor_tables.h"
#include "kheap.h"
#include "scheduler.h"
#include "timer.h"

// Kernel stack size
static const uint32_t kernelStackSize = 0x1000; // Tasks get a 4 KB kernel stack

// Some externs are needed
void fpu_setcw(uint16_t ctrlword); // fpu.c

extern void* globalUserProgAddr;
extern uint32_t globalUserProgSize;


bool task_switching = false; // We allow task switching when tasking and scheduler are installed.

task_t kernelTask = { // Needed to find out when the kernel task is exited
    .pid = 0, .esp = 0, .privilege = 0, .FPUptr = 0, .console = &kernelConsole, .attrib = 0x0F, .eventQueue = 0, .type = TASK,
    .blocker.type = 0, // The task is not blocked (scheduler.h/c)
    .kernelStack = 0, // The kerneltask does not need a kernel-stack because it does not call his own functions by syscall
    .threads = 0 // No threads associated with the task at the moment. List is created later if necessary
};

volatile task_t* currentTask = &kernelTask;
list_t* tasks; // List of all tasks. Not sorted by pid

static uint32_t next_pid = 1; // The next available process ID (kernel has 0, so we start with 1 here). TODO: Reuse old pid


void tasking_install()
{
    #ifdef _TASKING_DIAGNOSIS_
    textColor(TEXT);
    printf("Install tasking\n");
    textColor(TEXT);
    #endif

    tasks = list_Create();

    kernelTask.pageDirectory = kernelPageDirectory;
    kernelTask.eventQueue = event_createQueue();

    list_Append(kernelTask.console->tasks, &kernelTask);
    list_Append(tasks, &kernelTask);

    scheduler_install();
    scheduler_insertTask(&kernelTask);

    task_switching = true;
}

int32_t getpid()
{
    return(currentTask->pid);
}

bool waitForTask(task_t* blockingTask, uint32_t timeout)
{
    if(timeout > 0)
        return(scheduler_blockCurrentTask(BL_TASK, (void*)blockingTask->pid, max(1, timer_millisecondsToTicks(timeout))));
    else
    {
        scheduler_blockCurrentTask(BL_TASK, (void*)blockingTask->pid, 0);
        return(true);
    }
}


/// Functions to create tasks

static void createThreadTaskBase(task_t* newTask, pageDirectory_t* directory, void(*entry)(), uint8_t privilege, size_t argc, char* argv[])
{
    newTask->pid           = next_pid++;
    newTask->pageDirectory = directory;
    newTask->privilege     = privilege;
    newTask->FPUptr        = 0;
    newTask->attrib        = 0x0F;
    newTask->blocker.type  = 0;
    newTask->threads       = 0; // No threads associated with the task at the moment. created later if necessary
    newTask->userProgAddr  = 0;
    newTask->userProgSize  = 0;
    newTask->eventQueue    = 0; // Event handling is disabled per default

    if (newTask->privilege == 3)
    {
        newTask->heap_top = USER_heapStart;

        if(newTask->type != VM86)
        {
            newTask->userProgAddr = globalUserProgAddr;
            newTask->userProgSize = globalUserProgSize;

            paging_alloc(newTask->pageDirectory, (void*)(USER_STACK - 10*PAGESIZE), 10*PAGESIZE, MEM_USER|MEM_WRITE); // Stack starts at USER_STACK-StackSize*PAGESIZE
        }
    }

    newTask->kernelStack = malloc(kernelStackSize, 4, "task-kernelstack")+kernelStackSize;
    uint32_t* kernelStack = newTask->kernelStack;

    uint32_t code_segment = 0x08;

    if(newTask->type != VM86)
    {
        if(newTask->type == THREAD)
            *(--kernelStack) = (uintptr_t)&exit;

        if (newTask->privilege == 3)
        {
            // General information: Intel 3A Chapter 5.12
            *(--kernelStack) = 0x23;       // ss
            *(--kernelStack) = USER_STACK; // esp
            code_segment = 0x1B;           // 0x18|0x3=0x1B
        }

        *(--kernelStack) = 0x0202; // eflags: interrupts activated, iopl = 0
    }
    else
    {
        code_segment = 0;
        *(--kernelStack) = 0x0000;  // ss
        *(--kernelStack) = 4090;    // USER_STACK;
        *(--kernelStack) = 0x20202; // eflags = vm86 (bit17), interrupts (bit9), iopl=0
    }

    *(--kernelStack) = code_segment;    // cs
    *(--kernelStack) = (uint32_t)entry; // eip
    *(--kernelStack) = 0;               // error code

    *(--kernelStack) = 0; // Interrupt nummer

    // General purpose registers w/o esp
    *(--kernelStack) = argc;            // eax. Used to give argc to user programs.
    *(--kernelStack) = (uintptr_t)argv; // ecx. Used to give argv to user programs.
    *(--kernelStack) = 0;
    *(--kernelStack) = 0;
    *(--kernelStack) = 0;
    *(--kernelStack) = 0;
    *(--kernelStack) = 0;

    uint32_t data_segment = 0x10;
    if (newTask->privilege == 3) data_segment = 0x23; // 0x20|0x3=0x23

    *(--kernelStack) = data_segment;
    *(--kernelStack) = data_segment;
    *(--kernelStack) = data_segment;
    *(--kernelStack) = data_segment;

    newTask->esp = (uint32_t)kernelStack;
    newTask->ss  = data_segment;

    list_Append(tasks, newTask);
    scheduler_insertTask(newTask); // newTask is inserted as last task in queue

    #ifdef _TASKING_DIAGNOSIS_
    task_log(newTask);
    #endif
}

task_t* create_ctask(pageDirectory_t* directory, void(*entry)(), uint8_t privilege, size_t argc, char* argv[], const char* consoleName)
{
    task_t* newTask = create_task(directory, entry, privilege, argc, argv);
    list_Delete(newTask->console->tasks, newTask);
    newTask->console = malloc(sizeof(console_t), 0, "task-console");
    console_init(newTask->console, consoleName);
    list_Append(newTask->console->tasks, newTask);
    return(newTask);
}

task_t* create_task(pageDirectory_t* directory, void(*entry)(), uint8_t privilege, size_t argc, char* argv[])
{
    #ifdef _TASKING_DIAGNOSIS_
    textColor(TEXT);
    printf("create task");
    textColor(TEXT);
    #endif

    task_t* newTask = malloc(sizeof(task_t),0, "task-newtask");
    newTask->type = TASK;

    createThreadTaskBase(newTask, directory, entry, privilege, argc, argv);

    newTask->console = currentTask->console; // task shares the console of the current task
    list_Append(newTask->console->tasks, newTask);
    newTask->eventQueue = event_createQueue(); // For tasks event handling is enabled per default

    return newTask;
}

task_t* create_cthread(void(*entry)(), const char* consoleName)
{
    task_t* newTask = create_thread(entry);
    list_Delete(newTask->console->tasks, newTask);
    newTask->console = malloc(sizeof(console_t), 0, "thread-console");
    console_init(newTask->console, consoleName);
    list_Append(newTask->console->tasks, newTask);
    newTask->eventQueue = event_createQueue(); // Every thread with an own console gets an own eventQueue, because otherwise lots of input will never arrive.
    return(newTask);
}

task_t* create_thread(void(*entry)())
{
    #ifdef _TASKING_DIAGNOSIS_
    textColor(TEXT);
    printf("create thread");
    textColor(TEXT);
    #endif

    task_t* newTask = malloc(sizeof(task_t),0, "task-newthread");
    newTask->type = THREAD;

    createThreadTaskBase(newTask, currentTask->pageDirectory, entry, currentTask->privilege, 0, 0);

    // Attach the thread to its parent
    newTask->parent = (task_t*)currentTask;
    if(currentTask->threads == 0)
        currentTask->threads = list_Create();
    list_Append(currentTask->threads, newTask);

    newTask->console = currentTask->console; // task shares the console of the current task
    list_Append(newTask->console->tasks, newTask);

    return newTask;
}

task_t* create_vm86_task(pageDirectory_t* pd, void(*entry)())
{
    #ifdef _TASKING_DIAGNOSIS_
    textColor(TEXT);
    printf("create task");
    textColor(TEXT);
    #endif

    task_t* newTask = malloc(sizeof(task_t),0, "vm86-task");
    newTask->type = VM86;

    createThreadTaskBase(newTask, pd, entry, 3, 0, 0);

    newTask->console = reachableConsoles[KERNELCONSOLE_ID]; // Task uses the same console as the kernel
    list_Append(newTask->console->tasks, newTask);

    return newTask;
}


/// Functions to switch the task
void task_saveState(uint32_t esp)
{
    currentTask->esp = esp;
}

uint32_t task_switch(task_t* newTask)
{
    task_switching = false;

    if(newTask->pageDirectory != currentTask->pageDirectory) // Only switch page directory if the new task has a different one than the current task
        paging_switch(newTask->pageDirectory);

    currentTask = newTask;

    tss_switch((uintptr_t)currentTask->kernelStack, currentTask->esp, currentTask->ss); // esp0, esp, ss

    #ifdef _TASKING_DIAGNOSIS_
    textColor(TEXT);
    printf("%u ", currentTask->pid);
    textColor(TEXT);
    #endif

    // Set TS
    if (currentTask == FPUTask)
    {
        __asm__ volatile("CLTS"); // CLearTS: reset the TS bit (no. 3) in CR0 to disable #NM
    }
    else
    {
        uint32_t cr0;
        __asm__ volatile("mov %%cr0, %0": "=r"(cr0)); // Read cr0
        cr0 |= BIT(3); // Set the TS bit (no. 3) in CR0 to enable #NM (exception no. 7)
        __asm__ volatile("mov %0, %%cr0":: "r"(cr0)); // Write cr0
    }

    task_switching = true;
    return currentTask->esp; // Return new task's esp
}

void switch_context() // Switch to next task (by interrupt)
{
    if(!scheduler_shouldSwitchTask()) // If the scheduler does not want to switch the task ...
        hlt(); // Wait one cycle

    __asm__ volatile("int $0x7E"); // Call interrupt that will call task_switch.
}


// Functions to kill a task
static void kill(task_t* task)
{
    task_switching = false; // There should not occur a task switch while we are exiting from a task, to avoid data corruption

    #ifdef _TASKING_DIAGNOSIS_
    scheduler_log();
    #endif

    // Cleanup
    list_Delete(task->console->tasks, task);
    if(task->console->tasks->head == 0)
    {
        // Delete current task's console from list of our reachable consoles, if it is in that list
        for (uint8_t i = 1; i < 11; i++)
        {
            if (task->console == reachableConsoles[i])
            {
                reachableConsoles[i] = 0;
                break;
            }
        }
        // Switch back to kernel console if the tasks console is displayed at the moment
        if (task->console == console_displayed)
        {
            console_display(KERNELCONSOLE_ID);
        }
        // Free memory
        console_exit(task->console);
        free(task->console);
    }

    // Free user memory, if this task has an own PD
    if (task->type != THREAD && task->type != VM86 && task->pageDirectory != kernelPageDirectory)
    {
        paging_destroyUserPageDirectory(task->pageDirectory);
    }

    // Inform the parent task that this task has exited
    if (task->type == THREAD)
    {
        list_Delete(task->parent->threads, task);
    }

    // Kill all child-threads of this task
    if (task->threads)
    {
        for(element_t* e = task->threads->head; e != 0; e = e->next)
        {
            kill(e->data);
        }
        list_DeleteAll(task->threads);
    }

    list_Delete(tasks, task);
    scheduler_deleteTask(task);

    #ifdef _TASKING_DIAGNOSIS_
    textColor(TEXT);
    printf("exit finished.\n");
    scheduler_log();
    #endif

    if(task->eventQueue != 0)
        event_deleteQueue(task->eventQueue);

    if(task == &kernelTask) // TODO: Handle termination of shellTask
    {
        systemControl(REBOOT);
    }

    free(task->FPUptr);
    free(task->kernelStack - kernelStackSize); // Free kernelstack
    free(task);

    task_switching = true;

    if(task == currentTask) // Tasks adress is still saved, although its no longer valid so we can use it here
    {
        switch_context(); // Switch to next task
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
    kill((task_t*)currentTask);
}

void* task_grow_userheap(uint32_t increase)
{
    uint8_t* old_heap_top = currentTask->heap_top;
    increase = alignUp(increase, PAGESIZE);

    if (((uintptr_t)old_heap_top + increase > (uintptr_t)USER_heapEnd) ||
        !paging_alloc(currentTask->pageDirectory, old_heap_top, increase, MEM_USER | MEM_WRITE))
    {
        return 0;
    }

    currentTask->heap_top += increase;
    return old_heap_top;
}

void task_log(task_t* t)
{
    textColor(IMPORTANT);
    printf("\npid: %d\t", t->pid);          // Process ID
    textColor(TEXT);
    printf("esp: %Xh  ", t->esp);           // Stack pointer
    printf("PD: %Xh  ", t->pageDirectory);  // Page directory
    printf("k_stack: %Xh", t->kernelStack); // Kernel stack location
    if(t->type == THREAD)
        printf("  parent: %u", t->parent->pid);
    if(t->threads && t->threads->head)
    {
        printf("\n\t");
        printf("child-threads:");
        for(element_t* e = t->threads->head; e != 0; e = e->next)
        {
            printf(" %u ", ((task_t*)e->data)->pid);
        }
    }
}


/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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
