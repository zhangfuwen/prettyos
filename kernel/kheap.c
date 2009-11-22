#include "kheap.h"
#include "paging.h"



/*
The heap provides the malloc/free-functionality, i.e. dynamic allocation of
 memory. It manages a certain amount of continuous virtual memory, starting
 at "heap_start". Whenever more memory is requested than there is available,
 the heap expands.
For expansion, the heap asks the paging module to map physical memory to the
 following virtual addresses and increases it's "heap_size" variable afterwards.

To manage the free and reserved (allocated) areas of the heap an array of
 "region" elements are held. Each region specifies it's size and whether it
 is reserved/allocated. Free regions are always merged. Regions don't store
 their addresses, the third region's address is calculated by adding the first
 and second region's size to "heap_start":
 region_3_addr = heap_start + regions[0].size + regions[1].size.

Before the heap is set up memory is allocated on a "placement address". This 
 is an identity mapped area of continuous memory, the allocation just moves
 a pointer forward by the requested size and returns it's previous value.

The heap's management data is placed at this placement address, too. Since this
 area cannot grow, the heap has a maximum amount of region-objects
 ("region_max_count").
*/



typedef struct
{
    uint32_t size;
    bool     reserved;
} region_t;


static region_t*   regions = NULL;
static uint32_t    region_count = 0;
static uint32_t    region_max_count = 0;
static char* const heap_start = (char*)HEAP_START_ADDRESS;
static uint32_t    heap_size = 0;

static char* const PLACEMENT_BEGIN = (char*)(16*1024*1024);
static char* const PLACEMENT_END   = (char*)(20*1024*1024);

static const uint32_t HEAP_MIN_GROWTH = 256*1024;


void heap_install()
{
    // This gets us the current placement address
    regions = k_malloc( 0, 0 );

    // We take the rest of the placement area
    region_count = 0;
    region_max_count = (PLACEMENT_END-(char*)regions) / sizeof(region_t);
}


static bool heap_grow( uint32_t size, char* heap_end )
{
    // We will have to append another region-object to our array if
    //   we can't merge with the last region - check whether there
    //   would be enough space to insert the region-object
    if ( region_count>0 && regions[region_count-1].reserved && region_count+1>region_max_count )
        return false;

    // Enhance the memory
    if ( ! paging_alloc( NULL, heap_end, size, MEM_KERNEL|MEM_WRITE ) )
        return false;

    // Maybe we can merge with the last region object?
    if ( region_count>0 && !regions[region_count-1].reserved )
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


void* k_malloc( uint32_t size, uint32_t alignment )
{
    // Avoid odd addresses
    size = alignUp( size, 8 );

    // If the heap is not set up..
    if ( regions == NULL )
    {
        // Do simple placement allocation
        static char* addr = PLACEMENT_BEGIN;
        addr = (char*) alignUp( (uint32_t)addr, alignment );
        char* ret = addr;
        addr += size;
        return ret;
    }

    // Walk the regions and find a big-enough one
    char* region_addr = heap_start;
    for ( uint32_t i=0; i<region_count; ++i )
    {
        // Calculate aligned address and the how much more size is needed due to alignment
        char* aligned_addr = (char*)alignUp( (uint32_t)region_addr, alignment );
        uint32_t additional_size = aligned_addr - region_addr;

        // Check whether this region is free and big enough
        if ( !regions[i].reserved && regions[i].size>=(size+additional_size) )
        {
            // We will split up this region...
            // +--------------------------------------------------------+
            // |                      Current Region                    |
            // +--------------------------------------------------------+
            //
            // ...into three, the first and third remain free while the second
            //   gets reserved and its address returned
            //
            // +------------------+--------------------------+----------+
            // | Before Alignment | Aligned Destination Area | Leftover |
            // +------------------+--------------------------+----------+

            // Split the pre-alignment area
            if ( aligned_addr != region_addr )
            {
                // Check whether we are able to expand
                if ( region_count+1 > region_max_count )
                    return NULL;

                // Move all following regions ahead to get room for a new one
                for ( uint32_t j=region_count; j>i; --j )
                    regions[j] = regions[j-1];
                ++region_count;

                // Setup the regions
                regions[i].size = aligned_addr - region_addr;
                regions[i].reserved = false;
                regions[i+1].size -= regions[i].size;

                // "Aligned Destination Area" becomes the "current" region
                region_addr += regions[i].size;
                ++i;
            }

            // Split the leftover
            if ( regions[i].size > (size+additional_size) )
            {
                // Check whether we are able to expand
                if ( region_count+1 > region_max_count )
                    return NULL;

                // Move all following regions ahead to get room for a new one
                for ( uint32_t j=region_count; j>i+1; --j )
                    regions[j] = regions[j-1];
                ++region_count;

                // Setup the regions
                regions[i+1].size = regions[i].size - size;
                regions[i+1].reserved = false;
                regions[i].size = size;
            }

            // Set the region to "reserved" and return its address
            regions[i].reserved = true;
            return region_addr;
        }
        region_addr += regions[i].size;
    }

    // There is nothing free, try to expand the heap
    uint32_t to_grow = max( HEAP_MIN_GROWTH, alignUp(size*3/2,PAGESIZE) );
    if ( ! heap_grow( to_grow, heap_start+heap_size ) )
        return NULL;

    // Now there should be a region that is large enough
    void* address = k_malloc( size, alignment );

    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("%X ",address);
    settextcolor(15,0);
    #endif

    return address;
}


void k_free( void* addr )
{
    // Walk the regions and find the correct one
    char* region_addr = heap_start;
    for ( uint32_t i=0; i<region_count; ++i )
    {
        if ( region_addr == addr )
        {
            regions[i].reserved = false;

            // Check for a merge with the next region
            if ( i+1<region_count && !regions[i+1].reserved )
            {
                // Adjust the size of the now-free region
                regions[i].size += regions[i+1].size;

                // Move all following regions back by one
                for ( uint32_t j=i+1; j<region_count-1; ++j )
                    regions[j] = regions[j+1];
                --region_count;
            }

            // Check for a merge with the previous region
            if ( i>0 && !regions[i-1].reserved )
            {
                // Adjust the size of the previous region
                regions[i-1].size += regions[i].size;

                // Move all following regions back by one
                for ( uint32_t j=i; j<region_count-1; ++j )
                    regions[j] = regions[j+1];
                --region_count;
            }

            // We are done
            return;
        }

        region_addr += regions[i].size;
    }

    printformat( "Broken free: %X\n", addr );
    ASSERT( false );
}
