#ifndef KHEAP_H
#define KHEAP_H

#include "os.h"



void heap_install();
void* malloc( uint32_t size, uint32_t alignment );
void free( void* mem );


#endif
