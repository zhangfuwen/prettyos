#ifndef EXECUTABLE_H
#define EXECUTABLE_H

#include "filesystem/fsmanager.h"
#include "paging.h"


typedef struct
{
    bool  (*filename)(const char*);
    bool  (*fileheader)(file_t*);
    void* (*prepare)(const void*, size_t, pageDirectory_t*); // file content, length, page directory. Returns entry point
} filetype_t;

enum FILETYPES {FT_ELF, FT_END};

FS_ERROR executeFile(const char* path, size_t argc, char* argv[]);


#endif
