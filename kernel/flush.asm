section .text

; flush.asm -- contains global descriptor table and interrupt descriptor table setup code

GLOBAL _GDTflush      ; Allows the C code to call GDTflush().

_GDTflush:
    mov eax, [esp+4]  ; Get the pointer to the GDT, passed as a parameter.
    lgdt [eax]        ; Load the new GDT register

    mov ax, 0x10      ; 0x10 is the offset in the GDT to our data segment
    mov ds, ax        ; Load all data segment selectors
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush   ; 0x08 is the offset to our code segment: Far jump!
.flush:
    ret


GLOBAL _TSSflush    ; Allows our C code to call TSSflush().

_TSSflush:
   mov ax, 0x2B      ; Load the index of our TSS structure - The index is
                     ; 0x28, as it is the 5th selector and each is 8 bytes
                     ; long, but we set the bottom two bits (making 0x2B)
                     ; so that it has an RPL of 3, not zero.
   ltr ax            ; LTR loads the task register (TR) with a segment selector pointing to a Task State Segment (TSS).
                     ; The addressed TSS is marked busy, but no hardware task switch occurs.
   ret
