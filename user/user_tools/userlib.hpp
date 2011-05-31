#ifndef USERLIB_HPP
#define USERLIB_HPP

#define _cplusplus

extern "C" {
#include "userlib.h"

void* operator new      (size_t size);
void* operator new[]    (size_t size);
void  operator delete   (void* ptr);
void  operator delete[] (void* ptr);
}

#endif
