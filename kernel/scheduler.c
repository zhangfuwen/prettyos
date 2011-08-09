/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "timer.h"
#include "task.h"
#include "todo_list.h"
#include "irq.h"
#include "ring.h"
#include "scheduler.h"

ring_t* runningTasks = 0; 
ring_t* blockedTasks = 0;

static     task_t* freetimeTask = 0;

blockerType_t blocker[] =
{
    {0},                    // BL_TIME
    {0},                    // BL_SYNC
    {&irq_unlockTask},      // BL_INTERRUPT. Interrupts seem to be good for event based handling, but they are not, because we count interrupts occuring before the block was set.
    {0},                    // BL_TASK
    {&todoList_unlockTask}, // BL_TODOLIST
    {&event_unlockTask},    // BL_EVENT
    {0}                     // BL_NETPACKET
};


// Function for freetime task. Executed when the ring of running tasks is empty.
static void doNothing()
{
    while(true)
    {
        hlt();
    }
}

void scheduler_install()
{
    // Init scheduler rings
    runningTasks = ring_create();
    blockedTasks = ring_create();
}

static void unblockTask(task_t* task, bool timeout)
{
    // Write the reason for the unblock in the data field of the blocker (false in case of timeout)
    task->blocker.data = (void*)(!timeout);

    // Move task into the scheduler ring
    ring_move(runningTasks, blockedTasks, task);
}

void scheduler_unblockEvent(BLOCKERTYPE type, void* data) // Event based blocks are handled here
{
    if(!blockedTasks || blockedTasks->begin == 0) return; // Ring is empty

    blockedTasks->current = blockedTasks->begin;
    do
    {
        task_t* current = (task_t*)blockedTasks->current->data;
        if(current->blocker.type == &blocker[type] && current->blocker.data == data) // The blocking event this ring element is waiting for appeared -> unblock
        {
            unblockTask(current, false);
        }
        blockedTasks->current = blockedTasks->current->next; // Iterate through the ring
    } while(blockedTasks->begin != 0 && blockedTasks->current != blockedTasks->begin);
}

static void checkBlocked() // Not event based blocks are handled here (polling)
{
    if(blockedTasks->begin == 0) return; // Ring is empty

    blockedTasks->current = blockedTasks->begin;
    do
    {
        task_t* current = (task_t*)blockedTasks->current->data;
        if(current->blocker.type && current->blocker.type->unlock && current->blocker.type->unlock(current->blocker.data)) // Unblock function specified and the task should not be blocked any more...
        {
            unblockTask(current, false);
        }
        else if(current->blocker.timeout != 0 && current->blocker.timeout <= timer_getTicks()) // ...or timeout reached
        {
            unblockTask(current, true);
        }
        blockedTasks->current = blockedTasks->current->next; // Iterate through the ring
    } while(blockedTasks->begin != 0 && blockedTasks->current != blockedTasks->begin);
}

bool scheduler_shouldSwitchTask() // This function increases performance if there is just one task running by avoiding task switches
{
    return(runningTasks->begin == 0 || runningTasks->begin != runningTasks->begin->next); // No running tasks or only one running task
}

static task_t* scheduler_getNextTask()
{
    checkBlocked();

    if(runningTasks->begin == 0) // Ring is empty. Freetime for the CPU.
    {
        if(freetimeTask == 0) // The freetime task has not been needed until now. Use spare time to create it.
        {
            freetimeTask = create_task(PROCESS, kernelPageDirectory, &doNothing, 0, &kernelConsole, 0, 0);
        }
        return(freetimeTask);
    }

    runningTasks->current = runningTasks->current->next; // Take next task from the ring
    return((task_t*)runningTasks->current->data);
}

uint32_t scheduler_taskSwitch(uint32_t esp)
{
    if(runningTasks == 0)
        return(esp); // Tasking seems to be not installed -> Don't switch task.

    task_saveState(esp);

    task_t* oldTask = (task_t*)currentTask;
    task_t* newTask = scheduler_getNextTask();
    if(oldTask == newTask) // No task switch needed
        return(esp);

    return(task_switch(newTask));
}

void scheduler_insertTask(task_t* task)
{
    ring_insert(runningTasks, task, true); // We only want to have a task one time in the ring, because we steer the priority (later) with multiple rings and not by inserting a task multiple times into the ring
}

void scheduler_deleteTask(task_t* task)
{
    // Take task out of our rings
    ring_deleteFirst(runningTasks, task);
    ring_deleteFirst(blockedTasks, task);

    scheduler_unblockEvent(BL_TASK, (void*)task->pid); // Unblock tasks waiting for the end of the given task
}

bool scheduler_blockCurrentTask(BLOCKERTYPE reason, void* data, uint32_t timeout)
{
    currentTask->blocker.type = &blocker[reason];
    currentTask->blocker.data = data;
    if(timeout == 0)
        currentTask->blocker.timeout = 0;
    else
        currentTask->blocker.timeout = timer_getTicks()+timeout;

    cli();
    ring_move(blockedTasks, runningTasks, (task_t*)currentTask);
    sti();

    switch_context(); // Leave task. This task will not be called again until block ended.

    return((bool)currentTask->blocker.data); // data field contains the reason for unblock after the block is released
}

void scheduler_log()
{
    textColor(HEADLINE);
    printf("\ncurrent task: ");
    textColor(TEXT);
    printf("pid: %u", currentTask->pid);

    if(runningTasks->begin != 0)
    {
        textColor(HEADLINE);
        printf("\nrunning tasks:");
        textColor(TEXT);
        slelement_t* temp = runningTasks->begin;
        do
        {
            task_log((task_t*)temp->data);
            temp = temp->next;
        }
        while (temp && temp != runningTasks->begin);
    }

    if(blockedTasks->begin != 0)
    {
        textColor(HEADLINE);
        printf("\nblocked tasks:");
        textColor(TEXT);
        slelement_t* temp = blockedTasks->begin;
        do
        {
            task_log((task_t*)temp->data);
            temp = temp->next;
        }
        while (temp && temp != blockedTasks->begin);
    }

    if(freetimeTask)
    {
        textColor(HEADLINE);
        printf("\nfreetime task:");
        textColor(TEXT);
        task_log(freetimeTask);
    }
    printf("\n\n");
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
