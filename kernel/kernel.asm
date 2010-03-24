[Bits 32]
section .text    ; ld needs that for coff format

KernelStart:
    mov ax, 0x10
    mov ds, ax   ; data descriptor --> data, stack and extra segment
    mov ss, ax
    mov es, ax
    xor ax, ax   ; null desriptor --> FS and GS
    mov fs, ax
    mov gs, ax

    mov esp, 0x190000 ; set stack below 2 MB limit

extern _main    ; entry point in ckernel.c
    call _main  ; ->-> C-Kernel

    cli
    hlt
