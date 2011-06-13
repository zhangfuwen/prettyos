; start.asm

[BITS 32]
extern main
extern exit
global _start

_start:
    push ecx  ; argv
    push eax  ; argc
    call main
    call exit
    jmp  $
