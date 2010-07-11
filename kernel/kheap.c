/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "console.h"
#include "paging.h"
#include "kheap.h"

/*
   The heap provides the malloc/free-functionality, i.e. dynamic allocation of memory. 
   It manages a certain amount of continuous virtual memory, starting at "heap_start". 
   Whenever more memory is requested than there is available, the heap expands.
   For expansion, the heap asks the paging module to map physical memory to the following virtual addresses 
   and increases its "heap_size" variable (but at least by "HEAP_MIN_GROWTH") afterwards.

   To manage the free and reserved (allocated) areas of the heap an array of "region" elements is kept. 
   Each region specifies its size and reservation status. 
   Free regions always get merged. Regions don't store their addresses. 
   The third region address is calculated by adding the first and second region size to "heap_start":
   region_3_addr = heap_start + regions[0].size + regions[1].size

   Before the heap is set up memory is allocated on a "placement address". 
   This is an identity mapped area of continuous memory,
   the allocation just moves a pointer forward by the requested size and returns its previous value.

   The heap's management data is placed at this placement address, too. 
   Since this area cannot grow, the heap has a maximum amount of region objects ("region_max_count").
*/

// TODO: Multithreading safety
// TODO: Ensure the heap will not overflow (over 4 GB)

static region_t*      regions          = NULL;
static uint32_t       region_count     = 0;
static uint32_t       region_max_count = 0;
static uint8_t* const heap_start       = KERNEL_HEAP_START;
static uint32_t       heap_size        = 0;
static const uint32_t HEAP_MIN_GROWTH  = 0x40000;

void heap_install()
{
    // This gets us the current placement address
    regions = malloc(0, 0);
    
    // We take the rest of the placement area
    region_count = 0;
    region_max_count = ((uintptr_t)PLACEMENT_END - (uintptr_t)regions) / sizeof(region_t);
}

static bool heap_grow(uint32_t size, uint8_t* heap_end)
{
    // We will have to append another region-object to our array if we can't merge with the last region - check whether there would be enough space to insert the region-object
    if ((region_count > 0) && regions[region_count-1].reserved && (region_count+1 > region_max_count))
    {
        return false;
    }

    // Enhance the memory
    if (!paging_alloc(kernel_pd, heap_end, size, MEM_KERNEL|MEM_WRITE))
    {
        return false;
    }

    // Maybe we can merge with the last region object?
    if ((region_count > 0) && !regions[region_count-1].reserved)
    {
        regions[region_count-1].size += size;
    }
    // Otherwise insert a new region object
    else
    {
        regions[region_count].reserved = false;
        regions[region_count].size = size;
        ++region_count;
    }

    heap_size += size;
    return true;
}

static void logHeapRegions()
{
    textColor(0x06);
    printf("\n\n---------------- HEAP REGIONS ----------------");
    printf("\naddress\t\treserved\tsize");
    
    uintptr_t region_addr = (uintptr_t)heap_start;
    for (uint32_t i=0; i<region_count; i++)
    {        
        printf("\n%X\t%s\t\t%X", region_addr, regions[i].reserved?"yes":"no", regions[i].size);
        region_addr += regions[i].size;
    }
    textColor(0x0F);
}

void* malloc(uint32_t size, uint32_t alignment)
{
    // TODO: make threadsafe 

    // Avoid odd addresses
    size = alignUp(size, 8);

    // If the heap is not set up..
    if (regions == NULL) 
    {
        // Do simple placement allocation
        static uint8_t* nextPlacement = PLACEMENT_BEGIN;
        nextPlacement = (uint8_t*) alignUp((uint32_t)nextPlacement, alignment);
        uint8_t* currPlacement = nextPlacement;
        nextPlacement += size;
        
        return currPlacement;
    }

    // Walk the regions and find one being suitable
    uint8_t* region_addr = heap_start;
    for (uint32_t i=0; i<region_count; ++i)
    {
        // Calculate aligned address and the additional size needed due to alignment
        uint8_t* aligned_addr = (uint8_t*)alignUp((uintptr_t)region_addr, alignment);
        uint32_t additional_size = aligned_addr - region_addr;

        // Check whether this region is free and big enough
        if (!regions[i].reserved && (regions[i].size >= size + additional_size))
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
            if (aligned_addr != region_addr)
            {
                // Check whether we are able to expand
                if (region_count+1 > region_max_count)
                {
                    return NULL;
                }

                // Move all following regions ahead to get room for a new one
                for (uint32_t j=region_count; j>i; --j)
                {
                    regions[j] = regions[j-1];
                }
                ++region_count;

                // Setup the regions
                regions[i].size     = aligned_addr - region_addr;
                regions[i].reserved = false;
                regions[i+1].size  -= regions[i].size;

                // "Aligned Destination Area" becomes the "current" region
                region_addr += regions[i].size;
                ++i;
            }

            // Split the leftover
            if (regions[i].size > size + additional_size)
            {
                // Check whether we are able to expand
                if (region_count+1 > region_max_count)
                {
                    return NULL;
                }

                // Move all following regions ahead to get room for a new one
                for (uint32_t j=region_count; j>i+1; --j)
                {
                    regions[j] = regions[j-1];
                }
                ++region_count;

                // Setup the regions
                regions[i+1].size     = regions[i].size - size;
                regions[i+1].reserved = false;
                regions[i].size       = size;
            }

            // Set the region to "reserved" and return its address
            regions[i].reserved = true;
            
            kdebug(3, "%X ", region_addr); 

          #ifdef _MALLOC_FREE_
            textColor(0x0E);
            printf("\nmalloc: %X", region_addr);
            textColor(0x0F);
          #endif
            
            return region_addr;

        } //region is free and big enough
        
        region_addr += regions[i].size;
    }

    // There is nothing free, try to expand the heap
    uint32_t to_grow = max(HEAP_MIN_GROWTH, alignUp(size*3/2,PAGESIZE));
    bool success = heap_grow(to_grow, (uint8_t*)(heap_start + (uintptr_t)heap_size));

    if (!success)
    {
        textColor(0x0C);
        printf("\nmalloc failed, heap could not be expanded!");
        textColor(0x0F);     
        return NULL;
    }
    else
    {
      #ifdef _MALLOC_FREE_
        textColor(0x0E);
        printf("\nheap expanded: %X heap end: %X", to_grow, (uintptr_t)(heap_start + (uintptr_t)heap_size));
        waitForKeyStroke();
        logHeapRegions();
        waitForKeyStroke();
        textColor(0x0F);
      #endif
    }

    // Now there should be a region that is large enough
    void* address = malloc(size, alignment);

    return address;
}


void free(void* addr)
{
  #ifdef _MALLOC_FREE_
    textColor(0x07);
    printf("\nfree:   %X", addr); 
    textColor(0x0F);
  #endif
    
    if (addr == 0) return; // NULL pointer

    // Walk the regions and find the correct one
    uint8_t* region_addr = heap_start;
    for (uint32_t i=0; i<region_count; ++i)
    {
        if (region_addr == addr)
        {
            regions[i].reserved = false; // free the region

            // Check for a merge with the next region
            if ((i+1 < region_count) && !regions[i+1].reserved)
            {
                // Adjust the size of the now free region
                regions[i].size += regions[i+1].size; // merge

                // Move all following regions back by one
                for (uint32_t j=i+1; j<region_count-1; ++j)
                {
                    regions[j] = regions[j+1];
                }
                --region_count;
            }

            // Check for a merge with the previous region
            if (i>0 && !regions[i-1].reserved)
            {
                // Adjust the size of the previous region
                regions[i-1].size += regions[i].size; // merge

                // Move all following regions back by one
                for (uint32_t j=i; j<region_count-1; ++j)
                {
                    regions[j] = regions[j+1];
                }
                --region_count;
            }

            return;
        }

        region_addr += regions[i].size;
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
