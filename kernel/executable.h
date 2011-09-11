#ifndef EXECUTABLE_H
#define EXECUTABLE_H

#include "filesystem/fsmanager.h"


FS_ERROR executeFile(const char* path, size_t argc, char* argv[]);


#endif
