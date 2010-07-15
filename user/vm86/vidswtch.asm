[bits 16]
[section .text]
org 0x100

; switch video mode
mov ax, 0013h
int 10h

mov ax, 0xa000
mov ds, ax
mov word [700], 2
mov word [701], 3
mov word [702], 4

;exit
int 3
