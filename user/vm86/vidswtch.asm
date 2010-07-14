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
int 3
