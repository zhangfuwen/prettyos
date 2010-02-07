; start.asm

[BITS 32]
extern __bss_start
extern __end
extern _main
global _start

_start:
    mov esp, 0x600000 ; stackpointer
    call _main
	
    jmp $
