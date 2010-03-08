;******************************************************************************
;	boot2.asm
;	Second Stage Bootloader
;******************************************************************************

[Bits 16]
org 0x500
jmp entry_point             ; go to entry point

;*******************************************************
;	Includes and Defines
;*******************************************************
%include "gdt.inc"          ; GDT definition
%include "A20.inc"          ; A20 gate enabling
%include "Fat12.inc"        ; FAT12 driver
%include "GetMemoryMap.inc" ; INT 0x15, eax = 0xE820 

%define IMAGE_PMODE_BASE 0x40000 ; where the kernel is to be loaded to in protected mode
%define IMAGE_RMODE_BASE 0x3000  ; where the kernel is to be loaded to in real mode

ImageName     db "KERNEL  BIN"

;=====================================================HOTFIX============
;ImageSize     dw 0 muß dd sein wg. "mov [ImageSize], Ecx" weiter unten
ImageSize     dd 0
;=====================================================HOTFIX============

;*******************************************************
;	Data Section
;*******************************************************
msgLoading db 0x0D, 0x0A, "Jumping to OS Kernel...", 0
msgFailure db 0x0D, 0x0A, "Missing KERNEL.BIN", 0x0D, 0x0A, 0x0A, 0

entry_point:
   cli	              ; clear interrupts
   xor ax, ax         ; null segments
   mov ds, ax
   mov es, ax

;=====================================================HOTFIX============
;   mov ss, ax
;   mov sp, 0xFFFF     ; stack begins at 0xffff (downwards)

; stack bei 0000:FFFF ?

mov ax,0x1000
mov ss,ax
xor sp,sp
dec sp
;=====================================================HOTFIX============

   sti	              ; enable interrupts

A20:	
   call EnableA20

;*******************************************************
;   Determine physical memory INT 0x15, eax = 0xE820                     
;   input: es:di -> destination buffer         
;*******************************************************

Get_Memory_Map:
    xor eax, eax
    mov ds, ax
    mov di, 0x1000
    call get_memory_by_int15_e820
    xor ax, ax
    mov es, ax ; important to null es!

Install_GDT:
    call InstallGDT
    sti	
	
Load_Root:

    call LoadRoot
    mov ebx, 0
    mov ebp, IMAGE_RMODE_BASE
    mov esi, ImageName

;=====================================================HOTFIX============
; ES muß erstmal IMAGE_RMODE_BASE sein wg. call zu FindFile in Fat12.inc
mov bx,0x3000
mov es,bx
xor bx,bx
;=====================================================HOTFIX============

    call LoadFile		

    mov DWORD [ImageSize], ecx
    cmp ax, 0
    je EnterProtectedMode
    mov si, msgFailure
    call print_string
    xor ah, ah

;*******************************************************
;   Switch from Real Mode (RM) to Protected Mode (PM)              
;*******************************************************
EnterProtectedMode:
    mov si, msgLoading
    call print_string

    ; switch off floppy disk motor
    mov dx,0x3F2      
    mov al,0x0C
    out dx,al     	

    ; switch to PM
    cli	                           
    mov eax, cr0                          ; set bit 0 in cr0 --> enter PM
    or eax, 1
    mov cr0, eax
    jmp DWORD CODE_DESC:ProtectedMode     ; far jump to fix CS. Remember that the code selector is 0x8!

[Bits 32]
ProtectedMode:
    mov ax, DATA_DESC	                  ; set data segments to data selector (0x10)
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov esp, 0x9000	

CopyImage:
    mov eax, DWORD [ImageSize]
    movzx ebx, WORD  [BytesPerSec]
    mul ebx
    mov ebx, 4
    div ebx
    cld
    mov esi, IMAGE_RMODE_BASE
    mov edi, IMAGE_PMODE_BASE
    mov ecx, eax
    rep movsd                             ; copy image to its protected mode address

;*******************************************************
;   Execute Kernel
;*******************************************************
EXECUTE:
    jmp DWORD CODE_DESC:IMAGE_PMODE_BASE
   	cli
    hlt
	
;*******************************************************
;   calls, e.g. print_string
;*******************************************************
[BITS 16]
print_string:
   lodsb          ; grab a byte from SI
   or al, al      ; logical or AL by itself
   jz .done       ; if the result is zero, get out
   mov ah, 0x0E
   int 0x10       ; otherwise, print out the character!
   jmp print_string
.done:
   ret	
