#ifndef PAGING_H
#define PAGING_H

#include "os.h"

#define PAGESIZE            0x1000  // size

enum MEM_FLAGS {MEM_KERNEL = 0, MEM_PRESENT = 1, MEM_WRITE = 2, MEM_USER = 4};

// Memory Map
typedef struct
{
    uint32_t ext;    // Unimportant for us, but necessary! Do not take out!
    uint64_t base;   // The region's address
    uint64_t size;   // The region's size
    uint32_t type;   // Is "1" for "free"
} __attribute__((packed)) memoryMapEntry_t;

// Paging
typedef struct
{
    uint32_t pages[1024];
} pageTable_t;

typedef struct
{
    uint32_t     codes[1024];
    pageTable_t* tables[1024];
    uint32_t     physAddr;
} __attribute__((packed)) pageDirectory_t;

extern pageDirectory_t* kernelPageDirectory;
extern void* memoryMapAdress; // Read from multiboot structure

void paging_switch(pageDirectory_t* pd);
uint32_t paging_install();

bool pagingAlloc(pageDirectory_t* pd, void* virtAddress, uint32_t size, uint32_t flags);
void pagingFree (pageDirectory_t* pd, void* virtAddress, uint32_t size);

pageDirectory_t* paging_createUserPageDirectory();
void paging_destroyUserPageDirectory(pageDirectory_t* pd);

void* paging_acquirePciMemory(uint32_t physAddress, uint32_t numberOfPages);
uint32_t paging_getPhysAddr(void* virtAddress);

void paging_analyzeBitTable();

#endif
