#ifndef MEMORY_H
#define MEMORY_H

#include "os.h"

/******************************************************************************
*                                                                             *
*                          K E R N E L   A R E A                              *
*                                                                             *
******************************************************************************/

// Kernel is located at 0x100000 // 1 MiB  // cf. kernel.ld

// Placement allocation
#define PLACEMENT_BEGIN   ((uint8_t*) 0x1000000)   // 16 MiB
#define PLACEMENT_END     ((uint8_t*) 0x1400000)   // 20 MiB

// Where the kernel's private data is stored (virtual addresses)
#define KERNEL_DATA_START ((uint8_t*)0xC0000000)   // 3 GiB
#define KERNEL_DATA_END   ((uint8_t*)0xE0000000)   // 3,5 GiB

// Virtual adress area for the kernel heap
#define KERNEL_heapStart KERNEL_DATA_START
#define KERNEL_heapEnd   PCI_MEM_START

// memory location for MMIO of devices (networking card, EHCI, grafics card, ...)
#define PCI_MEM_START     ((uint8_t*)0xE0000000)
#define PCI_MEM_END       ((uint8_t*)0xFFFFFFFF)   // 4 GiB - 1


/******************************************************************************
*                                                                             *
*                          U S E R    A R E A                                 *
*                                                                             *
******************************************************************************/

// User program starts at 0x1400000 // 20 MiB  // cf. user.ld

// User Stack
#define USER_STACK        0x1500000 // nearly 1 MiB

// Area to move data from kernel to userprogram (for example, parameter lists)
#define USER_DATA_BUFFER  USER_STACK  // 64 KiB

// User Heap management
#define USER_heapStart    ((uint8_t*)(USER_DATA_BUFFER  + 0x10000))   // 21 MiB plus 64 KiB
#define USER_heapEnd      ((uint8_t*)(KERNEL_DATA_START - 0x1000000)) //  3 GiB minus 16 MiB


#endif
