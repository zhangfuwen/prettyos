/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "list.h"
#include "util.h"
#include "kheap.h"


list_t* list_create()
{
    list_t* list = malloc(sizeof(list_t), 0, "listHead");
    if (list)
    {
        list->head = 0;
        list->tail = 0;
    }
    return(list);
}

dlelement_t* list_append(list_t* list, void* data)
{
    dlelement_t* newElement = malloc(sizeof(dlelement_t), 0, "listElement");
    if (newElement)
    {
        newElement->data = data;
        newElement->next = 0;
        newElement->prev = list->tail;

        if (list->head == 0)
        {
            list->head = newElement;
        }
        else
        {
            list->tail->next = newElement;
        }
        list->tail = newElement;
        return(newElement);
    }
    return (0);
}

dlelement_t* list_insert(list_t* list, dlelement_t* next, void* data)
{
    if (next == 0)
    {
        return(list_append(list, data));
    }

    dlelement_t* newElement = malloc(sizeof(dlelement_t), 0, "listElement");
    if (newElement)
    {
        newElement->data = data;

        if (next == list->head)
        {
            newElement->next = list->head;
            newElement->prev = 0;
            list->head->prev = newElement;
            list->head       = newElement;
        }
        else
        {
            next->prev->next = newElement;
            newElement->prev = next->prev;
            newElement->next = next;
            next->prev       = newElement;
        }
        
        return newElement;
    }

    return (0);
}

dlelement_t* list_delete(list_t* list, dlelement_t* elem)
{
    if (list->head == 0)
    {
        return 0;
    }

    if (list->head == list->tail)
    {
        free(elem);
        list->head = list->tail = 0;
        return 0;
    }

    dlelement_t* temp = elem->next;

    if (elem == list->head)
    {
        list->head       = elem->next;
        list->head->prev = 0;
    }
    else if (elem == list->tail)
    {
        list->tail       = elem->prev;
        list->tail->next = 0;
    }
    else
    {
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;
    }

    free(elem);

    return temp;
}

void list_free(list_t* list)
{
    dlelement_t* cur = list->head;
    dlelement_t* nex;

    while (cur)
    {
        nex=cur->next;
        free(cur);
        cur=nex;
    }
    
    free(list);
}

dlelement_t* list_getElement(list_t* list, uint32_t number)
{
    dlelement_t* cur = list->head;
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

dlelement_t* list_find(list_t* list, void* data)
{
    dlelement_t* cur = list->head;
    while (cur && cur->data != data)
    {
        cur = cur->next;
    }
    
    return(cur);
}

size_t list_getCount(list_t* list)
{
    size_t count;
    dlelement_t* cur = list->head;
    for (count = 0; cur ; count++) 
    {
        cur = cur->next;        
    }
    
    return(count);
}

bool list_isEmpty(list_t* list)
{
    return (list->head == 0);
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
