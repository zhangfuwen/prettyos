#ifndef PAGING_H
#define PAGING_H

#include "os.h"


#define MEMORY_MAP_ADDRESS 0x1000


static const uint32_t MEM_READONLY = 0;
static const uint32_t MEM_KERNEL = 0;
static const uint32_t MEM_WRITE = 2;
static const uint32_t MEM_USER = 4;

struct page_directory_;
typedef struct page_directory_ page_directory_t;

bool paging_alloc( page_directory_t* pd, void* addr, uint32_t size, uint32_t flags );
void paging_free( page_directory_t* pd, void* addr, uint32_t size );

void paging_activate_pd( page_directory_t* pd  );
page_directory_t* paging_create_user_pd();
void paging_destroy_user_pd( page_directory_t* pd );

uint32_t paging_install();


#endif
