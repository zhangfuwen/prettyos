/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "synchronisation.h"
#include "kheap.h"
#include "util.h"
#include "sys_speaker.h"

semaphore_t* semaphore_create(uint16_t resourceCount)
{
    semaphore_t* obj = malloc(sizeof(semaphore_t), 0);
    obj->resCount = max(resourceCount, 1);
    obj->resources = malloc(sizeof(task_t*) * obj->resCount, 0);
    memsetl((uint32_t*)obj->resources, 0, obj->resCount);
    obj->freeRes = 0;
    return(obj);
}

void semaphore_lock(semaphore_t* obj)
{
    if(obj == 0) return;

    if(obj->freeRes == 0xFFFFFFFF) // blocked -> wait (busy wait is a HACK)
    {
        currentTask->blockType = BL_SEMAPHORE;
        currentTask->blockData = obj;
		if(task_switching)
			switch_context();
    }
    while(obj->freeRes == 0xFFFFFFFF) {nop();} // HACK
    
    for(int i = 0; i < obj->resCount; i++)
    {
        if(obj->resources[i] == currentTask) // Task is already blocking
            return;
    }
    
    obj->resources[obj->freeRes++] = currentTask;
    for(; obj->freeRes < obj->resCount; obj->freeRes++)
    {
        if(obj->resources[obj->freeRes] == 0)
        {
            return;
        }
    }
    obj->freeRes = 0xFFFFFFFF; // All resources blocked now
}

void semaphore_unlock(semaphore_t* obj)
{
    if(obj == 0) return;

    for(int i = 0; i < obj->resCount; i++)
    {
        if(obj->resources[i] == currentTask) // found -> delete
        {
            obj->resources[i] = 0;
            obj->freeRes = min(i, obj->freeRes);
            return;
        }
    }
}

void semaphore_delete(semaphore_t* obj)
{
    free(obj->resources);
    free(obj);
}

/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
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
