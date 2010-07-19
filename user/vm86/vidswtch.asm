[map symbols vidswtch.map] ; use this for ckernel.c addresses
[bits 16]
[section .text]

org 0x100

video_mode:
	mov ax, 0013h
	int 10h

	mov ax, 0xa000
	mov ds, ax

logo_PrettyOS:
	mov word [0], 0 ; black pixel at x=0 and y=0 
	                ; this step is necessary. Why?
	xor ax, ax
	mov ds, ax 

	hlt     ; is translated as exit() at vm86.c
	jmp $   ; endless loop

text_mode:
	mov ax, 0x0002
	int 0x10	
	mov ax, 0x1112
	xor bl, bl
	int 0x10
	hlt     ; is translated as exit() at vm86.c
	jmp $   ; endless loop