[Bits 32]

extern _main     ; entry point in ckernel.c

GRUB_FLAGS           equ 10b                               ; Flags for GRUB header
GRUB_MAGIC_NUMBER    equ 0x1BADB002                        ; Magic number for GRUB header
GRUB_HEADER_CHECKSUM equ -(GRUB_MAGIC_NUMBER + GRUB_FLAGS) ; Checksum for GRUB header

align 4
MultiBootHeader:            ; This is a "multiboot" header for GRUB, this structure must be first (?)
    dd GRUB_MAGIC_NUMBER
    dd GRUB_FLAGS
    dd GRUB_HEADER_CHECKSUM

; This is an entry point for GRUB
KernelStart:
    mov esp, 0x190000

    push ebx     ; EBX contains address of the "multiboot" structure

    call _main   ; --> C-Kernel

    cli
    hlt
