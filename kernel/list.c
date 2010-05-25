/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "list.h"
#include "kheap.h"
#include "console.h"


// List
listHead_t* list_Create()
{
    listHead_t* hd = (listHead_t*)malloc(sizeof(listHead_t), 0);
    if (hd)
    {
        hd->head = hd->tail = 0;
    }
    return(hd);
}

bool list_Append(listHead_t* hd, void* data)
{
    element_t* ap = (element_t*)malloc(sizeof(element_t), 0);
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
element_t* ring_Create()
{
    element_t* ring = malloc(sizeof(element_t), 0);
    ring->data = 0;
    ring->next = ring;
    return(ring);
}

void ring_Insert(element_t* ring, void* data)
{
    if(ring->data == 0) // ring is empty
    {
        ring->data = data;
    }
    else
    {
        element_t* item = malloc(sizeof(element_t), 0);
        item->data = data;
        item->next = ring->next;
        ring->next = item;
    }
}

bool ring_isEmpty(element_t* ring)
{
    return(ring->data == 0);
}

#include "video.h"
bool ring_DeleteFirst(element_t* ring, void* data)
{
    if (ring->next == ring) // If the ring contains only one element
    {
        if(ring->data == data)
        {
            //writeInfo(1, "hehe");
            ring->data = 0; // "erase" it by setting data to 0
        }
        return(ring->data == data);
    }
    else
    {
        element_t* temp = ring;
        while (temp->next != ring)
        {
            if (temp->next->data == data) // next element will be deleted
            {
                //writeInfo(0, "tätära");
                ring = temp->next;
                temp->next = temp->next->next;
                free(ring);
                return(true);
            }
            if (temp->next->next == ring && ring->data == data) // ring will be finished soon, first element of the ring will be deleted
            {
                //writeInfo(2, "törötörö");
                temp->next = ring->next;
                free(ring);
                return(true);
            }
            temp = temp->next;
        }
        return(false);
    }
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
