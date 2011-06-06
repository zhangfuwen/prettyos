#ifndef EXECUTABLE_H
#define EXECUTABLE_H

#include "filesystem/fsmanager.h"

typedef struct
{
    bool (*filename)(const char*);
    bool (*fileheader)(file_t*);
    bool (*execute)(const void*, size_t, const char*); // file content, length, name
} filetype_t;

enum FILETYPES {FT_ELF, FT_END};

extern filetype_t filetypes[FT_END];

FS_ERROR executeFile(const char* path);

#endif
