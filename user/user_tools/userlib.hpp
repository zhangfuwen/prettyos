#ifndef USERLIB_HPP
#define USERLIB_HPP

#define _cplusplus

extern "C" {
#include "userlib.h"

#if __unix__
void* operator new      (size_t size);
void* operator new[]    (size_t size);
void  operator delete   (void* ptr);
void  operator delete[] (void* ptr);
#else
void* operator new      (unsigned long size);
void* operator new[]    (unsigned long size);
void  operator delete   (void* ptr);
void  operator delete[] (void* ptr);
#endif
}

#endif
