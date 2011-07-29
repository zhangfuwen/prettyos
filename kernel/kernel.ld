OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
STARTUP(object_files/kernel/kernel.o)
SECTIONS
{
    . = 0x00100000;
    _kernel_beg = .;
    .text   : { _text_start = .;   *(.text)                  }
    .data   : { _data_start = .;   *(.data)                  }
    .rodata : { _rodata_start = .; *(.rodata)                }
    .bss    : { _bss_start = .;    *(.bss);    _bss_end = .; }
    .cdi    : { _cdi_start = .;    *(.cdi);    _cdi_end = .; }
    _kernel_end = .;
}
