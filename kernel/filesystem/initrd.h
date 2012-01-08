#ifndef INITRD_H
#define INITRD_H

#include "fs.h"
#include "storage/devicemanager.h"


typedef struct
{
    partition_t* part;   // universal partition container (fsmanager)
    uint32_t     nfiles; // The number of files in the ramdisk.
} INITRD_partition_t;

typedef struct
{
    file_t* file;   // universal file container (fsmanager)
    uint32_t magic; // Magic number, for error checking.
    uint32_t off;   // Offset in the initrd that the file starts.
} INITRD_file_t;

typedef struct
{
    uint32_t nfiles; // The number of files in the ramdisk.
} initrd_header_t;

typedef struct
{
    uint32_t magic;  // Magic number, for error checking.
    char name[64];   // Filename.
    uint32_t off;    // Offset in the initrd that the file starts.
    uint32_t length; // Length of the file.
} initrd_file_header_t;


// Installs the initial ramdisk. It gets passed the address, and returns a completed filesystem node.
disk_t* ramdisk_install();
void*   initrd_install(disk_t* disk, size_t partitionID);
bool    initrd_loadShell();


#endif
