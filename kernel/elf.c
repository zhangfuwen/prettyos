/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/

#include "elf.h"
#include "util.h"
#include "task.h"

// in task.c used for pagingFree of memory for user program
void* globalUserProgAddr;
uint32_t globalUserProgSize;

enum elf_headerType
{
    ET_NONE  = 0,
    ET_REL   = 1,
    ET_EXEC  = 2,
    ET_DYN   = 3,
    ET_CORE  = 4
};

enum elf_headerMachine
{
    EM_NONE  = 0,
    EM_M32   = 1,
    EM_SPARC = 2,
    EM_386   = 3,
    EM_86K   = 4,
    EM_88K   = 5,
    EM_860   = 7,
    EM_MIPS  = 8
};

enum elf_headerVersion
{
    EV_NONE    = 0,
    EV_CURRENT = 1
};

enum elf_headerIdent
{
    EI_MAG0    = 0,
    EI_MAG1    = 1,
    EI_MAG2    = 2,
    EI_MAG3    = 3,
    EI_CLASS   = 4,
    EI_DATA    = 5,
    EI_VERSION = 6,
    EI_PAD     = 7
};

enum elf_headerIdentClass
{
    ELFCLASSNONE = 0,
    ELFCLASS32   = 1,
    ELFCLASS64   = 2
};

enum elf_headerIdentData
{
    ELFDATANONE = 0,
    ELFDATA2LSB = 1,
    ELFDATA2MSB = 2
};

typedef struct
{
    uint8_t  ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentrysize;
    uint16_t phnum;
    uint16_t shentrysize;
    uint16_t shnum;
    uint16_t shstrndx;
} elf_header_t;


enum elf_programHeaderTypes
{
    PT_NULL = 0,
    PT_LOAD,
    PT_DYNAMIC,
    PT_INTERP,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR
};

enum elf_programHeaderFlags
{
    PF_X = 1,
    PF_W = 2,
    PF_R = 4
};

typedef struct
{
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
} elf_programHeader_t;


bool elf_filename(const char* filename)
{
    return(strcmp(filename+strlen(filename)-4, ".elf") == 0);
}

bool elf_header(file_t* file)
{
    elf_header_t header;
    fread(&header, sizeof(elf_header_t), 1, file);

    bool valid = true;
    valid =          header.ident[EI_MAG0]    == 0x7F;
    valid = valid && header.ident[EI_MAG1]    == 'E';
    valid = valid && header.ident[EI_MAG2]    == 'L';
    valid = valid && header.ident[EI_MAG3]    == 'F';
    valid = valid && header.ident[EI_CLASS]   == ELFCLASS32;
    valid = valid && header.ident[EI_DATA]    == ELFDATA2LSB;
    valid = valid && header.ident[EI_VERSION] == EV_CURRENT;
    valid = valid && header.type              == ET_EXEC;
    valid = valid && header.machine           == EM_386;
    valid = valid && header.version           == EV_CURRENT;

    return(valid);
}

void* elf_prepare(const void* file, size_t size, pageDirectory_t* pd)
{
    // Read the header
    const elf_header_t* header = (elf_header_t*)file;

    // Read all program headers
    const elf_programHeader_t* ph = file + header->phoff;
    for (uint32_t i = 0; i < header->phnum; ++i)
    {
        // Check whether the entry exceeds the file
        if ((void*)(ph+i) >= file+size)
        {
            return(0);
        }

        #ifdef _DIAGNOSIS_
        textColor(GREEN);
        printf("ELF file program header:\n");
        const char* types[] = { "NULL", "Loadable Segment", "Dynamic Linking Information",
                                "Interpreter", "Note", "??", "Program Header" };
        printf("  %s, offset %u, vaddr %Xh, paddr %Xh, filesz %u, memsz %u, flags %u, align %u\n",
            types[ph[i].type], ph[i].offset, ph[i].vaddr, ph[i].paddr, ph[i].filesz, ph[i].memsz, ph[i].flags, ph[i].align);
        textColor(TEXT);
        #endif

        // Read flags from header
        uint32_t memFlags = MEM_USER;
        if(ph[i].flags & PF_W)
            memFlags |= MEM_WRITE;

        // Allocate code area for the user program
        globalUserProgAddr = (void*)(ph[i].vaddr);
        globalUserProgSize = alignUp(ph[i].memsz,PAGESIZE);
        if (!pagingAlloc(pd, globalUserProgAddr, globalUserProgSize, memFlags))
        {
            return(0);
        }

        /// TODO: check all sections, not only code

        // Copy the code, using the user's page directory
        cli();
        paging_switch(pd);
        memcpy((void*)ph[i].vaddr, file + ph[i].offset, ph[i].filesz);
        memset((void*)ph[i].vaddr + ph[i].filesz, 0, ph[i].memsz - ph[i].filesz); // to set the bss (Block Started by Symbol) to zero
        paging_switch(currentTask->pageDirectory);
        sti();
    }

    return((void*)header->entry);
}

/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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
