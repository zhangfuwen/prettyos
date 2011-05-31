#ifndef ELF_H
#define ELF_H

#include "filesystem/fsmanager.h"


bool elf_filename(const char* filename);
bool elf_header(file_t* file);
bool elf_exec(const void* file, size_t size, const char* programName);


#endif
