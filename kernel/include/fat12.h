#ifndef _FAT12_H
#define _FAT12_H

#include "os.h"

typedef struct RootDirEntry
{
    char name[11];
    int32_t  firstCluster;
    int32_t  filesize;
    //...
} RootDirEntry_t;

typedef struct File
{
    uint16_t chain[2850];
    uint16_t RDE_Num;
} File_t;

RootDirEntry_t RDE[224];

int32_t flpydsk_read_directory();

#endif
