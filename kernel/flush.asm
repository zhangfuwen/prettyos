section .text

; flush.asm -- contains global descriptor table and interrupt descriptor table setup code

GLOBAL _gdt_flush    ; Allows the C code to call gdt_flush().

_gdt_flush:
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

GLOBAL _idt_flush    ; Allows the C code to call idt_flush().

_idt_flush:
    mov eax, [esp+4]  ; Get the pointer to the IDT, passed as a parameter. 
    lidt [eax]        ; Load the IDT register
    ret

GLOBAL _tss_flush    ; Allows our C code to call tss_flush().

_tss_flush:
   mov ax, 0x2B      ; Load the index of our TSS structure - The index is
                     ; 0x28, as it is the 5th selector and each is 8 bytes
                     ; long, but we set the bottom two bits (making 0x2B)
                     ; so that it has an RPL of 3, not zero.
   ltr ax            ; Load 0x2B into the task state register.
   ret



