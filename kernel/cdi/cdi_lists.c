/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "cdi/lists.h"
#include "list.h"
#include "kheap.h"


cdi_list_t cdi_list_create()
{
    return(list_create());
}

void cdi_list_destroy(cdi_list_t list)
{
    list_free(list);
}

cdi_list_t cdi_list_push(cdi_list_t list, void* value)
{
    list_append(list, value); /// Probably not good... push maybe != append
    return(list); /// Maybe wrong
}

void* cdi_list_pop(cdi_list_t list) {
    void* retVal = list->head->data;
    element_t* temp = list->head;
    list->head = list->head->next;
    free(temp);
    return(retVal);
}

size_t cdi_list_empty(cdi_list_t list);

void* cdi_list_get(cdi_list_t list, size_t index)
{
    element_t* temp = list_getElement(list, index);
    if(temp == 0)
    {
        return(0);
    }
    return(temp->data);
}

cdi_list_t cdi_list_insert(cdi_list_t list, size_t index, void* value);

void* cdi_list_remove(cdi_list_t list, size_t index);

size_t cdi_list_size(cdi_list_t list);

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
