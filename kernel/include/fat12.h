// some ideas

#ifndef _FAT12_H
#define _FAT12_H

#include "os.h"

typedef struct RootDirEntry
{
    char name[11];
    int  firstCluster;
    int  filesize;
    //...
} RootDirEntry_t;

typedef struct File
{
    USHORT chain[2850];
    USHORT RDE_Num;
} File_t;

RootDirEntry_t RDE[224]

#endif
