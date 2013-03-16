#ifndef VM86_H
#define VM86_H

// code derived on basic proposal at http://osdev.berlios.de/v86.html

#include "paging.h"
#include "irq.h"

// eflags
#define VALID_FLAGS  0x3FFFFF
#define EFLAG_IF     BIT(9)
#define EFLAG_VM     BIT(17)
#define FP_TO_LINEAR(seg, off) ((void*) ((((uint16_t) (seg)) << 4) + ((uint16_t) (off))))


bool vm86_sensitiveOpcodehandler(registers_t* ctx);
void vm86_initPageDirectory(pageDirectory_t* pd, void* address, void*data, size_t size);
void vm86_executeSync(pageDirectory_t* pd, void (*entry)(void));


#endif
