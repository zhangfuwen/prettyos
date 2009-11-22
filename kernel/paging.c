#include "paging.h"
#include "kheap.h"




// Memory Map //
typedef struct
{
    uint64_t base;   // The region's address
    uint64_t size;   // The region's size
    uint32_t type;   // Is "1" for "free"
    uint32_t ext;    // Unimportant for us
} __attribute__((packed)) mem_map_entry_t;


static bool memorymap_availability( const mem_map_entry_t* entries, uint64_t beg, uint64_t end )
{
    // There must not be an "reserved" entry which reaches into the specified area
    for ( const mem_map_entry_t* entry=entries; entry->size; ++entry )
        if ( entry->type!=1 && entry->base<end && (entry->base+entry->size)>beg )
            return false;

    // Check whether the "free" entries cover the whole specified area.
    uint64_t covered = beg;
    for ( const mem_map_entry_t* outer_loop=entries; outer_loop->size; ++outer_loop )
        for ( const mem_map_entry_t* entry=entries; entry->size; ++entry )
            if ( entry->base<=covered && (entry->base+entry->size)>covered )
                covered = entry->base + entry->size;

    // Return whether the whole area is covered by "free" entries
    return covered >= end;
}




// Physical Memory //
static uint32_t* data;
static uint32_t  dword_count;
static uint32_t  first_free_dword;


static void phys_set_bits( uint32_t addr_begin, uint32_t addr_end, bool reserved )
{
    // Calculate the bit-numbers
    uint32_t start = alignUp( addr_begin, PAGESIZE ) / PAGESIZE;
    uint32_t end = alignDown( addr_end, PAGESIZE ) / PAGESIZE;

    // Set all these bits
    for ( uint32_t j=start; j<end; ++j )
        if ( reserved )  data[j/32] |= 1<<(j%32);
        else             data[j/32] &= ~(1<<(j%32));
}


static uint32_t phys_init()
{
    mem_map_entry_t* const entries = (mem_map_entry_t*)MEMORY_MAP_ADDRESS;

    // Some constants
    const uint64_t FOUR_GB    = 0x100000000ull;
    const uint32_t MAX_DWORDS = FOUR_GB / PAGESIZE / 32;

    // Print the memory map
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat( "Memory map:\n" );
    for ( mem_map_entry_t* entry=entries; entry->size; ++entry )
        printformat( "  %X -> %X %i\n", (uint32_t)(entry->base), (uint32_t)(entry->base+entry->size), entry->type );
    settextcolor(15,0);
    #endif
    //

    // Prepare the memory map entries, since we work with max
    //   4 GB only. The last entry in the entry-array has size==0.
    for ( mem_map_entry_t* entry=entries; entry->size; ++entry )
    {
        // We will completely ignore memory above 4 GB, move following
        //   entries backward by one then
        if ( entry->base >= FOUR_GB )
            for ( mem_map_entry_t* e=entry; e->size; ++e )
                *e = *(e+1);

        // Eventually reduce the size so the the block doesn't exceed 4 GB
        else if ( entry->base + entry->size >= FOUR_GB )
            entry->size = FOUR_GB - entry->base;
    }

    // Check that 16MB-20MB is free for use
    if ( ! memorymap_availability( entries, 16*1024*1024, 20*1024*1024 ) )
    {
        printformat( "The memory between 16 MB and 20 MB is not free for use\n" );
        for(;;);
    }

    // We store our data here, initialize all bits to "reserved"
    data = k_malloc( 128*1024, 0 );
    for ( uint32_t i=0; i<MAX_DWORDS; ++i )
        data[i] = 0xFFFFFFFF;

    // Set the bitmap bits according to the memory map now. "type==1" means "free".
    for ( mem_map_entry_t* entry=entries; entry->size; ++entry )
        if ( entry->type == 1 )
            phys_set_bits( entry->base, entry->base+entry->size, false );
    for ( mem_map_entry_t* entry=entries; entry->size; ++entry )
        if ( entry->type != 1 )
            phys_set_bits( entry->base, entry->base+entry->size, true );

    // Reserve first 20 MB and the region of our kernel code
    phys_set_bits( 0x00000000, 20*1024*1024, true );

    extern char _kernel_beg, _kernel_end;
    phys_set_bits( (uint32_t)&_kernel_beg, (uint32_t)&_kernel_end, true );

    // Find the number of dwords we can use, skipping the last, "reserved"-only ones
    for ( uint32_t i=0; i<MAX_DWORDS; ++i )
        if ( data[i] != 0xFFFFFFFF )
            dword_count = i+1;

    // Exclude the first 16 MB from being allocated (they'll be needed for DMA later on)
    first_free_dword = 16*1024*1024 / PAGESIZE / 32;

    // Return the amount of memory available (or rather the highest address)
    return dword_count*32*4096;
}


static uint32_t phys_alloc()
{
    // Search for a free uint32_t, i.e. one that not only contains ones
    for ( ; first_free_dword<dword_count; ++first_free_dword )
        if ( data[first_free_dword] != 0xFFFFFFFF )
        {
            // Find the number of a free bit
            uint32_t val = data[first_free_dword];
            uint32_t bitnr = 0;
            while ( val & 1 )
                val>>=1, ++bitnr;

            // Set the page to "reserved" and return the frame's address
            data[first_free_dword] |= 1<<(bitnr%32);
            return (first_free_dword*32+bitnr) * PAGESIZE;
        }

    // No free page found
    return NULL;
}


static void phys_free( uint32_t addr )
{
    // Calculate the number of the bit
    uint32_t bitnr = addr / PAGESIZE;

    // Maybe the affected dword (which has a free bit now) is ahead of first_free_dword?
    if ( bitnr/32 < first_free_dword )
        first_free_dword = bitnr/32;

    // Set the page to "free"
    data[bitnr/32] &= ~(1<<(bitnr%32));
}





// Paging //
typedef struct
{
    uint32_t pages[1024];
} page_table_t;

struct page_directory_
{
    uint32_t       codes[1024];
    page_table_t* tables[1024];
    uint32_t       pd_phys_addr;
} __attribute__((packed));


static const uint32_t MEM_PRESENT = 0x01;
static page_directory_t* kernel_pd;


static uint32_t paging_get_phys_addr( page_directory_t* pd, void* virt_addr )
{
    // Find the page table
    uint32_t pagenr = (uint32_t)virt_addr / PAGESIZE;
    page_table_t* pt = pd->tables[pagenr/1024];
    ASSERT( pt != 0 );

    // Read the address, cut off the flags, append the address' odd part
    return (pt->pages[pagenr%1024]&0xFFFF000) + (((uint32_t)virt_addr)&0x00000FFF);
}


uint32_t paging_install()
{
    uint32_t ram_available = phys_init();

    //// Setup the kernel page directory
    kernel_pd = k_malloc( sizeof(page_directory_t), PAGESIZE );
    k_memset( kernel_pd, 0, sizeof(page_directory_t) );
    kernel_pd->pd_phys_addr = (uint32_t)kernel_pd;

    // Setup the page tables for 0MB-20MB, identity mapping
    uint32_t addr = 0;
    for ( int i=0; i<5; ++i )
    {
        // Page directory entry, virt=phys due to placement allocation in id-mapped area
        kernel_pd->tables[i] = k_malloc( sizeof(page_table_t), PAGESIZE );
        kernel_pd->codes[i] = (uint32_t)kernel_pd->tables[i] | MEM_PRESENT;

        // Page table entries, identity mapping
        for ( int j=0; j<1024; ++j )
        {
            kernel_pd->tables[i]->pages[j] = addr | MEM_PRESENT;
            addr += PAGESIZE;
        }
    }

    // Set the page which covers the video area (0xB8000) to writeable
    kernel_pd->codes[0] |= MEM_USER | MEM_WRITE;
    kernel_pd->tables[0]->pages[184] |= MEM_USER | MEM_WRITE;

    // Setup the page tables for the kernel heap (3GB-4GB), unmapped
    page_table_t* heap_pts = k_malloc( 256*sizeof(page_table_t), PAGESIZE );
    k_memset( heap_pts, 0, 256*sizeof(page_table_t) );
    for ( uint32_t i=0; i<256; ++i )
    {
        kernel_pd->tables[768+i] = heap_pts;
        kernel_pd->codes[768+i] = (uint32_t)kernel_pd->tables[768+i] | MEM_PRESENT | MEM_WRITE;
        heap_pts += sizeof(page_table_t);
    }

    // Setup 0x00400000 to 0x00600000 as writeable userspace
    kernel_pd->codes[1] |= MEM_USER | MEM_WRITE;
    for ( int i=0; i<512; ++i )
        kernel_pd->tables[1]->pages[i] |= MEM_USER | MEM_WRITE;

    // Enable paging
    paging_switch( kernel_pd );
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0": "=r"(cr0)); // read cr0
    cr0 |= 0x80000000; // set the paging bit in CR0 to enable paging
    __asm__ volatile("mov %0, %%cr0":: "r"(cr0)); // write cr0

    return ram_available;
}


bool paging_alloc( page_directory_t* pd, void* addr, uint32_t size, uint32_t flags )
{
    // "addr" and "size" must be page-aligned
    ASSERT( ((uint32_t)addr)%PAGESIZE == 0 );
    ASSERT( size%PAGESIZE == 0 );

    // "pd" may be NULL in which case its the kernel's pd
    if ( ! pd )
        pd = kernel_pd;

    // We repeat allocating one page at once
    for ( uint32_t done=0; done!=size/PAGESIZE; ++done )
    {
        uint32_t pagenr = (uint32_t)addr/PAGESIZE + done;

        // Maybe there is already memory allocated?
        if ( pd->tables[pagenr/1024] && pd->tables[pagenr/1024]->pages[pagenr%1024] )
            continue;

        // Allocate physical memory
        uint32_t phys = phys_alloc();
        if ( phys == NULL )
        {
            // Undo the allocations and return an error
            paging_free( pd, addr, done*PAGESIZE );
            return false;
        }

        // Get the page table
        page_table_t* pt = pd->tables[pagenr/1024];
        if ( ! pt )
        {
            // Allocate the page table
            pt = (page_table_t*) k_malloc( sizeof(page_table_t), PAGESIZE );
            if ( ! pt )
            {
                // Undo the allocations and return an error
                phys_free( phys );
                paging_free( pd, addr, done*PAGESIZE );
                return false;
            }
            k_memset( pt, 0, sizeof(page_table_t) );
            pd->tables[pagenr/1024] = pt;

            // Set physical address and flags
            pd->codes[pagenr/1024] = paging_get_phys_addr(kernel_pd,pt) | MEM_PRESENT | MEM_WRITE | (flags&MEM_USER? MEM_USER : 0);
        }

        // Setup the page
        pt->pages[pagenr%1024] = phys | flags | MEM_PRESENT;
    }
    return true;
}


void paging_free( page_directory_t* pd, void* addr, uint32_t size )
{
    // "addr" and "size" must be page-aligned
    ASSERT( ((uint32_t)addr)%PAGESIZE == 0 );
    ASSERT( size%PAGESIZE == 0 );

    // "pd" may be NULL in which case the kernel's pd is meant
    if ( ! pd )
        pd = kernel_pd;

    // Go through all pages and free them
    uint32_t pagenr = (uint32_t)addr / PAGESIZE;
    while ( size )
    {
        // Get the physical address and invalidate the page
        uint32_t* page = &pd->tables[pagenr/1024]->pages[pagenr%1024];
        uint32_t phys_addr = *page & 0xFFFFF000;
        *page = 0;

        // Free memory and adjust variables for next loop run
        phys_free( phys_addr );
        size -= PAGESIZE;
        ++pagenr;
    }
}


page_directory_t* paging_create_user_pd()
{
    // Allocate memory for the page directory
    page_directory_t* pd = (page_directory_t*) k_malloc( sizeof(page_directory_t), PAGESIZE );
    if ( ! pd )
        return NULL;

    // Each user's page directory contains the same mapping as the kernel
    k_memcpy( pd, kernel_pd, sizeof(page_directory_t) );

    pd->pd_phys_addr = paging_get_phys_addr( kernel_pd, pd->codes );
    return pd;
}


void paging_destroy_user_pd( page_directory_t* pd )
{
    // Free all memory that is not from the kernel
    for ( uint32_t i=0; i<1024; ++i )
        if ( pd->tables[i] && pd->tables[i]!=kernel_pd->tables[i] )
        {
            for ( uint32_t j=0; j<1024; ++j )
            {
                uint32_t phys_addr = pd->tables[i]->pages[j] & 0xFFFFF000;
                if ( phys_addr )
                    phys_free( phys_addr );
            }
            k_free( pd->tables[i] );
        }

    k_free( pd );
}


void paging_switch( page_directory_t* pd  )
{
    if(!pd)  pd=kernel_pd;
    __asm__ volatile("mov %0, %%cr3" : : "r" (pd->pd_phys_addr));
}
