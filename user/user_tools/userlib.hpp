#ifndef USERLIB_HPP
#define USERLIB_HPP

#define _cplusplus

extern "C" {
#include "userlib.h"

void* operator new      (long unsigned int size);
void* operator new[]    (long unsigned int size);
void  operator delete   (void* ptr);
void  operator delete[] (void* ptr);
}

#endif
