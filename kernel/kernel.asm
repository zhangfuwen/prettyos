[Bits 32]
section .text    ; ld needs that for coff format
global KernelStart
	
KernelStart:
    mov ax, 0x10
    mov ds, ax   ; data descriptor --> data, stack and extra segment
    mov ss, ax           
    mov es, ax
    xor eax, eax ;null desriptor --> FS and GS
    mov fs, ax
    mov gs, ax    
	
; set bss to zero
extern __bss_start 
extern __end
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, __bss_start
    mov al, 0
    rep stosb   ; repeats instruction decrementing ECX until zero
	            ; and stores value from AL incrementing ES:EDI
			   
    mov esp, 0x190000 ; set stack below 2 MB limit			   
	
extern _main    ; entry point in ckernel.c
	call _main  ; ->-> C-Kernel
    
	cli
	hlt
 
