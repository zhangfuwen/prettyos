%ifndef GETMEMORYMAP_INC
%define GETMEMORYMAP_INC

;******************************************************************************
;    INT 0x15, eax = 0xE820 BIOS function to get a memory map
;    es:di -> destination buffer for 24 byte entries
;    trashes all registers except esi
;******************************************************************************

get_memory_by_int15_e820:
    xor ebx, ebx                    ; ebx must be 0 to start
    mov edx, 0x0534D4150            ; Place "SMAP" into edx
    mov eax, 0xe820
    mov [es:di + 20], dword 1       ; force a valid ACPI 3.X entry
    mov ecx, 24                     ; ask for 24 bytes
    int 0x15
    jc short .failed                ; carry set on first call means "unsupported function"
    mov edx, 0x0534D4150            ; Some BIOSes apparently trash this register?
    cmp eax, edx                    ; on success, eax must have been reset to "SMAP"
    jne short .failed
    test ebx, ebx                   ; ebx = 0 implies list is only 1 entry long (worthless)
    je short .failed
    jmp short .jmpin
.e820lp:
    mov eax, 0xe820                 ; eax, ecx get trashed on every int 0x15 call
    mov [es:di + 20], dword 1       ; force a valid ACPI 3.X entry
    mov ecx, 24                     ; ask for 24 bytes again
    int 0x15
    jc short .e820f                 ; carry set means "end of list already reached"
    mov edx, 0x0534D4150            ; repair potentially trashed register
.jmpin:
    jcxz .skipent                   ; skip any 0 length entries
    cmp cl, 20                      ; got a 24 byte ACPI 3.X response?
    jbe short .notext
    test byte [es:di + 20], 1       ; if so: is the "ignore this data" bit clear?
    je short .skipent
.notext:
    mov ecx, [es:di + 8]            ; get lower dword of memory region length
    test ecx, ecx                   ; is the qword == 0?
    jne short .goodent
    mov ecx, [es:di + 12]           ; get upper dword of memory region length
    jecxz .skipent                  ; if length qword is 0, skip entry
.goodent:
    add di, 24                      ; move to next storage spot
.skipent:
    test ebx, ebx                   ; if ebx resets to 0, list is complete
    jne short .e820lp
.e820f:
    mov [es:di + 8], dword 0        ; do a "size=0"-entry
    mov [es:di +12], dword 0
    ret                             ; test opcode cleared carry flag
.failed:
    stc                             ; "function unsupported" error exit
    ret

[BITS 32]
convert_mmap:
    mov ebp, esp
    mov DWORD [ebp-4], 0x1000 ; src
    mov DWORD [ebp-8], 0x1100 ; dest
    mov edi, 0                ; counts entries
    jmp .L2
.L3:
    mov eax, DWORD [ebp-4]
    mov eax, DWORD [eax]
    xor edx, edx
    mov ecx, DWORD [ebp-8]
    mov DWORD [ecx+4], eax
    mov DWORD [ecx+8], edx
    mov eax, DWORD [ebp-4]
    mov eax, DWORD [eax+8]
    mov ecx, DWORD [ebp-8]
    mov DWORD [ecx+12], eax
    mov DWORD [ecx+16], edx
    mov eax, DWORD [ebp-4]
    mov edx, DWORD [eax+16]
    mov eax, DWORD [ebp-8]
    mov DWORD [eax+20], edx
    mov eax, DWORD [ebp-8]
    mov DWORD [eax], 24
    add DWORD [ebp-4], 24
    add DWORD [ebp-8], 24
    inc edi
.L2:
    mov eax, DWORD [ebp-4]
    mov eax, DWORD [eax+8]
    test eax, eax
    jne .L3
    mov eax, edi
    add eax, eax
    add eax, edi
    sal eax, 3
    ret

%endif
