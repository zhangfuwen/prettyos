#ifndef KHEAP_H
#define KHEAP_H

#include "os.h"


void* heap_getCurrentEnd();
void  heap_install();
void  heap_logRegions();
void* malloc(uint32_t size, uint32_t alignment, char* comment);

#ifdef _BROKENFREE_DIAGNOSIS_
  void f_free(void* addr, const char* file, size_t line);
  #define free(addr) f_free(addr, __FILE__, __LINE__);
#else
  void free(void* addr);
#endif


#endif