/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "fs.h"

fs_node_t* fs_root = 0; // The root of the filesystem.

uint32_t read_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer)
{
    if (node->read != 0) // Has the node got a read callback?
        return node->read(node, offset, size, buffer);
    else
        return 0;
}

uint32_t write_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer)
{
    if (node->write != 0) // Has the node got a write callback?
        return node->write(node, offset, size, buffer);
    else
        return 0;
}

void open_fs(fs_node_t* node, uint8_t read, uint8_t write)
{
    if (node->open != 0) // Has the node got an open callback?
        return node->open(node);
}

void close_fs(fs_node_t* node)
{
    if (node->close != 0) // Has the node got a close callback?
        return node->close(node);
}

struct dirent* readdir_fs(fs_node_t* node, uint32_t index)
{
    // Is the node a directory, and does it have a callback?
    if (((node->flags&0x7) == FS_DIRECTORY) && (node->readdir != 0))
        return node->readdir(node,index);
    else
        return 0;
}

fs_node_t* finddir_fs(fs_node_t* node, const char*name)
{
    // Is the node a directory, and does it have a callback?
    if (((node->flags&0x7) == FS_DIRECTORY) && (node->finddir != 0))
        return node->finddir(node,name);
    else
        return 0;
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
