#ifndef CPU_H
#define CPU_H

#include "os.h"

typedef enum 
{ // Uses bits larger than 5 because the bitnumber inside the registers can be 31 at maximum which needs the first 5 bits
    CR_EAX = 1 << 5,
    CR_EBX = 1 << 6,
    CR_ECX = 1 << 7,
    CR_EDX = 1 << 8
} CPU_REGISTER;

typedef enum 
{ // In this enum the last five bits contain the bit in the register, the other bits are the register (see above)
    CF_FPU          = CR_EDX|0,
    CF_VM86         = CR_EDX|1,
    CF_IOBREAKPOINT = CR_EDX|2,
    CF_PAGES4MB     = CR_EDX|3,
    CF_RDTSC        = CR_EDX|4,
    CF_MSR          = CR_EDX|5,
    CF_PAE          = CR_EDX|6,
    CF_MCE          = CR_EDX|7,
    CF_X8           = CR_EDX|8,
    CF_APIC         = CR_EDX|9,
    CF_SYSENTEREXIT = CR_EDX|11,
    CF_MTTR         = CR_EDX|12,
    CF_PGE          = CR_EDX|13,
    CF_MCA          = CR_EDX|14,
    CF_CMOV         = CR_EDX|15,
    CF_PAT          = CR_EDX|16,
    CF_PSE36        = CR_EDX|17,
    CF_CLFLUSH      = CR_EDX|19,
    CF_MMX          = CR_EDX|23,
    CF_FXSR         = CR_EDX|24,
    CF_SSE          = CR_EDX|25,
    CF_SSE2         = CR_EDX|26,
    CF_SS           = CR_EDX|27,
    CF_HTT          = CR_EDX|28,
    
    CF_SSE3         = CR_ECX|0,
    CF_MONITOR      = CR_ECX|3,
    CF_SSSE3        = CR_ECX|9,
    CF_CMPXCHG16B   = CR_ECX|13,
    CF_SSE41        = CR_ECX|19,
    CF_POPCNT       = CR_ECX|23
} CPU_FEATURE;


extern char cpu_vendor[13];


void     cpu_analyze();
bool     cpu_supports(CPU_FEATURE feature);
uint32_t cpu_idGetRegister(uint32_t function, CPU_REGISTER reg);

#endif
