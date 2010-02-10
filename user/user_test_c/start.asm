; start.asm

[BITS 32]
extern __bss_start
extern __end
extern _main
extern _exit
extern _test
global _start

_start:
    mov esp, 0x600000 ; stackpointer
    call _main	
	
	call _exit	
	call _test
	jmp  $