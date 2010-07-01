/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "initrd.h"
#include "util.h"
#include "kheap.h"
#include "paging.h"
#include "devicemanager.h"
#include "fat.h" 


initrd_header_t*      initrd_header; // The header.
initrd_file_header_t* file_headers;  // The list of file headers.
fs_node_t*            initrd_root;   // Our root directory node.
fs_node_t*            initrd_dev;    // We also add a directory node for /dev, so we can mount devfs later on.
fs_node_t*            root_nodes;    // List of file nodes.
int                   nroot_nodes;   // Number of file nodes.

struct dirent dirent;

// file and data going to RAM disk
extern uintptr_t file_data_start;
extern uintptr_t file_data_end;

port_t      RAMDiskPort;
disk_t      RAMDisk;
partition_t RAMDiskVolume;

void* ramdisk_install(size_t size)
{
    kdebug(0x00, "rd_start: ");

    void* ramdisk_start = malloc(size, 0);
    // shell via incbin in data.asm
    memcpy(ramdisk_start, &file_data_start, (uintptr_t)&file_data_end - (uintptr_t)&file_data_start);
    fs_root = install_initrd(ramdisk_start);
        
    // volume
    RAMDiskVolume.buffer = malloc(512,0); // necessary?
    RAMDiskVolume.disk = &RAMDisk;

    //HACK
    RAMDiskVolume.serial = malloc(13, 0);
    itoa(((uint32_t)(ramdisk_start)/PAGESIZE), RAMDiskVolume.serial);
    RAMDiskVolume.serial[12] = 0;
    
    // disk
    RAMDisk.type         = &RAMDISK;
    RAMDisk.partition[0] = &RAMDiskVolume;
    strncpy(RAMDisk.name, "RAMDisk    ", 11);
    attachDisk(&RAMDisk);

    // port
    RAMDiskPort.type         = &RAM; 
    RAMDiskPort.insertedDisk = &RAMDisk;
    strncpy(RAMDiskPort.name, "RAM        ", 11);   
    attachPort(&RAMDiskPort);

    return(ramdisk_start);
}

static uint32_t initrd_read(fs_node_t* node, uint32_t offset, uint32_t size, char* buffer)
{
    initrd_file_header_t header = file_headers[node->inode];
    size = header.length;
    if (offset > header.length)
    {
        return 0;
    }
    if (offset+size > header.length)
    {
        size = header.length-offset;
    }
    memcpy(buffer, (void*)(header.off + offset), size);
    return size;
}

static struct dirent* initrd_readdir(fs_node_t* node, uint32_t index)
{
    if (node == initrd_root && index == 0)
    {
        strcpy(dirent.name, "dev");
        dirent.name[3] = 0; // NUL-terminate the string
        dirent.ino     = 0;
        return &dirent;
    }

    if (index-1 >= nroot_nodes)
    {
        return 0;
    }
    strcpy(dirent.name, root_nodes[index-1].name);
    dirent.name[strlen(root_nodes[index-1].name)] = 0; // NUL-terminate the string
    dirent.ino = root_nodes[index-1].inode;
    return &dirent;
}

static fs_node_t* initrd_finddir(fs_node_t* node, const char* name)
{
    if (node == initrd_root && !strcmp(name, "dev"))
    {
        return initrd_dev;
    }
    for (int32_t i=0; i<nroot_nodes; ++i)
    {
        if (!strcmp(name, root_nodes[i].name))
        {
            return &root_nodes[i];
        }
    }
    return 0;
}

fs_node_t* install_initrd(void* location)
{
    // Initialise the main and file header pointers and populate the root directory.
    initrd_header = (initrd_header_t*)location;
    file_headers  = (initrd_file_header_t*)(location+sizeof(initrd_header_t));

    // Initialise the root directory.
    kdebug(3, "rd_root: ");

    initrd_root          = malloc(sizeof(fs_node_t), 0);
    strcpy(initrd_root->name, "dev");
    initrd_root->mask    = initrd_root->uid = initrd_root->gid = initrd_root->inode = initrd_root->length = 0;
    initrd_root->flags   = FS_DIRECTORY;
    initrd_root->read    = 0;
    initrd_root->write   = 0;
    initrd_root->open    = 0;
    initrd_root->close   = 0;
    initrd_root->readdir = &initrd_readdir;
    initrd_root->finddir = &initrd_finddir;
    initrd_root->ptr     = 0;
    initrd_root->impl    = 0;

    // Initialise the /dev directory (required!)
    kdebug(3, "rd_dev: ");

    initrd_dev          = malloc(sizeof(fs_node_t), 0);
    strcpy(initrd_dev->name, "ramdisk");
    initrd_dev->mask    = initrd_dev->uid = initrd_dev->gid = initrd_dev->inode = initrd_dev->length = 0;
    initrd_dev->flags   = FS_DIRECTORY;
    initrd_dev->read    = 0;
    initrd_dev->write   = 0;
    initrd_dev->open    = 0;
    initrd_dev->close   = 0;
    initrd_dev->readdir = &initrd_readdir;
    initrd_dev->finddir = &initrd_finddir;
    initrd_dev->ptr     = 0;
    initrd_dev->impl    = 0;

    kdebug(3, "root_nodes: ");

    root_nodes  = malloc(sizeof(fs_node_t)*initrd_header->nfiles, 0);
    nroot_nodes = initrd_header->nfiles;

    // For every file...
    for (uint32_t i=0; i<initrd_header->nfiles; ++i)
    {
        // Edit the file's header - currently it holds the file offset relative to the start of the ramdisk.
        file_headers[i].off += (uintptr_t)location; /// We want it relative to the start of memory. ///

        // Create a new file node.
        strncpy(root_nodes[i].name, file_headers[i].name, 64); /// critical !!!
        root_nodes[i].name[64] = 0;

        root_nodes[i].mask    = root_nodes[i].uid = root_nodes[i].gid = 0;
        root_nodes[i].length  = file_headers[i].length;
        root_nodes[i].inode   = i;
        root_nodes[i].flags   = FS_FILE;
        root_nodes[i].read    = (read_type_t) &initrd_read;
        root_nodes[i].write   = 0;
        root_nodes[i].readdir = 0;
        root_nodes[i].finddir = 0;
        root_nodes[i].open    = 0;
        root_nodes[i].close   = 0;
        root_nodes[i].impl    = 0;
    }
    return initrd_root;
}

/*
* Copyright (c) 2009 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
