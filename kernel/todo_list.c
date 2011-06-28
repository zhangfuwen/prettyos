/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "todo_list.h"
#include "kheap.h"
#include "scheduler.h"


todoList_t* todoList_create()
{
    todoList_t* list = malloc(sizeof(list), 0, "todoList");
    list->queue = list_Create();
    return(list);
}

void todoList_add(todoList_t* list, void (*function)())
{
    list_Append(list->queue, function);
}

void todoList_clear(todoList_t* list)
{
    if(list->queue->head != 0)
    {
        list_DeleteAll(list->queue);
        list->queue = list_Create();
    }
}

void todoList_execute(todoList_t* list)
{
    for(element_t* e = list->queue->head; e != 0; e = e->next)
    {
        ((void (*)())e->data)();
    }
    todoList_clear(list);
}

void todoList_wait(todoList_t* list)
{
    scheduler_blockCurrentTask(BL_TODOLIST, list, 0);
}

bool todoList_unlockTask(void* data)
{
    return(((todoList_t*)data)->queue->head != 0);
}

void todoList_delete(todoList_t* list)
{
    list_DeleteAll(list->queue);
    free(list);
}


/*
* Copyright (c) 2010-2011 The PrettyOS Project. All rights reserved.
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
