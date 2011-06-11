#ifndef VM86_H
#define VM86_H

// code derived on basic proposal at http://osdev.berlios.de/v86.html

#include "os.h"

// eflags
#define VALID_FLAGS  0x3FFFFF
#define EFLAG_IF     BIT(9)
#define EFLAG_VM     BIT(17)
#define FP_TO_LINEAR(seg, off) ((void*) ((((uint16_t) (seg)) << 4) + ((uint16_t) (off))))

bool vm86sensitiveOpcodehandler(registers_t* ctx);

#endif
