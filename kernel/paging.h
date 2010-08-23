#ifndef PAGING_H
#define PAGING_H

#include "os.h"

#define PAGESIZE            0x1000  // size 
#define MEMORY_MAP_ADDRESS  0x1000  // address

static const uint32_t MEM_PRESENT  = 1;
static const uint32_t MEM_READONLY = 0;
static const uint32_t MEM_KERNEL   = 0;
static const uint32_t MEM_WRITE    = 2;
static const uint32_t MEM_USER     = 4;

// Memory Map
typedef struct
{
    uint64_t base;   // The region's address
    uint64_t size;   // The region's size
    uint32_t type;   // Is "1" for "free"
    uint32_t ext;    // Unimportant for us, but necessary! Do not take out!
} __attribute__((packed)) mem_map_entry_t;

// Paging
typedef struct
{
    uint32_t pages[1024];
} page_table_t;

typedef struct
{
    uint32_t       codes[1024];
    page_table_t* tables[1024];
    uint32_t      pd_phys_addr;
} __attribute__((packed)) page_directory_t;

extern page_directory_t* kernel_pd;

void paging_switch (page_directory_t* pd);
uint32_t paging_install();

bool paging_alloc(page_directory_t* pd, void* virt_addr, uint32_t size, uint32_t flags);
void paging_free (page_directory_t* pd, void* virt_addr, uint32_t size);

page_directory_t* paging_create_user_pd();
void paging_destroy_user_pd(page_directory_t* pd);

void* paging_acquire_pcimem(uint32_t phys_addr, uint32_t numberOfPages);
uint32_t paging_get_phys_addr(void* virt_addr);

#endif
