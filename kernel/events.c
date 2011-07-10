/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "events.h"
#include "task.h"
#include "kheap.h"
#include "util.h"


event_queue_t* event_createQueue()
{
    event_queue_t* queue = malloc(sizeof(event_queue_t), 0, "event_queue");
    queue->num = 0;
    queue->mutex = mutex_create();
    queue->list = list_Create();
    return(queue);
}

void event_deleteQueue(event_queue_t* queue)
{
    for(element_t* e = queue->list->head; e != 0; e = e->next)
        free(e->data);
    list_DeleteAll(queue->list);
    mutex_delete(queue->mutex);
    free(queue);
}

uint8_t event_issue(event_queue_t* destination, EVENT_t type, void* data, size_t length)
{
    int8_t retVal;
    if(!destination) // Event handling disabled
    {
        retVal = 1;
        return (retVal);
    }

    if(destination->num == MAX_EVENTS)
    {
        // Overflow
        event_t* ev = malloc(sizeof(event_t), 0, "event(overflow)");
        ev->data = 0;
        ev->length = 0;
        ev->type = EVENT_OVERFLOW;
        mutex_lock(destination->mutex);
        list_Append(destination->list, ev);
        destination->num++;
        mutex_unlock(destination->mutex);

        retVal = 2;
    }
    else if(destination->num > MAX_EVENTS)
    {
        // Nothing to do. OVERFLOW event has already been added.
        retVal = 3;
    }
    else
    {
        // Add event
        event_t* ev = malloc(sizeof(event_t), 0, "event");
        ev->data = malloc(length, 0, "event->data");
        memcpy(ev->data, data, length);
        ev->length = length;
        ev->type = type;
        mutex_lock(destination->mutex);
        list_Append(destination->list, ev);
        destination->num++;
        mutex_unlock(destination->mutex);

        retVal = 0;
    }
    scheduler_unblockEvent(BL_EVENT, (void*)type);
    return retVal;
}

EVENT_t event_poll(void* destination, size_t maxLength, EVENT_t filter)
{
    task_t* task = (task_t*)currentTask;
    while(task->parent && task->type == THREAD && task->eventQueue == 0)
        task = task->parent; // Use parents eventQueue, if the thread has no own queue.

    if(task->eventQueue == 0 || task->eventQueue->num == 0) // Event handling disabled or no events available
        return(EVENT_NONE);


    event_t* ev = 0;
    mutex_lock(task->eventQueue->mutex);
    if(filter == EVENT_NONE)
    {
        ev = task->eventQueue->list->head->data;
    }
    else
    {
        for(element_t* e = task->eventQueue->list->head; e != 0; e = e->next)
        {
            ev = e->data;
            if(ev->type == filter)
                break; // found event
            else
                ev = 0;
        }
    }

    if(ev == 0)
    {
        mutex_unlock(task->eventQueue->mutex);
        return(EVENT_NONE);
    }

    EVENT_t type = EVENT_NONE;
    if(ev->length > maxLength)
    {
        type = EVENT_INVALID_ARGUMENTS;
    }
    else
    {
        type = ev->type;
        memcpy(destination, ev->data, ev->length);
    }
    task->eventQueue->num--;
    list_Delete(task->eventQueue->list, ev);
    mutex_unlock(task->eventQueue->mutex);
    free(ev->data);
    free(ev);

    return(type);
}

event_t* event_peek(event_queue_t* eventQueue, uint32_t i)
{
    element_t* elem = list_GetElement(eventQueue->list, i);
    if(elem == 0) return(0);
    return(elem->data);
}

void event_enable(bool b)
{
    if(b)
    {
        if(currentTask->eventQueue == 0)
            currentTask->eventQueue = event_createQueue();
    }
    else
    {
        if(currentTask->eventQueue != 0)
            event_deleteQueue(currentTask->eventQueue);
    }
}

/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
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
