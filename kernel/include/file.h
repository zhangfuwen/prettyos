#ifndef FILE_H
#define FILE_H

#include "os.h"

// structs, ...
// uint8_t file[51200]; /// <---- TODO: malloc(filesize) ???

// functions, ...
int32_t initCache();
int32_t flpydsk_load(const char* name, const char* ext);
int32_t flpydsk_write(const char* name, const char* ext, void* memory, uint32_t size);
int32_t file_ia(int32_t* fatEntry, uint32_t firstCluster, void* file);

#endif
