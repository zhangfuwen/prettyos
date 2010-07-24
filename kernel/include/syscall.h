#ifndef SYSCALL_H
#define SYSCALL_H

#include "os.h"

void syscall_install();
void syscall_handler(registers_t* regs);

#endif
