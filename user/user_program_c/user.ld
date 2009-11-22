ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)
SECTIONS
{
    . = 0x1400000;
    .text   : { __code_start = .;   *(.text*)         }
    .data   : { __data_start = .;   *(.data)          }    
    .rodata : { __rodata_start = .; *(.rodata)        }
    .bss    : { __bss_start = .;    *(.bss) *(COMMON) }
     __end = .; 	
}
