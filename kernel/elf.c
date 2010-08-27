/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "task.h"

// in task.c used for paging_free of memory for user program
void* globalUserProgAddr; 
uint32_t globalUserProgSize;

typedef enum
{
    ET_NONE  = 0,
    ET_REL   = 1,
    ET_EXEC  = 2,
    ET_DYN   = 3,
    ET_CORE  = 4
} elf_header_type_t;

typedef enum
{
    EM_NONE  = 0,
    EM_M32   = 1,
    EM_SPARC = 2,
    EM_386   = 3,
    EM_86K   = 4,
    EM_88K   = 5,
    EM_860   = 7,
    EM_MIPS  = 8
} elf_header_machine_t;

typedef enum
{
    EV_NONE    = 0,
    EV_CURRENT = 1
} elf_header_version_t;

typedef enum
{
    EI_MAG0    = 0,
    EI_MAG1    = 1,
    EI_MAG2    = 2,
    EI_MAG3    = 3,
    EI_CLASS   = 4,
    EI_DATA    = 5,
    EI_VERSION = 6,
    EI_PAD     = 7
} elf_header_ident_t;

typedef enum
{
    ELFCLASSNONE = 0,
    ELFCLASS32   = 1,
    ELFCLASS64   = 2
} elf_header_ident_class_t;

typedef enum
{
    ELFDATANONE = 0,
    ELFDATA2LSB = 1,
    ELFDATA2MSB = 2
} elf_header_ident_data_t;

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


typedef enum
{
    PT_NULL = 0,
    PT_LOAD,
    PT_DYNAMIC,
    PT_INTERP,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR
} program_header_flags_t;

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
} program_header_t;

bool elf_exec(const void* elf_file, uint32_t elf_file_size, const char* programName)
{
    const uint8_t* elf_beg = elf_file;
    const uint8_t* elf_end = elf_beg + elf_file_size;

    // Read the header
    const elf_header_t* header = (elf_header_t*) elf_beg;

    // validation checks
    bool validationFlag = true;

    textColor(0x0C);
    printf("\n");
    if (header->ident[EI_MAG0]    != 0x7F        ) { validationFlag = false; printf("\nmagic 0x07           failed."); }
    if (header->ident[EI_MAG1]    != 'E'         ) { validationFlag = false; printf("\nmagic E              failed."); }
    if (header->ident[EI_MAG2]    != 'L'         ) { validationFlag = false; printf("\nmagic L              failed."); }
    if (header->ident[EI_MAG3]    != 'F'         ) { validationFlag = false; printf("\nmagic F              failed."); }
    if (header->ident[EI_CLASS]   != ELFCLASS32  ) { validationFlag = false; printf("\nELFCLASS32           failed."); }
    if (header->ident[EI_DATA]    != ELFDATA2LSB ) { validationFlag = false; printf("\nELFDATA2LSB          failed."); }
    if (header->ident[EI_VERSION] != EV_CURRENT  ) { validationFlag = false; printf("\nEV_CURRENT           failed."); }
    if (header->type              != ET_EXEC     ) { validationFlag = false; printf("\nET_EXEC (type)       failed."); }
    if (header->machine           != EM_386      ) { validationFlag = false; printf("\nEM_386 (machine)     failed."); }
    if (header->version           != EV_CURRENT  ) { validationFlag = false; printf("\nEV_CURRENT (version) failed."); }

    if (validationFlag == false)
    {
        textColor(0x0C);
        printf("\n\nvalidation checks failed");
        textColor(0x0F);
        memshow(elf_file, 512);
        return false;
    }

    page_directory_t* pd = paging_create_user_pd();

    // Read all program headers
    const uint8_t* header_pos = elf_beg + header->phoff;
    for (uint32_t i=0; i<header->phnum; ++i)
    {
        // Check whether the entry exceeds the file
        if (header_pos+sizeof(program_header_t) >= elf_end)
        {
            return -1;
        }

        program_header_t* ph = (program_header_t*)header_pos;

        ///
        #ifdef _DIAGNOSIS_
        textColor(0x02);
        printf("ELF file program header:\n");
        const char* types[] = { "NULL", "Loadable Segment", "Dynamic Linking Information",
                                "Interpreter", "Note", "??", "Program Header" };
        printf("  %s, offset %u, vaddr %X, paddr %X, filesz %u, memsz %u, flags %u, align %u\n",
            types[ph->type], ph->offset, ph->vaddr, ph->paddr, ph->filesz, ph->memsz, ph->flags, ph->align);
        textColor(0x0F);
        #endif
        ///

        /// TODO: check read/write privileges (info in elf?)
        // Allocate code area for the user program
        
        globalUserProgAddr = (void*)(ph->vaddr);
        globalUserProgSize = alignUp(ph->memsz,PAGESIZE);
        if (! paging_alloc(pd, globalUserProgAddr, globalUserProgSize, MEM_USER | MEM_WRITE))
        {
            return false;
        }

        /// TODO: check all sections, not only code

        // Copy the code, using the user's page directory
        cli();
        paging_switch(pd);
        memset((void*)ph->vaddr, 0, ph->memsz); // to set the bss (Block Started by Symbol) to zero
        memcpy((void*)ph->vaddr, elf_beg+ph->offset, ph->filesz);
        paging_switch(kernel_pd);
        sti();

        header_pos += header->phentrysize;
    }

    // Execute the task
    if(strcmp("Shell", programName) == 0)
    {
        create_task(pd, (void*)header->entry, 3);
    }
    else
    {
        create_ctask(pd, (void*)header->entry, 3, programName);
    }

    return true;
}

/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
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
