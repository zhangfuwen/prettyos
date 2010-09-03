/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "video/console.h"
#include "paging.h"
#include "kheap.h"
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

// TODO: Multithreading safety
// TODO: Ensure the heap will not overflow (over 4 GB)

static region_t*      regions         = 0;
static uint32_t       regionCount     = 0;
static uint32_t       regionMaxCount  = 0;
static uint8_t* const heapStart       = KERNEL_heapStart;
static uint32_t       heapSize        = 0;
static const uint32_t HEAP_MIN_GROWTH = 0x40000;

#ifdef _MEMLEAK_FIND_
  static uint32_t counter = 0;
#endif

void heap_install()
{
    // This gets us the current placement address
    regions = malloc(0, 0, "heap-start");

    // We take the rest of the placement area
    regionCount = 0;
    regionMaxCount = ((uintptr_t)PLACEMENT_END - (uintptr_t)regions) / sizeof(region_t);
}

static bool heap_grow(uint32_t size, uint8_t* heapEnd)
{
    // We will have to append another region-object to our array if we can't merge with the last region - check whether there would be enough space to insert the region-object
    if ((regionCount > 0) && regions[regionCount-1].reserved && (regionCount+1 > regionMaxCount))
    {
        return false;
    }

    // Enhance the memory
    if (!pagingAlloc(kernelPageDirectory, heapEnd, size, MEM_KERNEL|MEM_WRITE))
    {
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
        strncpy(regions[regionCount].comment,"free",5);
        regions[regionCount].number=0;

        ++regionCount;
    }

    heapSize += size;
    return true;
}

void logHeapRegions()
{
    textColor(0x0E);
    task_switching = false;
    printf("\n\n---------------- HEAP REGIONS ----------------");
    printf("\naddress\t\treserved\tsize\t\tnumber\tcomment");
    uintptr_t regionAddress = (uintptr_t)heapStart;
    for (uint32_t i=0; i<regionCount; i++)
    {
        static uint8_t lineCounter = 0;
        textColor(0x06);
        if (regions[i].reserved)
        {
            printf("\n%X\t%s\t\t%X\t%u\t%s",
                    regionAddress,
                    regions[i].reserved ? "yes" : "no",
                    regions[i].size,
                    regions[i].number,
                    regions[i].comment);
        }
        lineCounter++;
        regionAddress += regions[i].size;
        if (lineCounter >= 35)
        {
            waitForKeyStroke();
            lineCounter = 0;
        }
    }
    task_switching = true;
    textColor(0x0F);
}

void* malloc(uint32_t size, uint32_t alignment, char* comment)
{
    // TODO: make threadsafe

    // consecutive number for detecting the sequence of mallocs at the heap
    static uint32_t consecutiveNumber = 0;

    // Avoid odd addresses
    size = alignUp(size, 8);

    // If the heap is not set up..
    if (regions == 0)
    {
        // Do simple placement allocation
        static uint8_t* nextPlacement = PLACEMENT_BEGIN;
        nextPlacement = (uint8_t*) alignUp((uint32_t)nextPlacement, alignment);
        uint8_t* currPlacement = nextPlacement;
        nextPlacement += size;

        return currPlacement;
    }

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
                if (regionCount+1 > regionMaxCount)
                {
                    return 0;
                }

                // Move all following regions ahead to get room for a new one
                for (uint32_t j=regionCount; j>i; --j)
                {
                    regions[j] = regions[j-1];
                }
                ++regionCount;

                // Setup the regions
                regions[i].size     = alignedAddress - regionAddress;
                regions[i].reserved = false;
                strncpy(regions[i].comment,"free",5);

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
                    return 0;
                }

                // Move all following regions ahead to get room for a new one
                for (uint32_t j=regionCount; j>i+1; --j)
                {
                    regions[j] = regions[j-1];
                }
                ++regionCount;

                // Setup the regions
                regions[i+1].size     = regions[i].size - size;
                regions[i+1].reserved = false;
                strncpy(regions[i+1].comment,"free",5);
                regions[i+1].number=0;

                regions[i].size       = size;
            }

            // Set the region to "reserved" and return its address
            regions[i].reserved = true;
            strncpy(regions[i].comment, comment, 20);
            regions[i].number   = ++consecutiveNumber;

            kdebug(3, "%X ", regionAddress);

          #ifdef _MEMLEAK_FIND_
            counter++;
            writeInfo(2, "Malloc - free: %u", counter);
          #endif
          #ifdef _MALLOC_FREE_
            textColor(0x0E);
            task_switching = false;
            printf("\nmalloc: %X %s", regionAddress, comment);
            task_switching = true;
            textColor(0x0F);
          #endif

            return regionAddress;

        } //region is free and big enough

        regionAddress += regions[i].size;
    }

    // There is nothing free, try to expand the heap
    uint32_t sizeToGrow = max(HEAP_MIN_GROWTH, alignUp(size*3/2,PAGESIZE));
    bool success = heap_grow(sizeToGrow, (uint8_t*)(heapStart + (uintptr_t)heapSize));

    if (!success)
    {
        textColor(0x0C);
        printf("\nmalloc failed, heap could not be expanded!");
        textColor(0x0F);
        return 0;
    }
    else
    {
      #ifdef _MALLOC_FREE_
        textColor(0x0E);
        task_switching = false;
        printf("\nheap expanded: %X heap end: %X", sizeToGrow, (uintptr_t)(heapStart + (uintptr_t)heapSize));
        task_switching = true;
        waitForKeyStroke();
        textColor(0x0F);
      #endif
    }

    // Now there should be a region that is large enough
    void* address = malloc(size, alignment, comment);

    return address;
}


void free(void* addr)
{
  #ifdef _MALLOC_FREE_
    textColor(0x07);
    task_switching = false;
    printf("\nfree:   %X", addr);
    task_switching = true;
    textColor(0x0F);
  #endif

    if (addr == 0) return;

  #ifdef _MEMLEAK_FIND_
    counter--;
    writeInfo(2, "Malloc - free: %u", counter);
  #endif

    // Walk the regions and find the correct one
    uint8_t* regionAddress = heapStart;
    for (uint32_t i=0; i<regionCount; ++i)
    {
        if (regionAddress == addr)
        {
          #ifdef _MALLOC_FREE_
            textColor(0x07);
            task_switching = false;
            printf(" %s", regions[i].comment);
            task_switching = true;
            textColor(0x0F);
          #endif
            regions[i].reserved = false; // free the region
            strncpy(regions[i].comment,"free",5);
            regions[i].number = 0;

            // Check for a merge with the next region
            if ((i+1 < regionCount) && !regions[i+1].reserved)
            {
                // Adjust the size of the now free region
                regions[i].size += regions[i+1].size; // merge

                // Move all following regions back by one
                for (uint32_t j=i+1; j<regionCount-1; ++j)
                {
                    regions[j] = regions[j+1];
                }
                --regionCount;
            }

            // Check for a merge with the previous region
            if (i>0 && !regions[i-1].reserved)
            {
                // Adjust the size of the previous region
                regions[i-1].size += regions[i].size; // merge

                // Move all following regions back by one
                for (uint32_t j=i; j<regionCount-1; ++j)
                {
                    regions[j] = regions[j+1];
                }
                --regionCount;
            }

            return;
        }

        regionAddress += regions[i].size;
    }

    textColor(0x0C);
    printf("Broken free: %X\n", addr);
    textColor(0x0F);
    //ASSERT(false);
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
