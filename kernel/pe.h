#ifndef PE_H
#define PE_H

#include "filesystem/fsmanager.h"


bool pe_filename(const char* filename);
bool pe_header(file_t* file);
void* pe_prepare(const void* file, size_t size, pageDirectory_t* pd);


#endif
