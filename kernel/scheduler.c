/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "task.h"
#include "kheap.h"

// The start of the task linked list.
volatile task_t* ready_queue; /// TODO: clarify, whether volatile is necessary

//void tasking_install()
task_t* initTaskQueue()
{
    ready_queue = (task_t*)malloc(sizeof(task_t),0); // first task (kernel task)
    return (task_t*)ready_queue;
}

task_t* getReadyTask()
{
    return (task_t*)ready_queue;
}

void setNextTask(task_t* task1, task_t* task2)
{
    task1->next = task2;
}

task_t* getLastTask()
{
    task_t* tmp_task = getReadyTask();
    while (tmp_task->next)
    {
        tmp_task = tmp_task->next;
    }
    return tmp_task;
}

// take task out of linked list
void clearTaskTask(task_t* task1)
{
    task_t* tmp_task = getReadyTask();
    do
    {
        if (tmp_task->next == task1)
        {
            tmp_task->next = task1->next;
        }
        if (tmp_task->next)
        {
            tmp_task = tmp_task->next;
        }
    }
    while (tmp_task->next);
}

void log_task_list()
{
    task_t* tmp_task = getReadyTask();
    do
    {
        task_log(tmp_task);
        tmp_task = tmp_task->next;
    }
    while (tmp_task->next);
    task_log(tmp_task);
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
