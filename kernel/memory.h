#ifndef MEMORY_H
#define MEMORY_H

#include "os.h"

//// Memory configuration ////

/******************************************************************************
*                                                                             *
*                          K E R N E L   A R E A                              *
*                                                                             *
******************************************************************************/

// Kernel is located at 0x100000 // 1 MiB  // cf. kernel.ld

// Placement allocation
#define PLACEMENT_BEGIN   ((uint8_t*) 0x1000000)     // 16 MiB
#define PLACEMENT_END     ((uint8_t*) 0x1400000)     // 20 MiB

// Where the kernel's private data is stored (virtual addresses)
#define KERNEL_DATA_START ((uint8_t*)0x0C0000000)  // 3 GiB
#define KERNEL_DATA_END   ((uint8_t*)0xE0000000)   // 3,75 GiB

// Virtual adress area for the kernel heap
#define KERNEL_heapStart KERNEL_DATA_START
#define KERNEL_heapEnd   PCI_MEM_START
#define KERNEL_heapSize  ((uint8_t*)((uintptr_t)KERNEL_heapEnd - (uintptr_t)KERNEL_heapStart))

// memory location for MMIO of devices (networking card, EHCI, grafics card, ...)
#define PCI_MEM_START     ((uint8_t*)0x0E0000000)
#define PCI_MEM_END       ((uint8_t*)0x100000000)   // 4 GiB


/******************************************************************************
*                                                                             *
*                          U S E R    A R E A                                 *
*                                                                             *
******************************************************************************/

// User prorams start at 0x1400000 // 20 MiB  // cf. user.ld

// User Stack
#define USER_STACK 0x1420000

// User Heap management
#define USER_heapStart   ((uint8_t*)0x1420000)                       // 20 MiB plus 128 KiB
#define USER_heapEnd     ((uint8_t*)(KERNEL_DATA_START - 0x1000000)) //  3 GiB minus 16 MiB

#endif
