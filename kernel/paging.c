/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// TODO: Apply multithreading safety

#include "paging.h"
#include "util.h"
#include "kheap.h"
#include "console.h"

page_directory_t* kernel_pd;

// Memory Map
extern char _kernel_beg, _kernel_end; // defined in linker script

// Physical Memory
static const uint32_t MAX_DWORDS = 0x100000000ull / PAGESIZE / 32;
static uint32_t* bittable;
static uint32_t  first_free_dword;
static uint32_t phys_init();

uint32_t paging_install()
{
    uint32_t ram_available = phys_init();

    // Setup the kernel page directory
    kernel_pd = malloc(sizeof(page_directory_t), PAGESIZE, "pag-kernelPD");
    memset(kernel_pd, 0, sizeof(page_directory_t));
    kernel_pd->pd_phys_addr = (uint32_t)kernel_pd;

    kdebug(3, "\nkernel_pd (virt.): %X ",kernel_pd);
    kdebug(3, "kernel_pd (phys.): %X\n",kernel_pd->pd_phys_addr);

    // Setup the page tables for 0 MiB - 20 MiB, identity mapping
    uint32_t addr = 0;
    for (int i=0; i<5; ++i)
    {
        // Page directory entry, virt=phys due to placement allocation in id-mapped area
        kernel_pd->tables[i] = malloc(sizeof(page_table_t), PAGESIZE, "pag-kernelPT");
        kernel_pd->codes[i] = (uint32_t)kernel_pd->tables[i] | MEM_PRESENT;

        // Page table entries, identity mapping
        for (int j=0; j<1024; ++j)
        {
            kernel_pd->tables[i]->pages[j] = addr | MEM_PRESENT;
            addr += PAGESIZE;
        }
    }

    // Set the page which covers the video area (0xB8000) to writeable
    // kernel_pd->codes[0]               |= MEM_USER | MEM_WRITE;
    // kernel_pd->tables[0]->pages[0xB8] |= MEM_USER | MEM_WRITE; // 184 * 0x1000 = 0xB8000

    // --------------------- VM86 Pages -------------------------------------------------------------------------------
    kernel_pd->codes[0]               |= MEM_USER | MEM_WRITE;
    kernel_pd->tables[0]->pages[0x00] |= MEM_USER | MEM_WRITE; // 0 * 0x1000 = 0x0000
    for (uint32_t i=0; i<96; ++i)
    {
        kernel_pd->tables[0]->pages[0xA0+i] |= MEM_USER | MEM_WRITE; // 0xA0 * 0x1000 = 0xA0000 (until 0xFFFFF)
    }
    // --------------------- VM86 Pages -------------------------------------------------------------------------------

    // Setup the page tables for the kernel heap (3GB-4GB), unmapped
    page_table_t* heap_pts = malloc(256*sizeof(page_table_t), PAGESIZE, "pag-PTheap");
    memset(heap_pts, 0, 256*sizeof(page_table_t));
    for (uint32_t i=0; i<256; ++i)
    {
        kernel_pd->tables[0x300+i] = &heap_pts[i];
        kernel_pd->codes[0x300+i]  = (uint32_t)kernel_pd->tables[0x300+i] | MEM_PRESENT | MEM_WRITE;
    }

    // Tell CPU to enable paging
    paging_switch (kernel_pd);
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0": "=r"(cr0)); // read cr0
    cr0 |= 0x80000000;                            // set the paging bit in CR0
    __asm__ volatile("mov %0, %%cr0":: "r"(cr0)); // write cr0

    return ram_available;
}

uint32_t paging_get_phys_addr(page_directory_t* pd, void* virt_addr)
{
    // Find the page table
    uint32_t pagenr = (uint32_t)virt_addr / PAGESIZE;
    page_table_t* pt = pd->tables[pagenr/1024];

    kdebug(3, "\nvirt-->phys: pagenr: %u ",pagenr);
    kdebug(3, "pt: %X\n",pt);

    ASSERT(pt);

    // Read the address, cut off the flags, append the address' odd part
    return (pt->pages[pagenr%1024]&0xFFFF000) + (((uint32_t)virt_addr)&0x00000FFF);
}

static bool memorymap_availability(const mem_map_entry_t* entries, uint64_t beg, uint64_t end)
{
    // There must not be an "reserved" entry which reaches into the specified area
    for (const mem_map_entry_t* entry=entries; entry->size; ++entry)
    {
        if (entry->type!=1 && entry->base<end && (entry->base+entry->size)>beg)
        {
            return false;
        }
    }

    // Check whether the "free" entries cover the whole specified area.
    uint64_t covered = beg;
    for (const mem_map_entry_t* outer_loop=entries; outer_loop->size; ++outer_loop)
    {
        for (const mem_map_entry_t* entry=entries; entry->size; ++entry)
        {
            if (entry->base<=covered && (entry->base+entry->size)>covered)
            {
                covered = entry->base + entry->size;
            }
        }
    }

    // Return whether the whole area is covered by "free" entries
    return covered >= end;
}

static void phys_set_bits(uint32_t addr_begin, uint32_t addr_end, bool reserved)
{
    // Calculate the bit-numbers
    uint32_t start = alignUp   (addr_begin, PAGESIZE) / PAGESIZE;
    uint32_t end   = alignDown (addr_end,   PAGESIZE) / PAGESIZE;

    // Set all these bits
    for (uint32_t j=start; j<end; ++j)
    {
        if (reserved)
        {
            bittable[j/32] |= 1<<(j%32);
        }
        else
        {
            bittable[j/32] &= ~(1<<(j%32));
        }
    }
}

static uint32_t phys_init()
{
    static const uint64_t FOUR_GB  = 0x100000000ull;
    mem_map_entry_t* const entries = (mem_map_entry_t*)MEMORY_MAP_ADDRESS;

    // Print the memory map
    kdebug(3, "Memory map:\n");
    for (mem_map_entry_t* entry=entries; entry->size; ++entry)
    {
        kdebug(3, "  %X -> %X %i\n", (uint32_t)(entry->base), (uint32_t)(entry->base+entry->size), entry->type);
    }

    // Prepare the memory map entries, since we work with max 4 GB only. The last entry in the entry-array has size 0.
    for (mem_map_entry_t* entry=entries; entry->size; ++entry)
    {
        // We will completely ignore memory above 4 GB, move following entries backward by one then
        if (entry->base >= FOUR_GB)
        {
            for (mem_map_entry_t* e=entry; e->size; ++e)
            {
                *e = *(e+1);
            }
        }

        // Eventually reduce the size so the the block doesn't exceed 4 GB
        else if (entry->base + entry->size >= FOUR_GB)
        {
            entry->size = FOUR_GB - entry->base;
        }
    }

    // Check that 10 MiB-20 MiB is free for use
    if (!memorymap_availability(entries, 10*1024*1024, 20*1024*1024))
    {
        textColor(0x0C);
        printf("The memory between 10 MiB and 20 MiB is not free for use. OS halted!\n");
        for (;;);
    }

    // We store our data here, initialize all bits to "reserved"
    bittable = malloc(128*1024, 0, "pag-bittable");
    for (uint32_t i=0; i<MAX_DWORDS; ++i)
    {
        bittable[i] = 0xFFFFFFFF;
    }

    // Set the bitmap bits according to the memory map now. "type==1" means "free".
    for (mem_map_entry_t* entry=entries; entry->size; ++entry)
    {
        phys_set_bits(entry->base, entry->base+entry->size, !entry->type);
    }

    // Reserve first 20 MiB and the region of our kernel code
    phys_set_bits(0x00000000, 20*1024*1024, true);

    phys_set_bits((uint32_t)&_kernel_beg, (uint32_t)&_kernel_end, true);

    // Find the number of dwords we can use, skipping the last, "reserved"-only ones
    uint32_t dword_count = 0;
    for (uint32_t i=0; i<MAX_DWORDS; ++i)
    {
        if (bittable[i] != 0xFFFFFFFF)
        {
            dword_count = i+1;
        }
    }

    // Exclude the first 10 MiB from being allocated (they'll be needed for DMA later on)
    first_free_dword = 10*1024*1024 / PAGESIZE / 32;

    kdebug(3, "Highest available RAM: %X\n", dword_count*32*4096);

    // Return the amount of memory available (or rather the highest address)
    return dword_count*32*4096;
}

static uint32_t phys_alloc()
{
    // Search for a free uint32_t, i.e. one that not only contains ones
    for (; first_free_dword<MAX_DWORDS; ++first_free_dword)
    {
        if (bittable[first_free_dword] != 0xFFFFFFFF)
        {
            // Find the number of a free bit
            uint32_t val = bittable[first_free_dword];
            uint32_t bitnr = 0;
            while (val & 1)
            {
                val>>=1, ++bitnr;
            }

            // Set the page to "reserved" and return the frame's address
            bittable[first_free_dword] |= 1<<(bitnr%32);
            return (first_free_dword*32+bitnr) * PAGESIZE;
        }
    }
    // No free page found
    return 0;
}

static void phys_free(uint32_t addr)
{
    // Calculate the number of the bit
    uint32_t bitnr = addr / PAGESIZE;

    // Maybe the affected dword (which has a free bit now) is ahead of first_free_dword?
    if (bitnr/32 < first_free_dword)
    {
        first_free_dword = bitnr/32;
    }

    // Set the page to "free"
    bittable[bitnr/32] &= ~(1<<(bitnr%32));
}

bool paging_alloc(page_directory_t* pd, void* virt_addr, uint32_t size, uint32_t flags)
{
    // "virt_addr" and "size" must be page-aligned
    ASSERT(((uint32_t)virt_addr) % PAGESIZE == 0);
    ASSERT(size % PAGESIZE == 0);

    // We repeat allocating one page at once
    for (uint32_t done=0; done!=size/PAGESIZE; ++done)
    {
        uint32_t pagenr = (uint32_t)virt_addr/PAGESIZE + done;

        // Maybe there is already memory allocated?

        if (pd->tables[pagenr/1024] && pd->tables[pagenr/1024]->pages[pagenr%1024])
        {
            kdebug(3, "pagenumber already allocated: %u\n",pagenr);
            continue;
        }

        // Allocate physical memory
        uint32_t phys = phys_alloc();
        if (phys == 0)
        {
            // Undo the allocations and return an error
            paging_free(pd, virt_addr, done*PAGESIZE);
            return false;
        }

        // Get the page table
        page_table_t* pt = pd->tables[pagenr/1024];
        if (!pt)
        {
            // Allocate the page table
            pt = (page_table_t*) malloc(sizeof(page_table_t), PAGESIZE, "pag-PT");
            if (!pt)
            {
                // Undo the allocations and return an error
                phys_free(phys);
                paging_free(pd, virt_addr, done*PAGESIZE);
                return false;
            }
            memset(pt, 0, sizeof(page_table_t));
            pd->tables[pagenr/1024] = pt;

            // Set physical address and flags
            pd->codes[pagenr/1024] = paging_get_phys_addr(kernel_pd,pt) | MEM_PRESENT | MEM_WRITE | (flags&MEM_USER? MEM_USER : 0);
        }

        // Setup the page
        pt->pages[pagenr%1024] = phys | flags | MEM_PRESENT;

        if (flags & MEM_USER)
        {
            kdebug(3, "pagenumber now allocated: %u phys: %X\n",pagenr,phys);
        }
    }
    return true;
}

void paging_free(page_directory_t* pd, void* virt_addr, uint32_t size)
{
    // "virt_addr" and "size" must be page-aligned
    ASSERT(((uint32_t)virt_addr) % PAGESIZE == 0);
    ASSERT(size % PAGESIZE == 0);

    // Go through all pages and free them
    uint32_t pagenr = (uint32_t)virt_addr / PAGESIZE;
    while (size)
    {
        // Get the physical address and invalidate the page
        uint32_t* page = &pd->tables[pagenr/1024]->pages[pagenr%1024];
        uint32_t phys_addr = *page & 0xFFFFF000;
        *page = 0;

        // Free memory and adjust variables for next loop run
        phys_free(phys_addr);
        size -= PAGESIZE;
        ++pagenr;
    }
}

page_directory_t* paging_create_user_pd()
{
    // Allocate memory for the page directory
    page_directory_t* pd = (page_directory_t*) malloc(sizeof(page_directory_t), PAGESIZE,"pag-userPD");
    if (!pd)
    {
        return NULL;
    }

    // Each user's page directory contains the same mapping as the kernel
    memcpy(pd, kernel_pd, sizeof(page_directory_t));
    pd->pd_phys_addr = paging_get_phys_addr(kernel_pd, pd->codes);

    return pd;
}

void paging_destroy_user_pd(page_directory_t* pd)
{
    // The kernel's page directory must not be destroyed
    ASSERT(pd != kernel_pd);

    // Free all memory that is not from the kernel
    for (uint32_t i=0; i<1024; ++i)
    {
        if (pd->tables[i] && pd->tables[i]!=kernel_pd->tables[i])
        {
            for (uint32_t j=0; j<1024; ++j)
            {
                uint32_t phys_addr = pd->tables[i]->pages[j] & 0xFFFFF000;

                if (phys_addr)
                {
                    phys_free(phys_addr);
                }
            }
            free(pd->tables[i]);
        }
    }

    free(pd);
}

void paging_switch (page_directory_t* pd)
{
    __asm__ volatile("mov %0, %%cr3" : : "r" (pd->pd_phys_addr));
}

void* paging_acquire_pcimem(uint32_t phys_addr)
{
    static uint8_t* virt = PCI_MEM_START;

    if (virt == PCI_MEM_END)
    {
        textColor(0x0C);
        panic_assert(__FILE__, __LINE__, "\nNot enough PCI-memory available");
        textColor(0x0F);
    }

    uint32_t pagenr = (uint32_t)virt/PAGESIZE;

    // Check the page table and setup the page
    ASSERT(kernel_pd->tables[pagenr/1024]);
    kernel_pd->tables[pagenr/1024]->pages[pagenr%1024] = phys_addr | MEM_PRESENT | MEM_WRITE | MEM_KERNEL;

    uint8_t* ret = virt;
    virt += PAGESIZE;
    return ret;
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
