/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "fsmanager.h"
#include "storage/devicemanager.h"
#include "fat.h"
#include "kheap.h"
#include "util.h"
#include "task.h"


fileSystem_t FAT    = {.fopen = &FAT_fopen, .fclose = &FAT_fclose, .fgetc = &FAT_fgetc, .fputc = &FAT_fputc, .fseek = &FAT_fseek, .remove = &FAT_remove, .rename = &FAT_rename, .pformat = &FAT_format, .pinstall = &FAT_pinstall, .folderAccess = &FAT_folderAccess, .folderClose = &FAT_folderClose},
             INITRD = {};


static uint64_t getFSType(FS_t type) // BIT 0-31: subtype, BIT 32-63: fileSystem_t
{
    switch (type)
    {
        case FS_FAT12: case FS_FAT16: case FS_FAT32:
            return(((uint64_t)(uintptr_t)&FAT << 32) | type);
        case FS_INITRD:
            return(((uint64_t)(uintptr_t)&INITRD << 32) | type);
    }
    return(0);
}

// Partition functions
FS_ERROR formatPartition(const char* path, FS_t type, const char* name)
{
    partition_t* part = getPartition(path);
    uint64_t ptype = getFSType(type);
    part->subtype = ptype;
    part->type = (fileSystem_t*)(uintptr_t)(ptype>>32);
    strcpy(part->serial, name);
    part->type->pformat(part);
    return(CE_GOOD);
}

FS_ERROR analyzePartition(partition_t* part)
{
    FS_ERROR e = CE_UNSUPPORTED_FS;

    part->buffer = malloc(512, 0, "part->buffer");

    // Determine type of the partition:
    uint8_t buffer[512];
    singleSectorRead(part->start, buffer, part->disk);

    // Is it a BPB? -> FAT
    BPBbase_t* BPB = (BPBbase_t*)buffer;
    if (BPB->FATcount > 0 && BPB->bytesPerSector%512 == 0)
        part->type = &FAT;
    else // We only know FAT at the moment
        return(e);

    if (part->type->pinstall)
        e = part->type->pinstall(part);

    if (e == CE_GOOD)
        part->mount = true;

    return(e);
}

// File functions
file_t* fopen(const char* path, const char* mode)
{
    file_t* file = malloc(sizeof(file_t), 0, "fsmgr-file");
    file->volume = getPartition(path);

    if (!file->volume)
    {
        free(file);
        printf("\n!file->volume");
        return(0);
    }

    file->seek   = 0;
    file->folder = file->volume->rootFolder; // HACK. Not all files are in the root folder
    file->size   = 0; // Init with 0 but set in FS-specific fopen
    file->EOF    = false;
    file->error  = CE_GOOD;
    file->name   = malloc(strlen(getFilename(path))+1, 0, "fsmgr-filename");
    strcpy(file->name, getFilename(path));

    bool appendMode = false; // Used to seek to end
    bool create = true;
    switch (mode[0])
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

    if(file->volume->type->fopen == 0)
    {
        textColor(ERROR);
        printf("\nERROR: function fopen not defined");
        textColor(TEXT);
        printf("\nfile->volume->type->fopen == 0");
        return (0);
    }

    if (file->volume->type->fopen(file, create, !appendMode&&create) != CE_GOOD)
    {
        free(file->name);
        free(file);
        printf("\nfile->volume->type->fopen(file, create, !appendMode&&create) != CE_GOOD");
        return(0);
    }

    if (appendMode)
    {
        //fseek(file, 0, SEEK_END); // To be used later
    }

    if(currentTask->files == 0)
        currentTask->files = list_create();
    list_append(currentTask->files, file);

    return(file);
}

void fclose(file_t* file)
{
    file->volume->type->fclose(file);
    free(file->name);
    free(file);
    list_delete(currentTask->files, list_find(currentTask->files, file));
    if(list_isEmpty(currentTask->files))
    {
        list_free(currentTask->files);
        currentTask->files = 0;
    }
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

    if (spart == dpart || dpart == 0) // same partition
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
    for (size_t i = 0; i < num; i++)
    {
        dest[i] = fgetc(file);
        if (dest[i] == 0)
            return(dest);
    }
    return(dest);
}

FS_ERROR fputs(const char* src, file_t* file)
{
    FS_ERROR retVal = CE_GOOD;
    for (; *src != 0 && retVal == CE_GOOD; src++)
    {
        retVal = fputc(*src, file);
    }
    return(retVal);
}

size_t fread(void* dest, size_t size, size_t count, file_t* file)
{
    size_t i = 0;
    for (; i < count; i++)
    {
        for (int j = 0; j < size; j++)
        {
            ((uint8_t*)dest)[i*size+j] = fgetc(file);
        }
    }
    return(++i);
}

size_t fwrite(const void* src, size_t size, size_t count, file_t* file)
{
    size_t i = 0;
    for (; i < count; i++)
    {
        for (size_t j = 0; j < size; j++)
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


// Folder functions
folder_t* folderAccess(const char* path, folderAccess_t mode)
{
    folder_t* folder = malloc(sizeof(folder_t), 0, "fsmgr-file");
    folder->volume   = getPartition(path);
    if (folder->volume == 0)
    {   // cleanup
        free(folder);
        return(0);
    }
    folder->files     = list_create();
    folder->subfolder = list_create();
    folder->folder    = folder->volume->rootFolder; // HACK. Not all folders are in the root folder
    folder->name      = malloc(strlen(getFilename(path))+1, 0, "fsmgr-foldername");
    strcpy(folder->name, getFilename(path));

    if (folder->volume->type->folderAccess(folder, mode) != CE_GOOD)
    {   // cleanup
        list_free(folder->files);
        list_free(folder->subfolder);
        free(folder->name);
        free(folder);
        return(0);
    }

    return(folder);
}

void folderClose(folder_t* folder)
{
    folder->volume->type->folderClose(folder);
    list_free(folder->files);
    list_free(folder->subfolder);
    free(folder->name);
    free(folder);
}


// General functions
void fsmanager_cleanup(task_t* task)
{
    if(task->files)
    {
        for(dlelement_t* e = task->files->head; e != 0; e = e->next)
        {
            file_t* file = e->data;
            fclose(file);
        }
        list_free(task->files);
    }
}


/*
* Copyright (c) 2010-2011 The PrettyOS Project. All rights reserved.
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
