/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "list.h"
#include "kheap.h"
#include "video/console.h"


// List
listHead_t* list_Create()
{
    listHead_t* hd = (listHead_t*)malloc(sizeof(listHead_t), 0, "listHead");
    if (hd)
    {
        hd->head = hd->tail = 0;
    }
    return(hd);
}

bool list_Append(listHead_t* hd, void* data)
{
    element_t* ap = (element_t*)malloc(sizeof(element_t), 0,"listElement");
    if (ap)
    {
        ap->data = data;
        ap->next = 0;

        if (!hd->head) // there exist no list element
        {
            hd->head = ap;
        }
        else // there exist at least one list element
        {
            hd->tail->next = ap;
        }
        hd->tail = ap;
        return(true);
    }
    return(false);
}

void list_Delete(listHead_t* list, void* data)
{
    element_t* cur = list->head;
    if(cur->data == data)
    {
        element_t* temp = cur;
        cur = cur->next;
        list->head = cur;
        free(temp);
    }
    while(cur != 0)
    {
        if(cur->next->data == data)
        {
            element_t* temp = cur->next;
            cur->next = cur->next->next;
            free(temp);
        }
        cur = cur->next;
    }
}

void list_DeleteAll(listHead_t* hd)
{
    element_t* cur = hd->head;
    element_t* nex;

    while (cur)
    {
        nex=cur->next;
        free(cur);
        cur=nex;
    }
    free(hd);
}

void list_Show(listHead_t* hd)
{
    printf("List elements:\n");
    element_t* cur = hd->head;
    if (!cur)
    {
        printf("The list is empty.");
    }
    else
    {
        while (cur)
        {
            printf("%X\t",cur);
            cur = cur->next;
        }
    }
}

element_t* list_GetElement(listHead_t* hd, uint32_t number)
{
    element_t* cur = hd->head;
    while (true)
    {
        if (number == 0)
        {
            return(cur);
        }
        --number;
        cur = cur->next;
    }
}


// Ring
ring_t* ring_Create()
{
    ring_t* ring = malloc(sizeof(ring_t), 0, "ring");
    ring->current = 0;
    ring->begin = 0;
    return(ring);
}

void ring_Insert(ring_t* ring, void* data)
{
    if(ring->begin == 0) // ring is empty
    {
        ring->begin = ring->current = malloc(sizeof(element_t), 0, "ring-begin");
        ring->current->next = ring->current;
        ring->current->data = data;
    }
    else
    {
        element_t* item = malloc(sizeof(element_t), 0, "ring-element");
        item->data = data;
        item->next = ring->current->next;
        ring->current->next = item;
    }
}

bool ring_isEmpty(ring_t* ring)
{
    return(ring->begin == 0);
}

bool ring_DeleteFirst(ring_t* ring, void* data)
{
    element_t* current = ring->current;
    if(ring->begin == 0) return(false);
    do
    {
        if (current->next->data == data) // next element will be deleted
        {
            element_t* temp = current->next;
            if(ring->begin == ring->begin->next) // Just one element
            {
                ring->begin = 0;
                ring->current = 0;
            }
            else
            {
                if(temp == ring->begin)   ring->begin   = ring->begin->next;
                if(temp == ring->current) ring->current = ring->current->next;
                current->next = current->next->next;
            }
            free(temp);
            return(true);
        }
        current = current->next;
    } while (current != ring->current);
    return(false);
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
