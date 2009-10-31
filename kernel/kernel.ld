OUTPUT_FORMAT("binary")
ENTRY(KernelStart)
SECTIONS
{ 
  . = 0x00040000;
  .text   : { __code_start = .;   *(EXCLUDE_FILE(shared_pages.o data.o).text) } 
  . = 0x00045000;
  .text1  : { shared_pages.o(.text)                                    } 
  . = 0x00046000;
  .text2  : { data.o(.text)                                            }
  .data   : { __data_start = .;   *(.data)                             } 	
  .rodata : { __rodata_start = .; *(.rodata) *(.rdata)                 }
  .bss    : { __bss_start = .;    *(.bss) *(COMMON)                    } 
  __end = .;
}