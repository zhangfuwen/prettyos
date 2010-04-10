#ifndef KHEAP_H
#define KHEAP_H

#include "os.h"


typedef struct
{
    uint32_t size;
    bool     reserved;
} region_t;


void heap_install();
void* malloc( uint32_t size, uint32_t alignment );
void free( void* mem );


#endif
