/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "fsmanager.h"
#include "devicemanager.h"
#include "kheap.h"
#include "util.h"
#include "console.h"

fileSystem_t FAT, INITRD;


static bool ValidateChars(char* FileName , bool mode)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> ValidateChars <<<<<");
  #endif

    bool radix = false;
    uint16_t StrSz = strlen(FileName);
    for(uint16_t i=0; i<StrSz; i++)
    {
        if (((FileName[i] <= 0x20) &&  (FileName[i] != 0x05)) ||
             (FileName[i] == 0x22) ||  (FileName[i] == 0x2B)  ||
             (FileName[i] == 0x2C) ||  (FileName[i] == 0x2F)  ||
             (FileName[i] == 0x3A) ||  (FileName[i] == 0x3B)  ||
             (FileName[i] == 0x3C) ||  (FileName[i] == 0x3D)  ||
             (FileName[i] == 0x3E) ||  (FileName[i] == 0x5B)  ||
             (FileName[i] == 0x5C) ||  (FileName[i] == 0x5D)  ||
             (FileName[i] == 0x7C) || ((FileName[i] == 0x2E)  && (radix == true)))
        {
            return false;
        }
        else
        {
            if (mode == false)
            {
                if ((FileName[i] == '*') || (FileName[i] == '?'))
                    return false;
            }
            if (FileName[i] == 0x2E)
            {
                radix = true;
            }
            // Convert lower-case to upper-case
            if ((FileName[i] >= 0x61) && (FileName[i] <= 0x7A))
            {
                FileName[i] -= 0x20;
            }
        }
    }
    return true;
}
static bool FormatFileName(const char* fileName, char* fN2, bool mode)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> FormatFileName <<<<<");
  #endif

    char* pExt;
    uint16_t temp;
    char szName[15];

    for (uint8_t count=0; count<11; count++)
    {
        fN2[count] = ' ';
    }

    if (fileName[0] == '.' || fileName[0] == 0)
    {
        return false;
    }

    temp = strlen(fileName);

    if (temp <= FILE_NAME_SIZE+1)
    {
        strcpy(szName, fileName);
    }
    else
    {
        return false; //long file name
    }

    if (!ValidateChars(szName, mode))
    {
        return false;
    }

    if ((pExt = strchr(szName, '.')) != 0)
    {
        *pExt = 0;
        pExt++;
        if (strlen(pExt) > 3)
        {
            return false;
        }
    }

    if (strlen(szName)>8)
    {
        return false;
    }

    for (uint8_t count=0; count<strlen(szName); count++)
    {
        fN2[count] = szName[count];
    }

    if (pExt && *pExt)
    {
        for (uint8_t count=0; count<strlen(pExt); count++)
        {
            fN2[count + 8] = pExt[count];
        }
    }

    return true;
}


void fsmanager_install()
{
    //FAT.fopen = &FAT_fopen;
    //FAT.fclose = &FAT_fclose;
    //FAT.fseek = &FAT_fseek;
}

// Partition functions
void formatPartition(const char* path)
{
    //partition_t* part = getPartition(path);
    //part->type->pformat(part);
}

void installPartition(partition_t* part)
{
    part->type->pinstall(part);
}

// File functions
file_t* fopen(const char* path, const char* mode)
{
    file_t* file = malloc(sizeof(file_t), 0);
    file->seek   = 0;
    //file->volume = getPartition(path);
    if(file->volume == 0)
    {
        free(file);
        return(0);
    }
    file->EOF    = false;
    file->error  = CE_GOOD;
    file->name   = malloc(13, 0);
    FormatFileName(getFilename(path), file->name, false);

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
        return(0);
    }

    if(appendMode)
    {
        fseek(file, 0, SEEK_END);
    }
    return(file);
}

void fclose(file_t* file)
{
    file->volume->type->fclose(file);
}


FS_ERROR remove(const char* path)
{
    partition_t* part = 0;// = getPartition(path);
    return(part->type->remove(getFilename(path), part));
}

FS_ERROR rename(const char* oldpath, const char* newpath)
{
    /*partition_t* dpart = getPartition(newpath);
    partition_t* spart = getPartition(oldpath);

    if(spart == dpart) // same partition
    {
        return(spart->type->rename(getFilename(oldpath), getFilename(newpath), spart));
    }
    else*/
    {
        return(0xFFFFFFFF); // Needs to be implemented: create File at destination, write content of old file to it and remove old file
    }
}


char* fgets(char* dest, size_t num, file_t* file)
{
    file->volume->type->fgets(file, dest, num);
    return(dest);
}

FS_ERROR fputs(const char* src, file_t* file)
{
    return(file->volume->type->fputs(file, src));
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
