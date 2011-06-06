/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "executable.h"
#include "util.h"
#include "kheap.h"
#include "video/console.h"
#include "elf.h"

filetype_t filetypes[FT_END] =
{
    {&elf_filename, &elf_header, &elf_exec}, // ELF
};

FS_ERROR executeFile(const char* path)
{
    // Open file
    file_t* file = fopen(path, "r");
    if (file == 0) // File not found
    {
        return(CE_FILE_NOT_FOUND);
    }

    // Find out fileformat
    size_t i = 0;
    for (; i < FT_END; i++) // Check name and content of the file
    {
        if (filetypes[i].filename != 0 && filetypes[i].filename(path))
        {
            rewind(file);
            if (filetypes[i].fileheader != 0 && filetypes[i].fileheader(file))
            {
                break; // found
            }
        }
    }

    if (i == FT_END) // Not found, now do not look at filename, just content
    {
        for (i = 0; i < FT_END; i++)
        {
            rewind(file);
            if (filetypes[i].fileheader != 0 && filetypes[i].fileheader(file))
            {
                break; // found
            }
        }
    }

    if (i == FT_END)
    {
        fclose(file);
        printf("The file has an unknown type so it cannot be executed.");
        return(CE_BAD_FILE);
    }

    // Now execute
    size_t size = file->size;
    void* buffer = malloc(size, 0, "executeFile");
    rewind(file);
    fread(buffer, 1, size, file);
    fclose(file);

    if (filetypes[i].execute != 0)
    {
        filetypes[i].execute(buffer, size, path);
    }
    else
    {
        free(buffer);
        printf("Executing the file failed");
        return(CE_BAD_FILE);
    }

    free(buffer);
    return(CE_GOOD);
}

/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
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
