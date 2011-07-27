/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "list.h"
#include "util.h"
#include "kheap.h"


list_t* list_create()
{
    list_t* list = (list_t*)malloc(sizeof(list_t), 0, "listHead");
    if (list)
    {
        list->head = list->tail = 0;
    }
    return(list);
}

bool list_append(list_t* list, void* data)
{
    ASSERT(list);
    element_t* newElement = (element_t*)malloc(sizeof(element_t), 0, "listElement");
    if (newElement)
    {
        newElement->data = data;
        newElement->next = 0;

        if (list->head == 0) // there exist no list element
        {
            list->head = list->tail = newElement;
        }
        else // there exist at least one list element
        {
            list->tail->next = newElement;
            list->tail = newElement;
        }
        return(true);
    }
    return(false);
}

bool list_insert(list_t* list, element_t* next, void* data)
{
    ASSERT(list);
    if (next == 0)
    {
        return (list_append(list, data));
    }

    if (next == list->head)
    {
        element_t* newElement = (element_t*)malloc(sizeof(element_t), 0,"listElement");
        if (newElement)
        {
            newElement->next = list->head;
            list->head = newElement;
            newElement->data = data;
            return true;
        }
        return(false);
    }

    element_t* prev = 0;
    element_t* cur  = list->head;

    while(cur != 0 && cur != next)
    {
        prev = cur;
        cur  = cur->next;
    }

    // insert left of next
    element_t* newElement = (element_t*)malloc(sizeof(element_t), 0,"listElement");
    if (newElement)
    {
        newElement->data = data;
        newElement->next = next;
        prev->next = newElement;
        return true;
    }
    return false;
}

element_t* list_delete(list_t* list, void* data)
{
    ASSERT(list);
    element_t* cur = list->head;
    element_t* retval = 0;
    while(cur != 0 && cur->data == data)
    {
        element_t* temp = cur;
        if(!retval) retval = temp->next; // Save element behind the element we are going to delete
        cur = cur->next;
        if(list->head == list->tail) list->tail = 0;
        list->head = cur;
        free(temp);
    }
    while(cur != 0 && cur->next != 0)
    {
        if(cur->next->data == data)
        {
            element_t* temp = cur->next;
            if(!retval) retval = temp->next; // Save element behind the element we are going to delete
            if(cur->next == list->tail) list->tail = cur;
            cur->next = cur->next->next;
            free(temp);
        }
        cur = cur->next;
    }
    return(retval);
}

void list_free(list_t* list)
{
    ASSERT(list);
    element_t* cur = list->head;
    element_t* nex;

    while (cur)
    {
        nex=cur->next;
        free(cur);
        cur=nex;
    }
    free(list);
}

element_t* list_getElement(list_t* list, uint32_t number)
{
    ASSERT(list);
    element_t* cur = list->head;
    while (true)
    {
        if (number == 0 || cur == 0)
        {
            return(cur);
        }
        --number;
        cur = cur->next;
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
