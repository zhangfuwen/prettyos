OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
STARTUP(object_files/kernel/kernel.o)
SECTIONS
{
    . = 0x00100000;
    _kernel_beg = .;
    .text   : { _code_start = .;                        }
    . = ALIGN(0x1000);
    .text1  : { object_files/kernel/data.o(.text)       }
    .data   : { _data_start = .;   *(.data)             }
    .rodata : { _rodata_start = .; *(.rodata) *(.rdata) }
    .bss    : { _bss_start = .;    *(.bss)              }
    _start_cdi_drivers = .;
    .cdi    : { _cdi_start = .;    *(.cdi)              }
    _stop_cdi_drivers = .;
    _kernel_end = .;
}
