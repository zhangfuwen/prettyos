; The CPU needs a table that contains information about interrupts that may
;  occur. Each entry is 8 bytes in size and holds the interrupt-routine's
;  address as well as some flags related to that interrupt. The default flags
;  are "0x0008, 0x8E00". If user code shall be allowed to trigger a specific
;  interrupt, set the second flag word to 0xEE00 instead of 0x8E00.
; The interrupts numbered 0-32 are called "Interrupt Service Routines (ISRs)"
;  and are triggered automatically by the CPU when something bad happens, e.g.
;  at division by zero, occurance of broken instruction code or access to a
;  disallowed memory area.
; Interrupts 32-256 are called "Interrupt ReQuests (IRQs)", some of them
;  (no 32-48) are again triggered externally e.g. by the timer  chip or the
;  keyboard and the rest can be fired internally using an "int 0xXX" assembler
;  instruction.
;
; This file contains:
;  - Code for each interrupt routine we are using. The code is highly redundant
;    so we use NASM's macro facility to create the routines. Each routine
;    disables the interrupts, eventually pushes an dummy error code, pushes
;    it's interrupt number and jumps to a "common" routine which proceeds.
;
;  - The common routine which is invoked by each interrupt routine. It saves
;    the registers and jumps to some C-code which finally handles the
;    interrupt.
;
;  - The initialization code:
;      * We fill the interrupt descriptor table with valid entries.
;      * The IRQs initially collide with the ISRs (both use 0-15) so we have to
;        "remap" the IRQs from 0-15 to 32-48.
;      * Perform the actual "load" operation.

global idt_install
extern irq_handler

%define CONTEXT_SWITCH_CALL 126
%define SYSCALL_NUMBER 127



section .text


; Template for a single interrupt-routine:
%macro IR_ROUTINE 1
    _ir%1:
    cli
    %if (%1!=8) && (%1<10 || %1>14)
    push dword 0    ; Dummy error code needs to be pushed on some interrupts
    %endif
    push dword %1
    jmp ir_common_stub
%endmacro

; Create the 48 interrupt-routines
%assign routine_nr 0
%rep 48
    IR_ROUTINE routine_nr
    %assign routine_nr routine_nr+1
%endrep

; Create the Syscall ISR routines
IR_ROUTINE CONTEXT_SWITCH_CALL
IR_ROUTINE SYSCALL_NUMBER


; Called from each interrupt routine, saves registers and jumps to C-code
ir_common_stub:
    push eax
    push ecx
    push edx
    push ebx
    push ebp
    push esi
    push edi

    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp          ; parameter of _irq_handler
    call irq_handler
    mov esp, eax      ; return value: changed or unchanged esp

    pop gs
    pop fs
    pop es
    pop ds

    pop edi
    pop esi
    pop ebp
    pop ebx
    pop edx
    pop ecx
    pop eax

    add esp, 8
    iret


; Setup and load the IDT
idt_install:
    %macro DO_IDT_ENTRY 3
    mov ecx, _ir%1
    mov [_idt_table+(%1)*8+0], cx
    mov [_idt_table+(%1)*8+2], word %2
    mov [_idt_table+(%1)*8+4], word %3
    shr ecx, 16
    mov [_idt_table+(%1)*8+6], cx
    %endmacro

    ; Execute the macro to fill the interrupt table, unfilled entries remain zero.
    %assign COUNTER 0
    %rep 48
        DO_IDT_ENTRY COUNTER, 0x0008, 0x8E00
        %assign COUNTER COUNTER+1
    %endrep

    DO_IDT_ENTRY SYSCALL_NUMBER,      0x0008, 0xEE00
    DO_IDT_ENTRY CONTEXT_SWITCH_CALL, 0x0008, 0xEE00

    ; Remap IRQ 0-15 to 32-47 (see http://wiki.osdev.org/PIC#Initialisation)
    mov al, 0x11
    out 0x20, al
    out 0xA0, al
    mov al, 0x20
    out 0x21, al
    mov al, 0x28
    out 0xA1, al
    mov al, 0x04
    out 0x21, al
    mov al, 0x02
    out 0xA1, al
    dec al              ; al is now one
    out 0x21, al
    out 0xA1, al
    dec al              ; al is now zero
    out 0x21, al
    out 0xA1, al

    ; Perform the actual load operation
    lidt [idt_descriptor]
    ret


section .data

; Interrupt Descriptor Table
; Holds 256 entries each 8 bytes of size
_idt_table:
    times 256*8 db 0

; IDT Descriptor holds the IDT's size minus 1 and it's address
idt_descriptor:
    dw 0x07FF
    dd _idt_table
