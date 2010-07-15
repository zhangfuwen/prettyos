[bits 16]
[section .text]

; opcode tests
sti
cli
sti
cli
sti
pushf
popf
pushf
popf

; switch video mode
mov ax, 0013h
int 10h

mov ax, 0xa
mov ds, ax
mov word [700], 2
mov word [701], 3
mov word [702], 4

mov ax, 0xb
mov ds, ax
mov word [8700], 2
mov word [8701], 3
mov word [8702], 4

jmp $

;exit
int 3
