OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)
ENTRY(KernelStart)
SECTIONS
{
  . = 0x00040000;
  __kernel_beg = .;
  .text   : { __code_start = .; }
  . = ALIGN(0x1000);
  .text1  : { data.o(.text)                                            }
  .data   : { __data_start = .;   *(.data)                             }    
  .rodata : { __rodata_start = .; *(.rodata) *(.rdata)                 }
  .bss    : { __bss_start = .;    *(.bss) *(COMMON)                    }
  __kernel_end = .;
  __end = .;
}