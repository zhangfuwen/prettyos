#include "paging.h"
#include "kheap.h"

uint32_t uint32_t_MAX = 0xFFFFFFFF;
page_directory_t* kernel_directory  = 0;
page_directory_t* current_directory = 0;

uint32_t placement_address = 0x200000;
extern heap_t* kheap;

// A bitset of frames - used or free
uint32_t*  frames; // pointer to the bitset (functions: set/clear/test)


uint32_t k_malloc(uint32_t size, uint8_t align, uint32_t* phys)
{
    if( kheap!=0 )
    {
        uint32_t addr = (uint32_t) alloc(size, align, kheap);
        if (phys != 0)
        {
            page_t* page = get_page(addr, 0, kernel_directory);
            *phys = page->frame_addr * PAGESIZE + (addr&0xFFF);
        }

        ///
        #ifdef _DIAGNOSIS_
        settextcolor(3,0);
        printformat("%x ",addr);
        settextcolor(15,0);
        #endif
        ///

        return addr;
    }
    else
    {
        if( !(placement_address == (placement_address & 0xFFFFF000) ) )
        {
            placement_address &= 0xFFFFF000;
            placement_address += PAGESIZE;
        }

        if( phys )
        {
            *phys = placement_address;
        }
        uint32_t temp = placement_address;
        placement_address += size;     // new placement_address is increased

        ///
        #ifdef _DIAGNOSIS_
        settextcolor(9,0);
        printformat("%x ",temp);
        settextcolor(15,0);
        #endif
        ///

        return temp;                   // old placement_address is returned
    }
}

/************* bitset variables and functions **************/
uint32_t ind, offs;

static void get_Index_and_Offset(uint32_t frame_addr)
{
    uint32_t frame    = frame_addr/PAGESIZE;
    ind    = frame/32;
    offs   = frame%32;
}

static void set_frame(uint32_t frame_addr)
{
    get_Index_and_Offset(frame_addr);
    frames[ind] |= (1<<offs);
}

static void clear_frame(uint32_t frame_addr)
{
    get_Index_and_Offset(frame_addr);
    frames[ind] &= ~(1<<offs);
}

/*
static uint32_t test_frame(uint32_t frame_addr)
{
    get_Index_and_Offset(frame_addr);
    return( frames[ind] & (1<<offs) );
}
*/
/***********************************************************/

static uint32_t first_frame() // find the first free frame in frames bitset
{
    uint32_t index, offset;
    for(index=0; index<( (pODA->Memory_Size/PAGESIZE)/32 ); ++index)
    {
        if(frames[index] != uint32_t_MAX)
        {
            for(offset=0; offset<32; ++offset)
            {
                if( !(frames[index] & 1<<offset) ) // bit set to zero?
                    return (index*32 + offset);
            }
        }
    }
    return uint32_t_MAX; // no free page frames
}

void alloc_frame(page_t* page, int32_t is_kernel, int32_t is_writeable) // allocate a frame
{
    if( !(page->frame_addr) )
    {
        uint32_t index = first_frame(); // search first free page frame
        if( index == uint32_t_MAX )
            printformat("message from alloc_frame: no free frames!!! ");

        set_frame(index*PAGESIZE);

        page->present    = 1;
        page->rw         = ( is_writeable == 1 ) ? 1 : 0;
        page->user       = ( is_kernel    == 1 ) ? 0 : 1;
        page->frame_addr = index;
    }
}

void free_frame(page_t* page) // deallocate a frame
{
    if( page->frame_addr )
    {
        clear_frame(page->frame_addr);
        page->frame_addr = 0;
    }
}

void paging_install()
{
    // setup bitset
    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("bitset: ");
    settextcolor(15,0);
    #endif
    ///

    frames = (uint32_t*) k_malloc( (pODA->Memory_Size/PAGESIZE) /32, 0, 0 );
    k_memset( frames, 0, (pODA->Memory_Size/PAGESIZE) /32 );

    // make a kernel page directory
    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("PD: ");
    settextcolor(15,0);
    #endif
    ///

    kernel_directory = (page_directory_t*) k_malloc( sizeof(page_directory_t), 1, 0 );
    k_memset(kernel_directory, 0, sizeof(page_directory_t));
    kernel_directory->physicalAddr = (uint32_t)kernel_directory->tablesPhysical;

    uint32_t i=0;
    // Map some pages in the kernel heap area.
    for( i=KHEAP_START; i<KHEAP_START+KHEAP_INITIAL_SIZE; i+=PAGESIZE )
        get_page(i, 1, kernel_directory);

    // map (phys addr <---> virt addr) from 0x0 to the end of used memory
    // Allocate at least 0x2000 extra, that the kernel heap, tasks, and kernel stacks can be initialized properly
    i=0;
    while( i < placement_address + 0x6000 ) //important to add more!
    {
        if( ((i>=0xb8000) && (i<=0xbf000)) || ((i>=0x17000) && (i<0x18000)) )
        {
            alloc_frame( get_page(i, 1, kernel_directory), US, RW); // user and read-write
        }
        else
        {
            alloc_frame( get_page(i, 1, kernel_directory), SV, RO); // supervisor and read-only
        }
        i += PAGESIZE;
    }

    //Allocate user space
    uint32_t user_space_start = 0x400000;
    uint32_t user_space_end   = 0x600000;
    i=user_space_start;
    while( i <= user_space_end )
    {
        alloc_frame( get_page(i, 1, kernel_directory), US, RW); // user and read-write
        i += PAGESIZE;
    }

    // Now allocate those pages we mapped earlier.
    for( i=KHEAP_START; i<KHEAP_START+KHEAP_INITIAL_SIZE; i+=PAGESIZE )
         alloc_frame( get_page(i, 1, kernel_directory), SV, RW); // supervisor and read/write

    current_directory = clone_directory(kernel_directory);

    // cr3: PDBR (Page Directory Base Register)
    __asm__ volatile("mov %0, %%cr3":: "r"(kernel_directory->physicalAddr)); //set page directory base pointer

    // read cr0, set paging bit, write cr0 back
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0": "=r"(cr0)); // read cr0
    cr0 |= 0x80000000; // set the paging bit in CR0 to enable paging
    __asm__ volatile("mov %0, %%cr0":: "r"(cr0)); // write cr0
}

page_t* get_page(uint32_t address, uint8_t make, page_directory_t* dir)
{
    address /= PAGESIZE;                // address ==> index.
    uint32_t table_index = address / 1024; // ==> page table containing this address

    if(dir->tables[table_index])       // table already assigned
    {
        return &dir->tables[table_index]->pages[address%1024];
    }
    else if(make)
    {
        uint32_t phys;
        ///
        #ifdef _DIAGNOSIS_
        settextcolor(2,0);
        printformat("gp_make: ");
        settextcolor(15,0);
        #endif
        ///

        dir->tables[table_index] = (page_table_t*) k_malloc( sizeof(page_table_t), 1, &phys );
        k_memset(dir->tables[table_index], 0, PAGESIZE);
        dir->tablesPhysical[table_index] = phys | 0x7; // 111b meaning: PRESENT=1, RW=1, USER=1
        return &dir->tables[table_index]->pages[address%1024];
    }
    else
        return 0;
}

static page_table_t *clone_table(page_table_t* src, uint32_t* physAddr)
{
    // Make a new page table, which is page aligned.
    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("clone_PT: ");
    settextcolor(15,0);
    #endif
    ///

    page_table_t *table = (page_table_t*)k_malloc(sizeof(page_table_t),1,physAddr);
    // Ensure that the new table is blank.
    k_memset(table, 0, sizeof(page_directory_t));

    // For every entry in the table...
    int32_t i;
    for(i=0; i<1024; ++i)
    {
        // If the source entry has a frame associated with it...
        if (!src->pages[i].frame_addr)
            continue;
        // Get a new frame.
        alloc_frame(&table->pages[i], 0, 0);
        // Clone the flags from source to destination.
        if (src->pages[i].present) table->pages[i].present = 1;
        if (src->pages[i].rw)      table->pages[i].rw = 1;
        if (src->pages[i].user)    table->pages[i].user = 1;
        if (src->pages[i].accessed)table->pages[i].accessed = 1;
        if (src->pages[i].dirty)   table->pages[i].dirty = 1;

        // Physically copy the data across. This function is in process.s.
        copy_page_physical(src->pages[i].frame_addr*0x1000, table->pages[i].frame_addr*0x1000);
    }
    return table;
}

page_directory_t *clone_directory(page_directory_t *src)
{
    uint32_t phys;
    // Make a new page directory and obtain its physical address.
    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("clone_PD: ");
    settextcolor(15,0);
    #endif
    ///

    page_directory_t *dir = (page_directory_t*) k_malloc( sizeof(page_directory_t),1,&phys );
    // Ensure that it is blank.
    k_memset( dir, 0, sizeof(page_directory_t) );

    // Get the offset of tablesPhysical from the start of the page_directory_t structure.
    uint32_t offset = (uint32_t)dir->tablesPhysical - (uint32_t)dir;

    // Then the physical address of dir->tablesPhysical is:
    dir->physicalAddr = phys + offset;

    // Go through each page table. If the page table is in the kernel directory, do not make a new copy.
    int32_t i;
    for(i=0; i<1024; ++i)
    {
        if (!src->tables[i])
            continue;

        if (kernel_directory->tables[i] == src->tables[i])
        {
            // It's in the kernel, so just use the same pointer.
            dir->tables[i] = src->tables[i];
            dir->tablesPhysical[i] = src->tablesPhysical[i];
        }
        else
        {
            // Copy the table.
            uint32_t phys;
            dir->tables[i] = clone_table(src->tables[i], &phys);
            dir->tablesPhysical[i] = phys | 0x07;
        }
    }
    return dir;
}

void analyze_frames_bitset(uint32_t sec)
{
    uint32_t index, offset, counter1=0, counter2=0;
    for(index=0; index<( (pODA->Memory_Size/PAGESIZE) /32); ++index)
    {
        settextcolor(15,0);
        printformat("\n%x  ",index*32*0x1000);
        ++counter1;
        for(offset=0; offset<32; ++offset)
        {
            if( !(frames[index] & 1<<offset) )
            {
                settextcolor(4,0);
                putch('0');
            }
            else
            {
                settextcolor(2,0);
                putch('1');
                ++counter2;
            }
        }
        if(counter1==24)
        {
            counter1=0;
            if(counter2)
                sleepSeconds(sec);
            counter2=0;
        }
    }
}

uint32_t show_physical_address(uint32_t virtual_address)
{
    page_t* page = get_page(virtual_address, 0, kernel_directory);
    return( (page->frame_addr)*PAGESIZE + (virtual_address&0xFFF) );
}

void analyze_physical_addresses()
{
   int32_t i,j,k=0, k_old;
    for(i=0; i<( (pODA->Memory_Size/PAGESIZE) / 0x18000 + 1 ); ++i)
    {
        for(j=i*0x18000; j<i*0x18000+0x18000; j+=0x1000)
        {
            if(show_physical_address(j)==0)
            {
                settextcolor(4,0);
                k_old=k; k=1;
            }
            else
            {
                if(show_physical_address(j)-j)
                {
                    settextcolor(3,0);
                    k_old=k; k=2;
                }
                else
                {
                    settextcolor(2,0);
                    k_old=k; k=3;
                }
            }
            if(k!=k_old)
                printformat("%x %x\n", j, show_physical_address(j));
        }
    }
}
