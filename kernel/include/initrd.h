#ifndef INITRD_H
#define INITRD_H

#include "fs.h"

extern bool RAMDISKflag;

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
void* ramdisk_install(size_t size);
fs_node_t* install_initrd(void* location);

#endif
