/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "kheap.h"
#include "util.h"
#include "memory.h"
#include "video/console.h"
#include "paging.h"
#include "serial.h"
#include "task.h"

/*
   The heap provides the malloc/free-functionality, i.e. dynamic allocation of memory.
   It manages a certain amount of continuous virtual memory, starting at "heapStart".
   Whenever more memory is requested than there is available, the heap expands.
   For expansion, the heap asks the paging module to map physical memory to the following virtual addresses
   and increases its "heapSize" variable (but at least by "HEAP_MIN_GROWTH") afterwards.

   To manage the free and reserved (allocated) areas of the heap an array of "region" elements is kept.
   Each region specifies its size and reservation status.
   Free regions always get merged. Regions don't store their addresses.
   The third region address is calculated by adding the first and second region size to "heapStart":
   region_3_addr = heapStart + regions[0].size + regions[1].size

   Before the heap is set up memory is allocated on a "placement address".
   This is an identity mapped area of continuous memory,
   the allocation just moves a pointer forward by the requested size and returns its previous value.

   The heap's management data is placed at this placement address, too.
   Since this area cannot grow, the heap has a maximum amount of region objects ("regionMaxCount").
*/

// TODO: Ensure the heap will not overflow (over 4 GB)

static region_t*      regions         = 0;
static uint32_t       regionCount     = 0;
static uint32_t       regionMaxCount  = 0;
static uint8_t* const heapStart       = KERNEL_heapStart;
static uint32_t       heapSize        = 0;
static const uint32_t HEAP_MIN_GROWTH = 0x40000;

static mutex_t* mutex = 0;

#ifdef _MEMLEAK_FIND_
  static uint32_t counter = 0;
#endif

static void* placementMalloc(uint32_t size, uint32_t alignment);


void heap_install()
{
    mutex = mutex_create();

    // This gets us the current placement address
    regions = placementMalloc(0, 0);

    // We take the rest of the placement area
    regionCount = 0;
    regionMaxCount = ((uintptr_t)PLACEMENT_END - (uintptr_t)regions) / sizeof(region_t);
}

static bool heap_grow(uint32_t size, uint8_t* heapEnd)
{
    // We will have to append another region-object to our array if we can't merge with the last region - check whether there would be enough space to insert the region-object
    if ((regionCount > 0) && regions[regionCount-1].reserved && (regionCount >= regionMaxCount))
    {
        return false;
    }

    mutex_lock(mutex);
    // Enhance the memory
    if (!paging_alloc(kernelPageDirectory, heapEnd, size, MEM_KERNEL|MEM_WRITE))
    {
        mutex_unlock(mutex);
        return false;
    }

    // Maybe we can merge with the last region object?
    if ((regionCount > 0) && !regions[regionCount-1].reserved)
    {
        regions[regionCount-1].size += size;
    }
    // Otherwise insert a new region object
    else
    {
        regions[regionCount].reserved = false;
        regions[regionCount].size = size;
      #ifdef _MALLOC_FREE_
        strcpy(regions[regionCount].comment, "free");
        regions[regionCount].number=0;
      #endif

        ++regionCount;
    }

    heapSize += size;
    mutex_unlock(mutex);
    return true;
}

static void* placementMalloc(uint32_t size, uint32_t alignment)
{
    static void* nextPlacement = PLACEMENT_BEGIN;

    // Avoid odd addresses
    size = alignUp(size, 4);

    if((uintptr_t)nextPlacement+size > (uintptr_t)PLACEMENT_END)
        return(0);

    mutex_lock(mutex);
    // Do simple placement allocation
    nextPlacement = (void*)alignUp((uintptr_t)nextPlacement, alignment);
    void* currPlacement = nextPlacement;
    nextPlacement += size;

    mutex_unlock(mutex);
    return currPlacement;
}

void* malloc(uint32_t size, uint32_t alignment, char* comment)
{
    // consecutive number for detecting the sequence of mallocs at the heap
    static uint32_t consecutiveNumber = 0;

    // If the heap is not set up, do placement malloc
    if (regions == 0)
    {
        return placementMalloc(size, alignment);
    }

    // Avoid odd addresses
    size = alignUp(size, 4);

    mutex_lock(mutex);
    // Walk the regions and find one being suitable
    uint8_t* regionAddress = heapStart;
    for (uint32_t i=0; i<regionCount; ++i)
    {
        // Calculate aligned address and the additional size needed due to alignment
        uint8_t* alignedAddress = (uint8_t*)alignUp((uintptr_t)regionAddress, alignment);
        uint32_t additionalSize = alignedAddress - regionAddress;

        // Check whether this region is free and big enough
        if (!regions[i].reserved && (regions[i].size >= size + additionalSize))
        {
            // We will split up this region ...
            // +--------------------------------------------------------+
            // |                      Current Region                    |
            // +--------------------------------------------------------+
            //
            // ... into three, the first and third remain free,
            // while the second gets reserved, and its address is returned
            //
            // +------------------+--------------------------+----------+
            // | Before Alignment | Aligned Destination Area | Leftover |
            // +------------------+--------------------------+----------+

            // Split the pre-alignment area
            if (alignedAddress != regionAddress)
            {
                // Check whether we are able to expand
                if (regionCount >= regionMaxCount)
                {
                    mutex_unlock(mutex);
                    return 0;
                }

                // Move all following regions ahead to get room for a new one
                memmove(regions + i+1, regions + i, (regionCount-i)*sizeof(region_t));

                ++regionCount;

                // Setup the regions
                regions[i].size     = alignedAddress - regionAddress;
                regions[i].reserved = false;
              #ifdef _MALLOC_FREE_
                strcpy(regions[i].comment, "free");
              #endif

                regions[i+1].size  -= regions[i].size;

                // "Aligned Destination Area" becomes the "current" region
                regionAddress += regions[i].size;
                ++i;
            }

            // Split the leftover
            if (regions[i].size > size + additionalSize)
            {
                // Check whether we are able to expand
                if (regionCount+1 > regionMaxCount)
                {
                    mutex_unlock(mutex);
                    return 0;
                }

                // Move all following regions ahead to get room for a new one
                memmove(regions + i+2, regions + i+1, (regionCount-i-1)*sizeof(region_t));

                ++regionCount;

                // Setup the regions
                regions[i+1].size     = regions[i].size - size;
                regions[i+1].reserved = false;
              #ifdef _MALLOC_FREE_
                strcpy(regions[i+1].comment, "free");
                regions[i+1].number=0;
              #endif

                regions[i].size       = size;
            }

            // Set the region to "reserved" and return its address
            regions[i].reserved = true;
            strncpy(regions[i].comment, comment, 20);
            regions[i].comment[20] = 0;
            regions[i].number   = ++consecutiveNumber;

            kdebug(3, "%Xh ", regionAddress);

          #ifdef _MEMLEAK_FIND_
            counter++;
            writeInfo(2, "Malloc - free: %u", counter);
          #endif
          #ifdef _MALLOC_FREE_
            textColor(YELLOW);
            task_switching = false;
            printf("\nmalloc: %Xh %s", regionAddress, comment);
            task_switching = true;
            textColor(TEXT);
          #endif

            mutex_unlock(mutex);
            return regionAddress;

        } //region is free and big enough

        regionAddress += regions[i].size;
    }

    // There is nothing free, try to expand the heap
    uint32_t sizeToGrow = max(HEAP_MIN_GROWTH, alignUp(size*3/2,PAGESIZE));
    bool success = heap_grow(sizeToGrow, (uint8_t*)(heapStart + (uintptr_t)heapSize));

    mutex_unlock(mutex);

    if (!success)
    {
        textColor(RED);
        printf("\nmalloc failed, heap could not be expanded!");
        textColor(TEXT);
        return 0;
    }
    else
    {
      #ifdef _MALLOC_FREE_
        textColor(YELLOW);
        task_switching = false;
        printf("\nheap expanded: %Xh heap end: %Xh", sizeToGrow, (uintptr_t)(heapStart + (uintptr_t)heapSize));
        task_switching = true;
        textColor(TEXT);
      #endif
    }

    // Now there should be a region that is large enough
    void* address = malloc(size, alignment, comment);

    return address;
}


void free(void* addr)
{
  #ifdef _MALLOC_FREE_
    textColor(LIGHT_GRAY);
    task_switching = false;
    printf("\nfree:   %Xh", addr);
    task_switching = true;
    textColor(TEXT);
  #endif

    if (addr == 0) return;

  #ifdef _MEMLEAK_FIND_
    counter--;
    writeInfo(2, "Malloc - free: %u", counter);
  #endif

    mutex_lock(mutex);

    // Walk the regions and find the correct one
    uint8_t* regionAddress = heapStart;
    for (uint32_t i=0; i<regionCount; ++i)
    {
        if (regionAddress == addr)
        {
          #ifdef _MALLOC_FREE_
            textColor(LIGHT_GRAY);
            task_switching = false;
            printf(" %s", regions[i].comment);
            task_switching = true;
            textColor(TEXT);
            strcpy(regions[i].comment, "free");
            regions[i].number = 0;
          #endif
            regions[i].reserved = false; // free the region

            // Check for a merge with the next region
            if ((i+1 < regionCount) && !regions[i+1].reserved)
            {
                // Adjust the size of the now free region
                regions[i].size += regions[i+1].size; // merge

                // Move all following regions back by one
                memmove(regions + i+1, regions + i+2, (regionCount-1-i)*sizeof(region_t));

                --regionCount;
            }

            // Check for a merge with the previous region
            if (i>0 && !regions[i-1].reserved)
            {
                // Adjust the size of the previous region
                regions[i-1].size += regions[i].size; // merge

                // Move all following regions back by one
                memmove(regions + i, regions + i+1, (regionCount-i)*sizeof(region_t));

                --regionCount;
            }

            mutex_unlock(mutex);
            return;
        }

        regionAddress += regions[i].size;
    }

    mutex_unlock(mutex);

    textColor(ERROR);
    printf("Broken free: %Xh\n", addr);
    textColor(TEXT);
}

void heap_logRegions()
{
    printf("\nDebug: Heap regions sent to serial output.\n");
    serial_log(4,"\r\n\r\nregionMaxCount: %u\r\n", regionMaxCount);
    serial_log(4,"\r\n\r\n---------------- HEAP REGIONS ----------------\r\n");
    serial_log(4,"address\t\tsize\t\tnumber\tcomment");

    uintptr_t regionAddress = (uintptr_t)heapStart;

    for (uint32_t i=0; i<regionCount; i++)
    {
        if (regions[i].reserved)
        {
            serial_log(4,"\r\n%Xh\t%Xh\t%u\t%s", regionAddress, regions[i].size, regions[i].number, regions[i].comment);
        }
        regionAddress += regions[i].size;
    }
    serial_log(4,"\r\n---------------- HEAP REGIONS ----------------\r\n\r\n");
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
