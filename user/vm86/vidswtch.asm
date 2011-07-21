[map symbols documentation/vidswtch.map] ; use this for ckernel.c addresses
[bits 16]

org 0x100

SetDisplayStart:
    mov ax, 0x4F07
    xor bx, bx           ; Set bh to 0 and use it to set ds to 0, too.
    mov ds, bx
    mov dx, word[0x1800] ; Set first Displayed Scan Line
    mov cx, word[0x1802] ; Set first Displayed Pixel in Scan Line
    int 10h
    jmp exitvm86

Enable8bitPalette:
    mov ax, 0x4F08
    xor bl, bl
    mov bh, 8
    int 10h
    jmp exitvm86

SwitchToTextMode:
    mov ax, 0x0002
    int 0x10
    mov ax, 0x1112
    xor bl, bl
    int 0x10
    jmp exitvm86

SwitchToVideoMode:
    xor ax, ax
    mov ds, ax
    mov ax, 0x4F02
    mov bx, word [0x3600] ; video mode
    int 10h
    jmp exitvm86

GetVgaInfoBlock:
    xor ax, ax
    mov es, ax
    mov di, 0x3400
    mov ax, 0x4F00
    int 10h
    jmp exitvm86

GetModeInfoBlock:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov di, 0x3600
    mov ax, 0x4F01
    mov cx, word [0x3600] ; video mode
    int 10h
    jmp exitvm86

; stop and leave vm86-task
exitvm86:
    hlt          ; is translated as exit() at vm86.c
    jmp exitvm86 ; endless loop
