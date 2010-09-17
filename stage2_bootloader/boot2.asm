;boot2.asm
[map symbols boot2.map]
[Bits 16]
org 0x500
jmp entry_point

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Memory Management:
; org                   500
; data/extra segments     0
; stack               9FC00
; RM kernel            3000
; PM kernel          100000
; memory tables        1000
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;*******************************************************
;    Includes and Defines
;*******************************************************
%include "gdt.inc"                ; GDT definition
%include "A20.inc"                ; A20 gate enabling
%include "Fat12.inc"              ; FAT12 driver
%include "GetMemoryMap.inc"       ; INT 0x15, eax = 0xE820

%define IMAGE_PMODE_BASE 0x100000 ; final kernel location in Protected Mode
;%define IMAGE_RMODE_BASE 0x3000   ; intermediate kernel location in Real Mode

ImageName     db "KERNEL  BIN"
ImageSize     dd 0

;*******************************************************
;    Data Section
;*******************************************************
msgGTD            db 0x0D, 0x0A, "GTD installed ...", 0
msgUnrealMode     db 0x0D, 0x0A, "Unreal Mode entered ...", 0
msgLoadKernel     db 0x0D, 0x0A, "Now loading Kernel ...", 0
msgFloppyMotorOff db 0x0D, 0x0A, "Floppy Disk Motor switched off ...", 0
msgSwitchToPM     db 0x0D, 0x0A, "Now switching to Protected Mode (PM) ...", 0
msgFailure        db 0x0D, 0x0A, "Missing KERNEL.BIN (Fatal Error)", 0

entry_point:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ax,0x9000       ; stack
    mov ss,ax
    mov sp,0xFC00       ; stacksegment:stackpointer (linear): 9FC00h (below BIOS area)
    sti

A20:
    call EnableA20

;*******************************************************
;    Determine physical memory INT 0x15, eax = 0xE820
;    input: es:di (destination buffer)
;*******************************************************

Get_Memory_Map:
    xor eax, eax
    mov ds, ax
    mov di, 0x1000
    call get_memory_by_int15_e820
    xor ax, ax
    mov es, ax          ; important to null es!

Install_GDT:
 
    call InstallGDT
	mov si, msgGTD
    call print_string
 
    call EnterUnrealMode
	mov si, msgUnrealMode
    call print_string

    sti

Load_Root:

    mov si, msgLoadKernel
    call print_string

    call LoadRoot
    mov edi, IMAGE_PMODE_BASE
    mov esi, ImageName

    call LoadFile                      ; FAT12.inc

    mov DWORD [ImageSize], ecx
    cmp ax, 0
    je EnterProtectedMode
    mov si, msgFailure
    call print_string
    xor ah, ah

;*******************************************************
;    Switch from Real Mode (RM) to Protected Mode (PM)
;*******************************************************
EnterProtectedMode:

    ; switch off floppy disk motor
    mov dx,0x3F2
    mov al,0x0C
    out dx,al
	mov si, msgFloppyMotorOff
    call print_string

    ; switch to PM
	mov si, msgSwitchToPM
    call print_string
    cli
    mov eax, cr0                       ; bit 0 in CR0 has to be set for entering PM
    or al, 1
    mov cr0, eax
    jmp DWORD CODE_DESC:ProtectedMode  ; far jump to code selector (cs = 0x08)

[Bits 32]
ProtectedMode:
    mov ax, DATA_DESC                  ; set data segments to data selector (0x10)
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov esp, 0x9000

;*******************************************************
;    Execute Kernel
;*******************************************************
EXECUTE:
    jmp DWORD CODE_DESC:IMAGE_PMODE_BASE
    cli
    hlt

;*******************************************************
;    calls, e.g. print_string
;*******************************************************
[BITS 16]
print_string:
    mov ah, 0x0E
    print_string.loop:
    lodsb                       ; fetch a byte from SI
    or al, al
    jz .done                    ; if zero end loop
    int 0x10                    ; put character to sreen
    jmp print_string.loop
    .done:
    ret
