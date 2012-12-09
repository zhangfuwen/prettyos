#pragma once
#define __attribute__(a)
#define restrict
#define __asm__
#define volatile()

#ifndef __clang__
#    define __GNUC__ 4
#    define __GNUC_MINOR__ 7 // Assume GCC 4.7 crosscompiler
#endif
