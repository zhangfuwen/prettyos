OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)
STARTUP(object_files/kernel/kernel.o)
SECTIONS
{
    . = 0x00100000;
    __kernel_beg = .;
    .text   : { __code_start = .;                        }
    . = ALIGN(0x1000);
    .text1  : { object_files/kernel/data.o(.text)        }
    .data   : { __data_start = .;   *(.data)             }
    .rodata : { __rodata_start = .; *(.rodata) *(.rdata) }
    .bss    : { __bss_start = .;    *(.bss) *(COMMON)    }
    __start_cdi_drivers = .;
    .cdi    : { __cdi_start = .;    *(.cdi)              }
    __stop_cdi_drivers = .;
    __kernel_end = .;
    __end = .;
}
