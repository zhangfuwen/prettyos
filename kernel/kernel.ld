OUTPUT_FORMAT("binary")
ENTRY(KernelStart)
SECTIONS
{
  . = 0x00040000;
  __kernel_beg = .;
  .text   : { __code_start = .;   *(EXCLUDE_FILE(shared_pages.o data.o).text) }
  . = ALIGN(0x1000);
  .text1  : { shared_pages.o(.text)                                    }
  . = ALIGN(0x1000);
  .text2  : { data.o(.text)                                            }
  .data   : { __data_start = .;   *(.data)                             }    
  .rodata : { __rodata_start = .; *(.rodata) *(.rdata)                 }
  .bss    : { __bss_start = .;    *(.bss) *(COMMON)                    }
  __kernel_end = .;
  __end = .;
}