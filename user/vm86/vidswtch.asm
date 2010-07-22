[map symbols vidswtch.map] ; use this for ckernel.c addresses
[bits 16]
[section .text]

org 0x100

video_mode:
	;mov ax, 0013h
	;int 10h
	;VESA entry point
	;video memory to address 0xE0000000
	mov bx, 0x4101 ;0x4105 ;0x4111 ;0105h
	mov ax, 0x4F02
	int 10h
	
	
logo_PrettyOS:
	mov ax, 0xE0000000; 0xa0000 ;0xE0000000 ;0xa0000 ;0xa000 ? 16bits only
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

;SetBank:
;	mov ax, 0x4F05 ;4F05h
;	mov bx, 0
;   mov dx, bank

;GetBank:
;	mov ax, 0x4F05 ;4F05h
;	mov bx, 1
;   mov dx, bank

;SetScanLinePixel:
;   mov ax, 0x4F06
;   mov bl, 0

;GetScanLinePixel:
;	mov ax, 0x4F06
;	mov bl, 1

;SetScanLineBytes:
;   mov ax, 0x4F06
;   mov bl, 02

;GetMAXScanLine:
;	mov ax, 0x4F06
;	mov bl, 3

;SetDisplayStart:
;	mov ax, 0x4F07
;	mov bl, 0

;GetDisplayStart:
;	mov ax, 0x4F07
;	mov bl, 1

;SetDacPalette:
;	mov ax, 0x4F08
;	mov bl, 0

;GetDacPalette:
;	mov ax, 0x4F08
;	mov bl, 1

;SetPalette:
;	mov ax, 0x4F09
;	mov bl, 0			;=00h    Set palette data
						;=02h    Set secondary palette data
						;=80h    Set palette data during vertical retrace

GetPalette:
	mov ax, 0x4F09
    mov bl, 1			;=01h    Get palette data
	int 10h				;=03h    Get secondary palette data

;GetProtectedModeInterface:
;	mov ax, 0x4F0A
;	mov bl, 0

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
