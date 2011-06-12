#ifndef ELF_H
#define ELF_H

#include "filesystem/fsmanager.h"
#include "paging.h"


bool  elf_filename(const char* filename);
bool  elf_header(file_t* file);
void* elf_prepare(const void* file, size_t size, pageDirectory_t* pd);


#endif
