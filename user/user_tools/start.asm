; start.asm

[BITS 32]
extern _main
extern _exit
global _start

_start:
    call _main
    call _exit
    jmp  $
