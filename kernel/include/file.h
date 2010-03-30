#ifndef FILE_H
#define FILE_H

#include "os.h"

#define ADD 31

// structs, ...

// reserve caches for 5 floppy disk tracks (18*512 byte)
uint8_t cache[5][9216];
uint8_t cacheTrackNumber[5];

// functions, ...
int32_t cacheFirstTracks();
int32_t flpydsk_load(const char* name, const char* ext);
int32_t flpydsk_write(const char* name, const char* ext, void* memory, uint32_t size);
int32_t file_ia(int32_t* fatEntry, uint32_t firstCluster, void* file);
int32_t file_ia_cache(int32_t* fatEntry, uint32_t firstCluster, void* file);

#endif
