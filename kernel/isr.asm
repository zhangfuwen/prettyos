section .text

; Interrupt Service Routine isr0 ... isr32 for Exceptions  
global _isr0
global _isr1
global _isr2
global _isr3
global _isr4
global _isr5
global _isr6
global _isr7
global _isr8
global _isr9
global _isr10
global _isr11
global _isr12
global _isr13
global _isr14
global _isr15
global _isr16
global _isr17
global _isr18
global _isr19
global _isr20
global _isr21
global _isr22
global _isr23
global _isr24
global _isr25
global _isr26
global _isr27
global _isr28
global _isr29
global _isr30
global _isr31

global _isr127 ; syscall

;  0: Divide By Zero Exception
_isr0:
    cli
    push dword 0
    push dword 0
    jmp isr_common_stub

;  1: Debug Exception
_isr1:
    cli
    push dword 0
    push dword 1
    jmp isr_common_stub

;  2: Non Maskable Interrupt Exception
_isr2:
    cli
    push dword 0
    push dword 2
    jmp isr_common_stub

;  3: Int 3 Exception
_isr3:
    cli
    push dword 0
    push dword 3
    jmp isr_common_stub

;  4: INTO Exception
_isr4:
    cli
    push dword 0
    push dword 4
    jmp isr_common_stub

;  5: Out of Bounds Exception
_isr5:
    cli
    push dword 0
    push dword 5
    jmp isr_common_stub

;  6: Invalid Opcode Exception
_isr6:
    cli
    push dword 0
    push dword 6
    jmp isr_common_stub

;  7: Coprocessor Not Available Exception
_isr7:
    cli
    push dword 0
    push dword 7
    jmp isr_common_stub

;  8: Double Fault Exception (With Error Code!)
_isr8:
    cli
    push dword 8
    jmp isr_common_stub

;  9: Coprocessor Segment Overrun Exception
_isr9:
    cli
    push dword 0
    push dword 9
    jmp isr_common_stub

; 10: Bad TSS Exception (With Error Code!)
_isr10:
    cli
    push dword 10
    jmp isr_common_stub

; 11: Segment Not Present Exception (With Error Code!)
_isr11:
    cli
    push dword 11
    jmp isr_common_stub

; 12: Stack Fault Exception (With Error Code!)
_isr12:
    cli
    push dword 12
    jmp isr_common_stub

; 13: General Protection Fault Exception (With Error Code!)
_isr13:
    cli
    push dword 13
    jmp isr_common_stub

; 14: Page Fault Exception (With Error Code!)
_isr14:
    cli
    push dword 14
    jmp isr_common_stub

; 15: Reserved Exception
_isr15:
    cli
    push dword 0
    push dword 15
    jmp isr_common_stub

; 16: Floating Point Exception
_isr16:
    cli
    push dword 0
    push dword 16
    jmp isr_common_stub

; 17: Alignment Check Exception (With Error Code!)
_isr17:
    cli
    push dword 17
    jmp isr_common_stub

; 18: Machine Check Exception
_isr18:
    cli
    push dword 0
    push dword 18
    jmp isr_common_stub

; 19: Reserved
_isr19:
    cli
    push dword 0
    push dword 19
    jmp isr_common_stub

; 20: Reserved
_isr20:
    cli
    push dword 0
    push dword 20
    jmp isr_common_stub

; 21: Reserved
_isr21:
    cli
    push dword 0
    push dword 21
    jmp isr_common_stub

; 22: Reserved
_isr22:
    cli
    push dword 0
    push dword 22
    jmp isr_common_stub

; 23: Reserved
_isr23:
    cli
    push dword 0
    push dword 23
    jmp isr_common_stub

; 24: Reserved
_isr24:
    cli
    push dword 0
    push dword 24
    jmp isr_common_stub

; 25: Reserved
_isr25:
    cli
    push dword 0
    push dword 25
    jmp isr_common_stub

; 26: Reserved
_isr26:
    cli
    push dword 0
    push dword 26
    jmp isr_common_stub

; 27: Reserved
_isr27:
    cli
    push dword 0
    push dword 27
    jmp isr_common_stub

; 28: Reserved
_isr28:
    cli
    push dword 0
    push dword 28
    jmp isr_common_stub

; 29: Reserved
_isr29:
    cli
    push dword 0
    push dword 29
    jmp isr_common_stub

; 30: Reserved
_isr30:
    cli
    push dword 0
    push dword 30
    jmp isr_common_stub

; 31: Reserved
_isr31:
    cli
    push dword 0
    push dword 31
    jmp isr_common_stub

;127: Software Interrupt Sys Call
_isr127:
    cli
    push dword 0
    push dword 127
    jmp isr_common_stub


; Call of the C function fault_handler(...)
extern _fault_handler

; Common ISR stub saves processor state, sets up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame.
isr_common_stub:
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
    
	;identical stub
	push esp              ; parameter of _fault_handler 
    call _fault_handler   ;  
    global _fault_tail
    _fault_tail:
	mov esp, eax          ; return value: changed or unchanged esp
	
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

global _irq0
global _irq1
global _irq2
global _irq3
global _irq4
global _irq5
global _irq6
global _irq7
global _irq8
global _irq9
global _irq10
global _irq11
global _irq12
global _irq13
global _irq14
global _irq15

; 32: IRQ0
_irq0:
    cli
    push dword 0
    push dword 32
    jmp irq_common_stub

; 33: IRQ1
_irq1:
    cli
    push dword 0
    push dword 33
    jmp irq_common_stub

; 34: IRQ2
_irq2:
    cli
    push dword 0
    push dword 34
    jmp irq_common_stub

; 35: IRQ3
_irq3:
    cli
    push dword 0
    push dword 35
    jmp irq_common_stub

; 36: IRQ4
_irq4:
    cli
    push dword 0
    push dword 36
    jmp irq_common_stub

; 37: IRQ5
_irq5:
    cli
    push dword 0
    push dword 37
    jmp irq_common_stub

; 38: IRQ6
_irq6:
    cli
    push dword 0
    push dword 38
    jmp irq_common_stub

; 39: IRQ7
_irq7:
    cli
    push dword 0
    push dword 39
    jmp irq_common_stub

; 40: IRQ8
_irq8:
    cli
    push dword 0
    push dword 40
    jmp irq_common_stub

; 41: IRQ9
_irq9:
    cli
    push dword 0
    push dword 41
    jmp irq_common_stub

; 42: IRQ10
_irq10:
    cli
    push dword 0
    push dword 42
    jmp irq_common_stub

; 43: IRQ11
_irq11:
    cli
    push dword 0
    push dword 43
    jmp irq_common_stub

; 44: IRQ12
_irq12:
    cli
    push dword 0
    push dword 44
    jmp irq_common_stub

; 45: IRQ13
_irq13:
    cli
    push dword 0
    push dword 45
    jmp irq_common_stub

; 46: IRQ14
_irq14:
    cli
    push dword 0
    push dword 46
    jmp irq_common_stub

; 47: IRQ15
_irq15:
    cli
    push dword 0
    push dword 47
    jmp irq_common_stub

extern _irq_handler

irq_common_stub:
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
	
	push esp              ; parameter of _irq_handler 
    call _irq_handler    ;  
    global _irq_tail
    _irq_tail:
	mov esp, eax          ; return value: changed or unchanged esp

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
