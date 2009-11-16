#ifndef KHEAP_H
#define KHEAP_H

#include "os.h"


#define HEAP_START_ADDRESS 0xC0000000


void heap_install();
void* k_malloc( uint32_t size, uint32_t alignment );
void k_free( void* mem );


#endif
