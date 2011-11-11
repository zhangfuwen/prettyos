#ifndef PAGING_H
#define PAGING_H

#include "util/util.h"

#define PAGESIZE 0x1000 // size


typedef enum
{
    MEM_KERNEL = 0, MEM_PRESENT = 1, MEM_WRITE = 2, MEM_USER = 4,
    MEM_WRITETHROUGH = BIT(3), MEM_NOCACHE = BIT(4), MEM_NOTLBUPDATE = BIT(8)
} MEMFLAGS_t;


// Memory Map
typedef struct
{
    uint32_t mysize; // Size of this entry
    uint64_t base;   // The region's address
    uint64_t size;   // The region's size
    uint32_t type;   // Is "1" for "free"
} __attribute__((packed)) memoryMapEntry_t;

// Paging
typedef struct
{
    uint32_t pages[1024];
} __attribute__((packed)) pageTable_t;

typedef struct
{
    uint32_t     codes[1024];
    pageTable_t* tables[1024];
    uint32_t     physAddr;
} __attribute__((packed)) pageDirectory_t;


extern pageDirectory_t* kernelPageDirectory;
extern memoryMapEntry_t* memoryMapAdress;
extern memoryMapEntry_t* memoryMapEnd; // Read from multiboot structure


uint32_t paging_install();

bool  paging_alloc(pageDirectory_t* pd, void* virtAddress, uint32_t size, MEMFLAGS_t flags);
void  paging_free (pageDirectory_t* pd, void* virtAddress, uint32_t size);
void* paging_acquirePciMemory(uint32_t physAddress, uint32_t numberOfPages);

pageDirectory_t* paging_createUserPageDirectory();
void             paging_destroyUserPageDirectory(pageDirectory_t* pd);
void             paging_switch(pageDirectory_t* pd);

uintptr_t paging_getPhysAddr(void*     virtAddress);
void*     paging_getVirtAddr(uintptr_t physAddress);

void      paging_analyzeBitTable();


#endif
