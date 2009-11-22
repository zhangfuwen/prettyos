#include "os.h"
#include "paging.h"
#include "task.h"


static const void* USERCODE_VADDR = (void*)0x01400000;




typedef enum
{
	ET_NONE = 0,
	ET_REL = 1,
	ET_EXEC = 2,
	ET_DYN = 3,
	ET_CORE = 4
} elf_header_type_t;

typedef enum
{
	EM_NONE = 0,
	EM_M32 = 1,
	EM_SPARC = 2,
	EM_386 = 3,
	EM_86K = 4,
	EM_88K = 5,
	EM_860 = 7,
	EM_MIPS = 8
} elf_header_machine_t;

typedef enum
{
	EV_NONE = 0,
	EV_CURRENT = 1
} elf_header_version_t;

typedef enum
{
	EI_MAG0 = 0,
	EI_MAG1 = 1,
	EI_MAG2 = 2,
	EI_MAG3 = 3,
	EI_CLASS = 4,
	EI_DATA = 5,
	EI_VERSION = 6,
	EI_PAD = 7
} elf_header_ident_t;

typedef enum
{
	ELFCLASSNONE = 0,
	ELFCLASS32 = 1,
	ELFCLASS64 = 2
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




bool elf_exec( const void* elf_file, uint32_t elf_file_size )
{
    const char* elf_beg = elf_file;
    const char* elf_end = elf_beg + elf_file_size;

    // Read the header
    const elf_header_t* header = (elf_header_t*) elf_beg;
    if ( header->ident[EI_MAG0]!=0x7F || header->ident[EI_MAG1]!='E' || header->ident[EI_MAG2]!='L' || header->ident[EI_MAG3]!='F' )
        return false;
    if ( header->ident[EI_CLASS]!=ELFCLASS32 || header->ident[EI_DATA]!=ELFDATA2LSB || header->ident[EI_VERSION]!=EV_CURRENT )
        return false;
    if ( header->type!=ET_EXEC || header->machine!=EM_386 || header->version!=EV_CURRENT )
        return false;

    // Further restrictions..
    ASSERT( header->phnum == 1 );

    page_directory_t* pd = paging_create_user_pd();

    // Read all program headers
    const char* header_pos = elf_beg + header->phoff;
    for ( uint32_t i=0; i<header->phnum; ++i )
    {
        // Check whether the entry exceeds the file
        if ( header_pos+sizeof(program_header_t) >= elf_end )
            return -1;

        program_header_t* ph = (program_header_t*)header_pos;

        #ifdef _DIAGNOSIS_
        settextcolor(2,0);
        printformat( "ELF file program header:\n" );
        const char* types[] = { "NULL", "Loadable Segment", "Dynamic Linking Information", "Interpreter", "Note", "??", "Program Header" };
        printformat( "  %s, offset %i, vaddr %X, paddr %X, filesz %i, memsz %i, flags %i, align %i\n", types[ph->type], ph->offset, ph->vaddr, ph->paddr, ph->filesz, ph->memsz, ph->flags, ph->align );
        settextcolor(15,0);
        #endif
        //

        ASSERT( (const void*)(ph->vaddr) == USERCODE_VADDR );
        ASSERT( (const void*)(header->entry) == USERCODE_VADDR );

        // Allocate code area for the user program
        if ( ! paging_alloc( pd, (void*)(ph->vaddr), alignUp(ph->memsz,PAGESIZE), MEM_USER | MEM_WRITE ) )
            return false;

        // Copy the code
        cli();
        paging_switch( pd );
        k_memcpy( (void*)(ph->vaddr), elf_beg+ph->offset, ph->filesz );
        paging_switch( NULL );
        sti();

        header_pos += header->phentrysize;
    }

    // Execute the task
    create_task( pd, (void*)(header->entry), 3 );

    return true;
}
