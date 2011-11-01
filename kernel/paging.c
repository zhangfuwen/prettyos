/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// TODO: Apply multithreading safety

#include "paging.h"
#include "memory.h"
#include "util.h"
#include "timer.h"
#include "task.h"
#include "kheap.h"
#include "ipc.h"
#include "video/console.h"

#define IDMAP  5  // 0 MiB - 20 MiB, identity mapping

pageDirectory_t* kernelPageDirectory;

// Memory Map
memoryMapEntry_t* memoryMapAdress;
memoryMapEntry_t* memoryMapEnd;

extern char _kernel_beg, _kernel_end; // defined in linker script

// Physical Memory
static const uint32_t MAX_DWORDS = 0x100000000ull / PAGESIZE / 32;
static uint32_t*      bittable;
static uint32_t       firstFreeDWORD = 10 * 1024 * 1024 / PAGESIZE / 32; // Exclude the first 10 MiB from being allocated (needed for DMA later on)
static uint32_t       physMemInit();


void paging_switch(pageDirectory_t* pd)
{
  #ifdef _PAGING_DIAGNOSIS_
    textColor(MAGENTA);
    printf("\nDEBUG: paging_switch: pd=%X, pd->physAddr=%X", pd, pd->physAddr);
    textColor(TEXT);
  #endif
    __asm__ volatile("mov %0, %%cr3" : : "r" (pd->physAddr));
}

uint32_t paging_install()
{
    uint32_t ram_available = physMemInit();

    // Setup the kernel page directory
    kernelPageDirectory = malloc(sizeof(pageDirectory_t), PAGESIZE, "pag-kernelPD");
    memset(kernelPageDirectory, 0, sizeof(pageDirectory_t) - 4);
    kernelPageDirectory->physAddr = (uint32_t)kernelPageDirectory;

    kdebug(3, "\nkernelPageDirectory (virt., phys.): %Xh, %Xh\n", kernelPageDirectory, kernelPageDirectory->physAddr);

    // Setup the page tables for 0 MiB - 20 MiB, identity mapping
    uint32_t addr = 0;
    for (uint8_t i=0; i<IDMAP; i++)
    {
        // Page directory entry, virt=phys due to placement allocation in id-mapped area
        kernelPageDirectory->tables[i] = malloc(sizeof(pageTable_t), PAGESIZE, "pag-kernelPT");
        kernelPageDirectory->codes[i]  = (uint32_t)kernelPageDirectory->tables[i] | MEM_PRESENT;

        // Page table entries, identity mapping
        for (uint32_t j=0; j<1024; ++j)
        {
            kernelPageDirectory->tables[i]->pages[j] = addr | MEM_PRESENT | MEM_WRITE;
            addr += PAGESIZE;
        }
    }

    // Setup the page tables for PCI memory (3 - 3,5 GiB) and kernel heap (3,5 - 4 GiB), unmapped
    size_t kernelpts = 128 + min(128, ram_available / 4096 / 1024); // PCI memory size + maximum kernel heap size (limited by available memory)
    pageTable_t* heap_pts = malloc(kernelpts*sizeof(pageTable_t), PAGESIZE, "pag-PTheap");
    memset(heap_pts, 0, kernelpts * sizeof(pageTable_t));
    for (uint32_t i = 0; i < kernelpts; i++)
    {
        kernelPageDirectory->tables[0x300 + i] = heap_pts + i;
        kernelPageDirectory->codes [0x300 + i] = (uint32_t)kernelPageDirectory->tables[0x300 + i] | MEM_PRESENT | MEM_WRITE;
    }

    // Tell CPU to enable paging
    paging_switch(kernelPageDirectory);
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0": "=r"(cr0)); // read CR0
    cr0 |= 0x80000000;                            // set the paging bit in CR0
    __asm__ volatile("mov %0, %%cr0":: "r"(cr0)); // write CR0

    return (ram_available);
}


static bool isMemoryMapAvailable(const memoryMapEntry_t* entries, uint64_t beg, uint64_t end)
{
    uint64_t covered = beg;
    for (const memoryMapEntry_t* outerLoop=entries; outerLoop < memoryMapEnd; outerLoop++)
    {
        // There must not be an "reserved" entry which reaches into the specified area
        if ((outerLoop->type != 1) && (outerLoop->base < end) && ((outerLoop->base + outerLoop->size) > beg))
        {
            return (false);
        }
        // Check whether the "free" entries cover the whole specified area.
        for (const memoryMapEntry_t* entry=entries; entry < memoryMapEnd; entry++)
        {
            if (entry->base <= covered && (entry->base + entry->size) > covered)
            {
                covered = entry->base + entry->size;
            }
        }
    }

    // Return whether the whole area is covered by "free" entries
    return (covered >= end);
}

static void physSetBits(uint32_t addrBegin, uint32_t addrEnd, bool reserved)
{
    // Calculate the bit-numbers
    uint32_t start = alignUp  (addrBegin, PAGESIZE) / PAGESIZE;
    uint32_t end   = alignDown(addrEnd,   PAGESIZE) / PAGESIZE;

    // Set all these bits
    for (uint32_t j=start; j<end; ++j)
    {
        if (reserved)
        {
            bittable[j/32] |= BIT(j%32);
        }
        else
        {
            bittable[j/32] &= ~BIT(j%32);
        }
    }
}

static uint32_t physMemInit()
{
    static const uint64_t FOUR_GB  = 0x100000000ull;

  #ifdef _DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nMemory map:");
    textColor(TEXT);
  #endif

    // Prepare the memory map entries, since we work with max 4 GB only. The last entry in the entry-array has size 0.
    for (memoryMapEntry_t* entry = memoryMapAdress; entry < memoryMapEnd; entry++)
    {
      #ifdef _DIAGNOSIS_
        printf("\n  %Xh -> %Xh %u", entry->base, entry->base+entry->size, entry->type); // Print the memory map
      #endif

        // We will completely ignore memory above 4 GB or with size of 0, move following entries backward by one then
        if (entry->base >= FOUR_GB || entry->size == 0)
        {
            memmove(entry, entry + 1, (memoryMapEnd-entry) / sizeof(entry));
            memoryMapEnd--;
        }

        // Eventually reduce the size so the the block doesn't exceed 4 GB
        else if (entry->base + entry->size >= FOUR_GB)
        {
            entry->size = FOUR_GB - entry->base;
        }
    }

    // Check that 10 MiB-20 MiB is free for use
    if (!isMemoryMapAvailable(memoryMapAdress, 10*1024*1024, 20*1024*1024))
    {
        textColor(ERROR);
        printf("The memory between 10 MiB and 20 MiB is not free for use. OS halted!\n");
        cli();
        hlt();
    }

    // We store our data here, initialize all bits to "reserved"
    bittable = malloc(MAX_DWORDS * 4, 0, "pag-bittable");
    memset(bittable, 0xFF, MAX_DWORDS * 4);

    // Set the bitmap bits according to the memory map now. "type==1" means "free".
    for (const memoryMapEntry_t* entry = memoryMapAdress; entry < memoryMapEnd; entry++)
    {
        if(entry->type == 1) // Set bits to "free"
        {
            physSetBits(entry->base, entry->base+entry->size, false);
        }
    }

    // Find the number of dwords we can use, skipping the last, "reserved"-only ones
    uint32_t dwordCount = 0;

    for (uint32_t i=0; i<MAX_DWORDS; i++)
    {
        if (bittable[i] != 0xFFFFFFFF)
        {
            dwordCount = i + 1;
        }
    }

    // Reserve first 20 MiB
    physSetBits(0x00000000, 20*1024*1024, true);

    // Reserve the region of the kernel code
    physSetBits((uint32_t)&_kernel_beg, (uint32_t)&_kernel_end, true); // from linker script

    kdebug(3, "Highest available RAM: %Xh\n", dwordCount*32*4096);

    // Return the amount of memory available (or rather the highest address)
    return (dwordCount * 32 * 4096);
}

static uint32_t physMemAlloc()
{
    // Search for a free uint32_t, i.e. one that not only contains ones
    for (; firstFreeDWORD < MAX_DWORDS; firstFreeDWORD++)
    {
        if (bittable[firstFreeDWORD] != 0xFFFFFFFF)
        {
            uint32_t bitnr;
            // Find the number of first free bit.
            // This inline assembler instruction is smaller and faster than a C loop to identify this bit
            __asm__ volatile("bsfl %1, %0" : "=r"(bitnr) : "r"(~bittable[firstFreeDWORD]));

            // Set the page to "reserved" and return the frame's address
            bittable[firstFreeDWORD] |= BIT(bitnr % 32);
            return ((firstFreeDWORD * 32 + bitnr) * PAGESIZE);
        }
    }

    // No free page found
    return (0);
}

static void physMemFree(uint32_t addr)
{
    // Calculate the number of the bit
    uint32_t bitnr = addr / PAGESIZE;

    // Maybe the affected dword (which has a free bit now) is ahead of firstFreeDWORD?
    if (bitnr/32 < firstFreeDWORD)
    {
        firstFreeDWORD = bitnr/32;
    }

    // Set the page to "free"
    bittable[bitnr/32] &= ~BIT(bitnr%32);
}

bool paging_alloc(pageDirectory_t* pd, void* virtAddress, uint32_t size, MEMFLAGS_t flags)
{
    // "virtAddress" and "size" must be page-aligned
    ASSERT(((uint32_t)virtAddress) % PAGESIZE == 0);
    ASSERT(size % PAGESIZE == 0);

    // We repeat allocating one page at once
    for (uint32_t done=0; done<size/PAGESIZE; done++)
    {
        uint32_t pagenr = (uint32_t)virtAddress/PAGESIZE + done;

        // Maybe there is already memory allocated?
        if (pd->tables[pagenr/1024] && pd->tables[pagenr/1024]->pages[pagenr%1024])
        {
            kdebug(3, "pagenumber already allocated: %u\n", pagenr);
            continue;
        }

        // Allocate physical memory
        uint32_t physAddress = physMemAlloc();

        if (physAddress == 0)
        {
            // Undo the allocations and return an error
            paging_free(pd, virtAddress, done*PAGESIZE);
            return (false);
        }

        // Get the page table
        pageTable_t* pt = pd->tables[pagenr/1024];

        if (!pt)
        {
            // Allocate the page table
            pt = malloc(sizeof(pageTable_t), PAGESIZE, "pageTable");

            if (!pt)
            {
                // Undo the allocations and return an error
                physMemFree(physAddress);
                paging_free(pd, virtAddress, done*PAGESIZE);
                return (false);
            }
            memset(pt, 0, sizeof(pageTable_t));
            pd->tables[pagenr/1024] = pt;

            // Set physical address and flags
            pd->codes[pagenr/1024] = paging_getPhysAddr(pt) | MEM_PRESENT | MEM_WRITE | flags;
        }

        // Setup the page
        pt->pages[pagenr%1024] = physAddress | flags | MEM_PRESENT;

        if (flags & MEM_USER)
        {
            kdebug(3, "pagenumber now allocated: %u physAddress: %Xh\n", pagenr, physAddress);
        }
    }
    return (true);
}

void paging_free(pageDirectory_t* pd, void* virtAddress, uint32_t size)
{
    // "virtAddress" and "size" must be page-aligned
    ASSERT(((uint32_t)virtAddress) % PAGESIZE == 0);
    ASSERT(size % PAGESIZE == 0);

    // Go through all pages and free them
    uint32_t pagenr = (uint32_t)virtAddress / PAGESIZE;

    while (size)
    {
        // Get the physical address and invalidate the page
        uint32_t* page = &pd->tables[pagenr/1024]->pages[pagenr%1024];
        uint32_t physAddress = *page & 0xFFFFF000;
        *page = 0;

        // Free memory and adjust variables for next loop run
        physMemFree(physAddress);
        size -= PAGESIZE;
        pagenr++;
    }
}

pageDirectory_t* paging_createUserPageDirectory()
{
    // Allocate memory for the page directory
    pageDirectory_t* pd = (pageDirectory_t*) malloc(sizeof(pageDirectory_t), PAGESIZE,"pag-userPD");

    if (!pd)
    {
        return (0);
    }

    // Each user's page directory contains the same mapping as the kernel
    memcpy(pd, kernelPageDirectory, sizeof(pageDirectory_t));
    pd->physAddr = paging_getPhysAddr(pd->codes);

    return (pd);
}

void paging_destroyUserPageDirectory(pageDirectory_t* pd)
{
    // The kernel's page directory must not be destroyed
    ASSERT(pd != kernelPageDirectory);

    // Free all memory that is not from the kernel
    for (uint32_t i=0; i<1024; i++)
    {
        if (pd->tables[i] && (pd->tables[i] != kernelPageDirectory->tables[i]))
        {
            for (uint32_t j=0; j<1024; j++)
            {
                uint32_t physAddress = pd->tables[i]->pages[j] & 0xFFFFF000;

                if (physAddress)
                {
                    physMemFree(physAddress);
                }
            }
            free(pd->tables[i]);
        }
    }

    free(pd);
}


void* paging_acquirePciMemory(uint32_t physAddress, uint32_t numberOfPages)
{
    static void* virtAddress = PCI_MEM_START;
    void* retVal = virtAddress;
    task_switching  = false;

    for (uint32_t i=0; i<numberOfPages; i++)
    {
        if (virtAddress == PCI_MEM_END)
        {
            textColor(ERROR);
            printf("\nNot enough PCI-memory available");
            textColor(TEXT);
            task_switching = true;
            return (0);
        }

        uint32_t pagenr = (uintptr_t)virtAddress/PAGESIZE;

        // Check the page table and setup the page
        ASSERT(kernelPageDirectory->tables[pagenr/1024]);
        kernelPageDirectory->tables[pagenr/1024]->pages[pagenr%1024] = physAddress | MEM_PRESENT | MEM_WRITE | MEM_KERNEL;

        // next page
        virtAddress += PAGESIZE;
        physAddress += PAGESIZE;
    }

    task_switching = true;
    return (retVal);
}

uintptr_t paging_getPhysAddr(void* virtAddress)
{
    pageDirectory_t* pd = kernelPageDirectory;

    // Find the page table
    uint32_t pageNumber = (uintptr_t)virtAddress / PAGESIZE;
    pageTable_t* pt = pd->tables[pageNumber/1024];

  #ifdef _DIAGNOSIS_
    kdebug(3, "\nvirt-->phys: pagenr: %u ", pagenr);
    kdebug(3, "pt: %Xh\n", pt);
  #endif

    if (pt)
    {
        // Read the address, cut off the flags, append the odd part of the address
        return ( (pt->pages[pageNumber % 1024] & 0xFFFFF000) + ((uintptr_t)virtAddress & 0x00000FFF) );
    }
    else
    {
        return (0); // not mapped
    }
}

static void* lookForVirtAddr(uintptr_t physAddr, uintptr_t start, uintptr_t end)
{
    for (uintptr_t i = start; i < end; i+=PAGESIZE)
    {
        if (paging_getPhysAddr((void*)i) == (physAddr & 0xFFFFF000))
        {
            return (void*)(i + (physAddr & 0x00000FFF));
        }
    }
    return (0); // Not mapped between start and end
}

void* paging_getVirtAddr(uintptr_t physAddress)
{
    // check idendity mapping area
    if (physAddress < IDMAP * 1024 * PAGESIZE)
    {
        return (void*)physAddress;
    }

    // check current used heap
    void* virtAddr = lookForVirtAddr(physAddress, (uintptr_t)KERNEL_HEAP_START, (uintptr_t)heap_getCurrentEnd());
    if(virtAddr)
        return (virtAddr);

    // check pci memory
    lookForVirtAddr(physAddress, (uintptr_t)PCI_MEM_START, (uintptr_t)PCI_MEM_END);
    if(virtAddr)
        return (virtAddr);

    // check between idendity mapping area and heap start
    lookForVirtAddr(physAddress, IDMAP * 1024 * PAGESIZE, (uintptr_t)KERNEL_HEAP_START);
    if(virtAddr)
        return (virtAddr);

    // check between current heap end and theoretical heap end
    lookForVirtAddr(physAddress, (uintptr_t)heap_getCurrentEnd(), (uintptr_t)KERNEL_HEAP_END);
    return (virtAddr);
}


void paging_analyzeBitTable()
{
    uint32_t k = 0, k_old = 2;
    int64_t ramsize;
    ipc_getInt("PrettyOS/RAM", &ramsize);
    uint32_t maximum = min(((uint32_t)ramsize)/PAGESIZE/32, MAX_DWORDS);

    for (uint32_t i=0; i < maximum; i++)
    {
        textColor(TEXT);
        printf("\n%Xh: ", 32 * i * PAGESIZE);

        for (uint32_t offset=0; offset<32; offset++)
        {
            if (!(bittable[i] & BIT(offset)))
            {
                textColor(GREEN);
                putch('0');

                if (offset == 31)
                {
                    k_old = k;
                    k = 0;
                }
            }
            else
            {
                textColor(LIGHT_GRAY);
                putch('1');

                if (offset == 31)
                {
                    k_old = k;
                    k = 1;
                }
            }
        }

        if (k != k_old)
        {
            sleepSeconds(3); // wait a little bit at changes
        }

    }
    textColor(TEXT);
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
