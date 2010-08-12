/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "fsmanager.h"
#include "storage/devicemanager.h"
#include "fat.h"
#include "kheap.h"
#include "paging.h"
#include "util.h"
#include "video/console.h"

fileSystem_t FAT, INITRD;


void fsmanager_install()
{
    FAT.fopen  = &FAT_fopen;
    FAT.fclose = &FAT_fclose;
    FAT.fgetc  = &FAT_fgetc;
    FAT.fputc  = &FAT_fputc;
    FAT.fseek  = &FAT_fseek;
    FAT.remove = &FAT_remove;
    FAT.rename = &FAT_rename;
}

// Partition functions
void formatPartition(const char* path)
{
    partition_t* part = getPartition(path);
    part->type->pformat(part);
}

void installPartition(partition_t* part)
{
    part->type->pinstall(part);
}

// File functions
file_t* fopen(const char* path, const char* mode)
{
    file_t* file = malloc(sizeof(file_t), 0, "fsmgr-file");
    file->seek   = 0;
    file->volume = getPartition(path);
    file->size   = 0; // Init with 0 but set in FS-specific fopen
    if(file->volume == NULL)
    {
        free(file);
        return(NULL);
    }
    file->EOF    = false;
    file->error  = CE_GOOD;
    file->name   = malloc(strlen(getFilename(path))+1, 0, "fsmgr-filename");
    strcpy(file->name, getFilename(path));

    bool appendMode = false; // Used to seek to end
    bool create = true;
    switch(mode[0])
    {
        case 'W': case 'w':
            file->write = true;
            file->read = mode[1]=='+';
            break;
        case 'A': case 'a':
            appendMode = true;
            file->write = true;
            file->read = mode[1]=='+';
            break;
        case 'R': case 'r':
            file->read = true;
            create = false;
            file->write = mode[1]=='+';
            break;
        default: // error
            file->read = false;
            file->write = false;
            break;
    }

    if(file->volume->type->fopen(file, create, !appendMode&&create) != CE_GOOD)
    {
        // cleanup
        free(file->name);
        free(file);
        return(NULL);
    }

    if(appendMode)
    {
        //fseek(file, 0, SEEK_END); // To be used later
    }

    return(file);
}

void fclose(file_t* file)
{
    file->volume->type->fclose(file);
    free(file->name);
    free(file);
}


FS_ERROR remove(const char* path)
{
    partition_t* part = getPartition(path);
    return(part->type->remove(getFilename(path), part));
}

FS_ERROR rename(const char* oldpath, const char* newpath)
{
    partition_t* spart = getPartition(oldpath);
    partition_t* dpart = getPartition(newpath);

    if(spart == dpart || dpart == 0) // same partition
    {
        return(spart->type->rename(getFilename(oldpath), getFilename(newpath), spart));
    }
    else
    {
        return(0xFFFFFFFF); // Needs to be implemented: create File at destination, write content of old file to it and remove old file
    }
}

char fgetc(file_t* file)
{
    return(file->volume->type->fgetc(file));
}

FS_ERROR fputc(char c, file_t* file)
{
    return(file->volume->type->fputc(file, c));
}

char* fgets(char* dest, size_t num, file_t* file)
{
    for(size_t i = 0; i < num; i++)
    {
        dest[i] = fgetc(file);
        if(dest[i] == 0)
            return(dest);
    }
    return(dest);
}

FS_ERROR fputs(const char* src, file_t* file)
{
    FS_ERROR retVal = CE_GOOD;
    for(; *src != 0 && retVal == CE_GOOD; src++)
    {
        retVal = fputc(*src, file);
    }
    return(retVal);
}

size_t fread(void* dest, size_t size, size_t count, file_t* file)
{
    size_t i = 0;
    for(; i < count; i++)
    {
        for(int j = 0; j < size; j++)
        {
            ((uint8_t*)dest)[i*size+j] = fgetc(file);
        }
    }
    return(++i);
}

size_t fwrite(const void* src, size_t size, size_t count, file_t* file)
{
    size_t i = 0;
    for(; i < count; i++)
    {
        for(int j = 0; j < size; j++)
        {
            fputc(((uint8_t*)src)[i*size+j], file);
        }
    }
    return(++i);
}


FS_ERROR fflush(file_t* file)
{
    return(file->volume->type->fflush(file));
}


size_t ftell(file_t* file)
{
    return(file->seek);
}

FS_ERROR fseek(file_t* file, size_t offset, SEEK_ORIGIN origin)
{
    return(file->volume->type->fseek(file, offset, origin));
}

FS_ERROR rewind(file_t* file)
{
    return(fseek(file, 0, SEEK_SET));
}


bool feof(file_t* file)
{
    return(file->EOF);
}


FS_ERROR ferror(file_t* file)
{
    return(file->error);
}

void clearerr(file_t* file)
{
    file->error = CE_GOOD;
}


/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
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
