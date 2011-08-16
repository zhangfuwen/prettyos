/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "events.h"
#include "util.h"
#include "kheap.h"
#include "task.h"


extern list_t* tasks;   // flushEvent 

event_queue_t* event_createQueue()
{
    event_queue_t* queue = malloc(sizeof(event_queue_t), 0, "event_queue");
    queue->num = 0;
    queue->mutex = mutex_create();
    queue->list = list_create();
    return (queue);
}

void event_deleteQueue(event_queue_t* queue)
{
    for (dlelement_t* e = queue->list->head; e != 0; e = e->next)
    {
        free(((event_t*)e->data)->data);
        free(e->data);
    }
    list_free(queue->list);
    mutex_delete(queue->mutex);
    free(queue);
}

uint8_t event_issue(event_queue_t* destination, EVENT_t type, void* data, size_t length)
{
    if (destination == 0) // Event handling disabled
    {
        return (1);
    }

    if (destination->num == MAX_EVENTS)
    {
        // Overflow
        event_t* ev = malloc(sizeof(event_t), 0, "event (overflow)");
        ev->data = 0;
        ev->length = 0;
        ev->type = EVENT_OVERFLOW;
        mutex_lock(destination->mutex);
        list_append(destination->list, ev);
        destination->num++;
        mutex_unlock(destination->mutex);

        return (2);
    }
    else if (destination->num > MAX_EVENTS)
    {
        // Nothing to do. OVERFLOW event has already been added.
        return (3);
    }
    else
    {
        // Add event
        event_t* ev = malloc(sizeof(event_t), 0, "event");
        if (length > sizeof(ev->data)) // data does not fit in pointer
        {
            ev->data = malloc(length, 0, "event->data");
            memcpy(ev->data, data, length);
        }
        else // Data fits in pointer: Optimization for small data, save data in pointer itself
        {
            memcpy(&ev->data, data, length);
        }
        ev->length = length;
        ev->type = type;
        mutex_lock(destination->mutex);
        list_append(destination->list, ev);
        destination->num++;
        mutex_unlock(destination->mutex);

        return (0);
    }
}

bool flushEvent(uint32_t pid, EVENT_t filter)
{
    for (dlelement_t* e = tasks->head; e != 0; e = e->next)
    {
        if (((task_t*)e->data)->pid == pid)
        {
            list_t* eventlist = ((task_t*)e->data)->eventQueue->list;
            
            for (dlelement_t* element = eventlist->head; element != 0; element = element->next)
            {
                if ( ((event_t*)(element->data))->type == filter )
                {
                    free(element->data); 
                    mutex_lock(((task_t*)e->data)->eventQueue->mutex);
                    list_delete(eventlist, element);
                    ((task_t*)e->data)->eventQueue->num--;
                    mutex_unlock(((task_t*)e->data)->eventQueue->mutex);
                }
            }
        }
    }
    return (true);
}

EVENT_t event_poll(void* destination, size_t maxLength, EVENT_t filter)
{
    task_t* task = (task_t*)currentTask;
    
    while (task->parent && task->type == THREAD && task->eventQueue == 0)
    {
        task = task->parent; // Use parents eventQueue, if the thread has no own queue.
    }

    if (task->eventQueue == 0 || task->eventQueue->num == 0) // Event handling disabled or no events available
    {
        return(EVENT_NONE);
    }

    event_t* ev = 0;
    mutex_lock(task->eventQueue->mutex);
    
    if (filter == EVENT_NONE)
    {
        ev = task->eventQueue->list->head->data;
    }
    else
    {
        for (dlelement_t* e = task->eventQueue->list->head; e != 0; e = e->next)
        {
            ev = e->data;
            if (ev->type == filter)
                break; // found event
            else
                ev = 0;
        }
    }

    if (ev == 0)
    {
        mutex_unlock(task->eventQueue->mutex);
        return (EVENT_NONE);
    }

    EVENT_t type = EVENT_NONE;
    
    if (ev->length > maxLength)
    {
        type = EVENT_INVALID_ARGUMENTS;
        
        if (ev->length > sizeof(ev->data)) // data do not fit in pointer
        {
            free(ev->data);
        }
    }
    else
    {
        type = ev->type;
        
        if (ev->length > sizeof(ev->data)) // data does not fit in pointer
        {
            memcpy(destination, ev->data, ev->length);
            free(ev->data);
        }
        else // Data fits in pointer: Optimization for small data, data saved in pointer itself
        {
            memcpy(destination, &ev->data, ev->length);
        }
    }
    task->eventQueue->num--;
    list_delete(task->eventQueue->list, list_find(task->eventQueue->list, ev));
    mutex_unlock(task->eventQueue->mutex);
    free(ev);

    return (type);
}

event_t* event_peek(event_queue_t* eventQueue, uint32_t i)
{
    dlelement_t* elem = list_getElement(eventQueue->list, i);
    
    if (elem == 0) 
    {
        return(0);
    }
    
    return (elem->data);
}

bool event_unlockTask(void* data)
{
    return (((event_queue_t*)data)->num > 0);
}

bool waitForEvent(uint32_t timeout)
{
    return (scheduler_blockCurrentTask(BL_EVENT, currentTask->eventQueue, timeout));
}

void event_enable(bool b)
{
    if (b)
    {
        if (currentTask->eventQueue == 0)
        {
            currentTask->eventQueue = event_createQueue();
        }
    }
    else
    {
        if (currentTask->eventQueue)
        {
            event_deleteQueue(currentTask->eventQueue);
        }
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
