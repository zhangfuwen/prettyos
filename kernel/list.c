/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
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
	ASSERT(hd);
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

void list_Delete(listHead_t* hd, void* data)
{
	ASSERT(hd);
	element_t* cur = hd->head;
    while(cur != 0 && cur->data == data)
    {
        element_t* temp = cur;
        cur = cur->next;
        if(hd->head == hd->tail) hd->tail = 0;
        hd->head = cur;
        free(temp);
    }
    while(cur != 0 && cur->next != 0)
    {
        if(cur->next->data == data)
        {
            element_t* temp = cur->next;
            if(cur->next == hd->tail) hd->tail = cur;
            cur->next = cur->next->next;
            free(temp);
        }
        cur = cur->next;
    }
}

void list_DeleteAll(listHead_t* hd)
{
    ASSERT(hd);
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

element_t* list_GetElement(listHead_t* hd, uint32_t number)
{
    ASSERT(hd);
	element_t* cur = hd->head;
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


// Ring
ring_t* ring_Create()
{
	ring_t* ring = malloc(sizeof(ring_t), 0, "ring");
    ring->current = 0;
    ring->begin = 0;
    return(ring);
}

static void putIn(ring_t* ring, element_t* prev, element_t* elem)
{
    ASSERT(ring);
	if(ring->begin == 0) // Ring is empty
    {
        ring->begin = elem;
        ring->current = elem;
        elem->next = elem;
    }
    else
    {
        elem->next = prev->next;
        prev->next = elem;
    }
}

static void takeOut(ring_t* ring, element_t* prev)
{
    ASSERT(ring);
	if(prev->next == prev) // Just one element in ring
    {
        ring->begin = 0;
        ring->current = 0;
    }
    else
    {
        if(prev->next == ring->begin)   ring->begin   = prev->next->next;
        if(prev->next == ring->current) ring->current = prev->next->next;
        prev->next = prev->next->next;
    }
}

void ring_Insert(ring_t* ring, void* data, bool single)
{
    ASSERT(ring);
	if(single && ring->begin != 0) // check if an element with the same data is already in the ring
    {
        element_t* current = ring->current;
        do
        {
            if(current->data == data) return;
            current = current->next;
        } while (current != ring->current);
    }
    element_t* item = malloc(sizeof(element_t), 0, "ring-element");
    item->data = data;
    putIn(ring, ring->current, item);
}

bool ring_isEmpty(ring_t* ring)
{
    ASSERT(ring);
	return(ring->begin == 0);
}

bool ring_DeleteFirst(ring_t* ring, void* data)
{
    ASSERT(ring);
	if(ring->begin == 0) return(false);

    element_t* current = ring->current;
    do
    {
        if (current->next->data == data) // next element will be deleted
        {
            element_t* temp = current->next;
            takeOut(ring, current);
            free(temp);
            return(true);
        }
        current = current->next;
    } while (current != ring->current);
    return(false);
}

void ring_move(ring_t* dest, ring_t* source, void* data)
{
    
	if(source == 0 || dest == 0 || source->begin == 0) return;

    element_t* prev = source->begin;
    element_t* current = prev->next;
    do
    {
        if(current->data == data) // Found. Take it out.
        {
            takeOut(source, prev);
            break;
        }
        prev = current;
        current = current->next;
    } while (prev != source->begin);

    if(current->data == data) // Found element. Insert it to dest ring.
        putIn(dest, dest->current, current);
    else // Create new element. Insert it to dest ring.
        ring_Insert(dest, data, true);
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
