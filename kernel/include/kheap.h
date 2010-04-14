#ifndef KHEAP_H
#define KHEAP_H

#include "os.h"


//// Memory configuration ////
// BE AWARE, THE MEMORY MANAGEMENT CODE STILL HAS SOME HARDCODED
//       ADDRESSES, SO DO NOT CHANGE THESE ONES HERE!

// Where the kernel's private data is stored (virtual addresses)
#define KERNEL_DATA_START ((uint8_t*)0x0C0000000)   // 3 GB
#define KERNEL_DATA_END   ((uint8_t*)0x100000000)   // 4 GB

// PCI/EHCI memory location for MMIO
#define PCI_MEM_START     ((uint8_t*)0x0FF000000)
#define PCI_MEM_END       ((uint8_t*)0x100000000)   // 4 GB

// Virtual adress area for the kernel heap
#define KERNEL_HEAP_START KERNEL_DATA_START
#define KERNEL_HEAP_END   PCI_MEM_START
#define KERNEL_HEAP_SIZE  ((uint8_t*)((uintptr_t)KERNEL_HEAP_END - (uintptr_t)KERNEL_HEAP_START))

// Placement allocation
#define PLACEMENT_BEGIN   ((uint8_t*)0x1000000)     // 16 MB
#define PLACEMENT_END     ((uint8_t*)0x1400000)     // 20 MB

// User Heap management
#define USER_HEAP_START   ((uint8_t*)0x1400000)     // 20 MB
#define USER_HEAP_END     ((uint8_t*)(KERNEL_DATA_START - 0x1000000)) // 3 GB minus 16 MB

// User Stack
#define USER_STACK 0x1420000


typedef struct
{
    uint32_t size;
    bool     reserved;
} region_t;


void heap_install();
void* malloc( uint32_t size, uint32_t alignment );
void free( void* mem );


#endif
