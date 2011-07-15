#ifndef KHEAP_H
#define KHEAP_H

#include "os.h"


typedef struct
{
    uint32_t size;
    bool     reserved;
    char     comment[21];
    uint32_t number;
} __attribute__((packed)) region_t;


void  heap_install();
void* malloc(uint32_t size, uint32_t alignment, char* comment);
void  free(void* mem);
void  heap_logRegions();


#endif
