[map symbols vidswtch.map] ; use this for ckernel.c addresses
[bits 16]
[section .text]

org 0x100

video_mode:
	mov ax, 0013h
	int 10h
	;VESA entry point
	;mov bx, 0x4111 ;0105h
	;mov ax, 0x4F02
	;int 10h
	
logo_PrettyOS:
	mov ax, 0xa000
	mov ds, ax
	mov word [0], 0 ; black pixel at x=0 and y=0 
	                ; this step is necessary. Why?
					
	xor ax, ax
	mov ds, ax 

VgaInfoBlock:
    xor ax, ax
	mov es, ax
	mov ax, 0x1000
	mov di, ax
	mov ax, 0x4F00
	int 10h

ModeInfoBlock:
    xor ax, ax
	mov es, ax
	mov ax, 0x1200
	mov di, ax
	mov ax, 0x4F01
	int 10h
	mov word [0x1300], ax
	

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
