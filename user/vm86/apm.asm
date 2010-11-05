[map symbols documentation/apm.map] ; use this for ckernel.c addresses
[bits 16]

org 0x200

ConnectAPM:
	; Disconnect first
	mov ah, 0x53
	mov al, 0x04
	xor bx, bx
	int 0x15
	jc .error1
	.error1:

	; Connect now
	mov al, 0x01
	int 0x15
	jc .error2
	.error2:
	ret

ActivateAPM:
	mov ah, 0x53
	mov al, 0x08
	mov bx, 1
	mov cx, 1
	int 0x15
	jc .error
	.error:
	ret


CheckAPM:
    mov ah, 0x53
	xor al, al
	xor bx, bx
	mov ds, bx
	int 0x15
	jc .error
		mov byte [0x1300], 1 ; APM is available, connect to it and activate it
		call ConnectAPM
		call ActivateAPM
		jmp exitvm86
	.error:
	    mov byte [0x1300], 0 ; APM is not available
		jmp exitvm86

SetAPMState:
	xor ax, ax
	mov ds, ax
	mov ah, 0x53
	mov al, 0x07
	mov bx, 1
	mov cx, word [0x1300]
	int 0x15
	jc .error
		mov byte [0x1300], 1
		jmp exitvm86
	.error:
		mov byte [0x1300], 0
		jmp exitvm86


; stop and leave vm86-task
exitvm86:
    hlt     ; is translated as exit() at vm86.c
    jmp $   ; endless loop
