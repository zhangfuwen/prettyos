#ifndef KHEAP_H
#define KHEAP_H

#include "os.h"


//// Memory configuration ////
// BE AWARE, THE MEMORY MANAGEMENT CODE STILL HAS SOME HARDCODED
//       ADDRESSES, SO DO NOT CHANGE THESE ONES HERE!

// Where the kernel's private data is stored (virtual addresses)
#define KERNEL_DATA_START ((uint8_t*)0x0C0000000)   // 3 GiB
#define KERNEL_DATA_END   ((uint8_t*)0xE0000000)   // 3,75 GiB

// PCI/EHCI memory location for MMIO
#define PCI_MEM_START     ((uint8_t*)0x0E0000000)
#define PCI_MEM_END       ((uint8_t*)0x100000000)   // 4 GiB

// Virtual adress area for the kernel heap
#define KERNEL_HEAP_START KERNEL_DATA_START
#define KERNEL_HEAP_END   PCI_MEM_START
#define KERNEL_HEAP_SIZE  ((uint8_t*)((uintptr_t)KERNEL_HEAP_END - (uintptr_t)KERNEL_HEAP_START))

// Placement allocation
#define PLACEMENT_BEGIN   ((uint8_t*) 0xA00000)     // 10 MiB // TEST   earlier 16 MiB
#define PLACEMENT_END     ((uint8_t*) 0xE00000)     // 14 MiB // TEST   earlier 20 MiB

// User Heap management
#define USER_HEAP_START   ((uint8_t*)0x1420000)                       // 20 MiB plus 128 KiB
#define USER_HEAP_END     ((uint8_t*)(KERNEL_DATA_START - 0x1000000)) //  3 GiB minus 16 MiB

// User Stack
#define USER_STACK 0x1420000


typedef struct
{
    uint32_t size;
    bool     reserved; 
    char     comment[21];
    uint32_t number;
} region_t;


void  heap_install();
void* malloc(uint32_t size, uint32_t alignment, char* comment);
void  free(void* mem);
void logHeapRegions();

#endif
