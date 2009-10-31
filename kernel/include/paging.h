#ifndef PAGING_H
#define PAGING_H

#include "os.h"

#define PAGESIZE        0x00001000           // 0x1000 = 4096 = 4K

#define SV   1   // supervisor
#define US   0   // user

#define RW   1   // read-write
#define RO   0   // read-only

/************************************************************************/
// page_directory ==> page_table ==> page

typedef struct page
{
    uint32_t present    :  1;   // Page present in memory
    uint32_t rw         :  1;   // 0: read-only,    1: read/write
    uint32_t user       :  1;   // 0: kernel level, 1: user level
    uint32_t accessed   :  1;   // Has the page been accessed since last refresh?
    uint32_t dirty      :  1;   // Has the page been written to since last refresh?
    uint32_t unused     :  7;   // Combination of unused and reserved bits
    uint32_t frame_addr : 20;   // Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table
{
    page_t pages[1024];
} page_table_t;

typedef struct page_directory
{
    page_table_t* tables[1024];
    uint32_t tablesPhysical[1024];
    uint32_t physicalAddr;
} page_directory_t;
/************************************************************************/

void              paging_install();

page_t*           get_page(uint32_t address, uint8_t make, page_directory_t* dir);

void              alloc_frame(page_t *page, int32_t is_kernel, int32_t is_writeable);
void              free_frame (page_t *page);

uint32_t             k_malloc(uint32_t sz, uint8_t align, uint32_t* phys);
uint32_t             k_malloc_stack(uint32_t sz, uint8_t align, uint32_t* phys);

page_directory_t* clone_directory(page_directory_t *src);

uint32_t show_physical_address(uint32_t virtual_address);

extern void       copy_page_physical(uint32_t source_address, uint32_t dest_address); //process.asm

#endif
