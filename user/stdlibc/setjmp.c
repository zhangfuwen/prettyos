#include "setjmp.h"
#include "stdio.h"


// TODO: Fix it.

int setjmp(jmp_buf env)
{
    register int retVal;
    __asm__ volatile("mov %%esp, %0\n"
                     "mov %%ebp, %1\n"
                     "mov -4(%%ebp), %2\n"
                     "mov $label1, %3\n"
                     "mov -8(%%ebp), %4\n"
                     "mov $0, %5\n"
                     "jmp label2\n"
                     "label1:\n"
                     "mov %%eax, %5\n"
                     "label2:\n" : "=r"(env[0].esp), "=r"(env[0].ebp), "=r"(env[0].oldebp), "=r"(env[0].jmpAdress), "=r"(env[0].returnAdress), "=r"(retVal));
    return(retVal);
}

void longjmp(jmp_buf env, int retVal)
{
    __asm__ volatile("mov %0, %%esp\n"
                     "mov %1, %%ebp\n"
                     "mov %2, -4(%%ebp)\n"
                     "mov %4, -8(%%ebp)\n"
                     "mov %5, %%eax\n"
                     "jmp *%3" : : "r"(env[0].esp), "r"(env[0].ebp), "r"(env[0].oldebp), "r"(env[0].jmpAdress), "r"(env[0].returnAdress), "r"(retVal));
}
