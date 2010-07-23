/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "task.h"
#include "scheduler.h"
#include "list.h"
#include "util.h"
#include "timer.h"
#include "synchronisation.h"

ring_t* task_queue;
ring_t* blockedTasks;

task_t* freetimeTask = 0;

blockerType_t BL_TIME, BL_SEMAPHORE, BL_INTERRUPT;

static void doNothing()
{
    switch_context();
    while(true)
    {
        hlt();
    }
}

void scheduler_install()
{
    task_queue = ring_Create();
    blockedTasks = ring_Create();

    BL_TIME.unlock      = &timer_unlockTask;
    BL_SEMAPHORE.unlock = &semaphore_unlockTask;
    BL_INTERRUPT.unlock = 0;
}

static void checkBlocked()
{
    if(blockedTasks->begin == 0) return;

    blockedTasks->current = blockedTasks->begin;
    do
    {
        if(((task_t*)blockedTasks->current->data)->blocker.type->unlock == 0 || ((task_t*)blockedTasks->current->data)->blocker.type->unlock(blockedTasks->current->data)) // Either no unblock-function specified (avoid blocks) or the task should not be blocked any more
        {
            scheduler_insertTask(blockedTasks->current->data);
            ring_DeleteFirst(blockedTasks, blockedTasks->current->data);
        }
        blockedTasks->current = blockedTasks->current->next;
    } while(blockedTasks->begin != 0 && blockedTasks->current != blockedTasks->begin);
}

task_t* scheduler_getNextTask()
{
    checkBlocked();

    if(task_queue->begin == 0)
    {
        if(freetimeTask == 0) // the task has not been needed until now. Use spare time to create it.
        {
            freetimeTask = create_thread(&doNothing);
            scheduler_deleteTask(freetimeTask);
        }
        return(freetimeTask);
    }

    task_queue->current = task_queue->current->next;
    return((task_t*)task_queue->current->data);
}

void scheduler_insertTask(task_t* task)
{
    ring_Insert(task_queue, task);
}

void scheduler_deleteTask(task_t* task)
{
    while(ring_DeleteFirst(task_queue, task));
}

void scheduler_blockCurrentTask(blockerType_t* reason, void* data)
{
    currentTask->blocker.type = reason;
    currentTask->blocker.data = data;

    ring_Insert(blockedTasks, currentTask);
    scheduler_deleteTask(currentTask);

    sti();
    switch_context();
}

void scheduler_log()
{
    textColor(0x0F);
    printf("\ncurrent task: ");
    textColor(0x05);
    printf("pid: %u", currentTask->pid);
    textColor(0x0F);
    printf("\nrunning tasks:");
    element_t* temp = task_queue->begin;
    do
    {
        if(temp == 0) break;

        task_log((task_t*)temp->data);
        temp = temp->next;
    }
    while (temp != task_queue->begin);

    printf("\nblocked tasks:");
    temp = blockedTasks->begin;
    do
    {
        if(temp == 0) break;

        task_log((task_t*)temp->data);
        temp = temp->next;
    }
    while (temp != blockedTasks->begin);

    printf("\n\n");
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
