%ifndef FAT12_INC
%define FAT12_INC

[BITS 16]

%include "Fat12_BPB.inc"

datasector  dw 0x0000
cluster     dw 0x0000

Sector      db 0x00
Head        db 0x00
Cylinder    db 0x00

;******************************************************************************
; Convert CHS to LBA
; LBA = (cluster - 2) * sectors per cluster
;******************************************************************************

Convert_Cluster_to_LBA:
    sub ax, 2                                  ; zero base cluster number
    xor cx, cx
    mov cl, BYTE [SecPerClus]                  ; convert byte to word
    mul cx
    add ax, WORD [datasector]                  ; base data sector
    ret

;******************************************************************************
; Convert LBA to CHS
; AX      LBA Address to convert
;
; sector          = (logical sector / sectors per track) + 1
; head            = (logical sector / sectors per track) MOD number of heads
; cylinder/track  = logical sector / (sectors per track * number of heads)
;
;******************************************************************************

Convert_LBA_to_CHS:
    xor dx, dx                              ; prepare dx:ax for operation
    div WORD [SecPerTrack]                  ; calculate
    inc dl                                  ; adjust for sector 0
    mov BYTE [Sector], dl
    xor dx, dx                              ; prepare dx:ax for operation
    div WORD [NumHeads]                     ; calculate
    mov BYTE [Head], dl
    mov BYTE [Cylinder], al
    ret

;******************************************************************************
; Read sectors
; CX      Number of sectors to read
; AX      Starting sector
; FS:EBX  Buffer to read to
;******************************************************************************

ReadSectors:
    push    edi
    push    es
    push    word 0x0F00                         ; enough space for up to 4096 byte sectors
    pop     es
.MAIN:
    mov     bp, 0x0005                          ; five retries for error
.SECTORLOOP:
    push    ax
    push    cx
    push    bx
    call    Convert_LBA_to_CHS                  ; convert starting sector to CHS
    mov     ah, 0x02                            ; BIOS read sector
    mov     al, 0x01                            ; read one sector
    xor     bx, bx
    mov     ch, BYTE [Cylinder]                 ; track
    mov     cl, BYTE [Sector]                   ; sector
    mov     dh, BYTE [Head]                     ; head
    mov     dl, BYTE [DriveNum]                 ; drive
    int     0x13                                ; invoke BIOS
    jnc     .SUCCESS                            ; test for read error
    xor     ax, ax                              ; BIOS reset disk
    int     0x13                                ; invoke BIOS
    dec     bp                                  ; decrement error counter
    pop     bx
    pop     cx
    pop     ax
    jnz     .SECTORLOOP                         ; attempt to read again
    int     0x18
.SUCCESS:
    pop     bx
    mov     edi, ebx                            ; copy to destination buffer
    mov     cx, WORD [BytesPerSec]
    shr     cx, 1
    xor     si, si
    .COPY_TO_DEST:
        mov     ax, [es:si]
        mov     [fs:edi], ax
        add     si, 2
        add     edi, 2
        loop    .COPY_TO_DEST

    movzx   eax, WORD [BytesPerSec]
    add     ebx, eax                            ; queue next buffer
    pop     cx
    pop     ax
    inc     ax                                  ; queue next sector
    loop    .MAIN                               ; read next sector
    pop     es
    pop     edi
    ret

;******************************************************************************
;    Defines
;******************************************************************************

%define ROOT_OFFSET  0x02e00
%define FAT_DEST     0x02c00
%define FAT_SEG      0x002c0
%define ROOT_DEST    0x02e00
%define ROOT_SEG     0x002e0

;******************************************************************************
; LoadRoot ()
; Transfers Root Directory Table into memory at 0x7e00
;******************************************************************************

LoadRoot:
    pusha

    ; compute size of root directory and store in "cx"
    xor     cx, cx
    xor     dx, dx
    mov     ax, 32                              ; 32 byte directory entry
    mul     WORD [RootEntries]                  ; total size of directory
    div     WORD [BytesPerSec]                  ; sectors used by directory
    xchg    ax, cx                              ; store in CX

    ; compute location of root directory and store in "ax"
        mov     al, BYTE [NumFATs]              ; number of FATs
    mul     WORD [FATSize]                      ; sectors used by FATs
    add     ax, WORD [ReservedSec]
    mov     WORD [datasector], ax               ; base of root directory
    add     WORD [datasector], cx

    ; read root directory into 0x7e00
    mov     ebx, ROOT_DEST
    call    ReadSectors                         ; read directory table
    popa
    ret

;******************************************************************************
; LoadFAT ()
; Loads FAT table into memory at 0x7c00
; ES:DI Root Directory Table
;******************************************************************************

LoadFAT:
    pusha

    ; compute size of FAT and store it into CX
    xor ax, ax
    mov al, BYTE [NumFATs]                      ; number of FATs
    mul WORD [FATSize]                          ; sectors used by FATs
    mov cx, ax

    ; compute location of FAT and store it into AX
    mov ax, WORD [ReservedSec]

    ; read FAT into memory (Overwrite bootloader at 0x7c00)
    mov ebx, FAT_DEST
    call ReadSectors
    popa
    ret

;******************************************************************************
; FindFile ()
; Search for filename in root table
; DS:SI  File name
; AX     Return value: File index number in directory table (error: -1)
;******************************************************************************

FindFile:

;=====================================================HOTFIX============
; ES muß hier 0x0000 sein wg. "rep cmps" -> [ES:DI] !!!
; und weil DI ungünstig initialisiert ist
push bx
xor bx,bx
mov es,bx
pop bx
;=====================================================HOTFIX============

    push cx                     ; store registers
    push dx
    push bx
    mov  bx, si                 ; copy filename for later

    ; browse root directory for binary image
    mov cx, WORD [RootEntries]  ; load loop counter
    mov di, ROOT_OFFSET         ; locate first root entry at 1 MB mark
    cld                         ; clear direction flag

.LOOP2:
    push cx
    mov cx, 11                  ; eleven character name. Image name is in SI
    mov si, bx                  ; image name is in BX
    push di
    rep cmpsb                   ; test for entry match
    pop di
    je .Found
    pop cx
    add di, 32                  ; queue next directory entry
    loop .LOOP2

.NotFound:
    pop bx                      ; restore registers and return
    pop dx
    pop cx
    mov ax, -1                  ; set error code
    ret

.Found:
    pop ax                      ; return value into AX contains entry of file
    pop bx                      ; restore registers and return
    pop dx
    pop cx
    ret

;******************************************************************************
; LoadFile ()
; ES:SI   File to load
; FS:EDI  Buffer to load file to
; AX      Return value: success 0, error -1
;******************************************************************************


LoadFile:
    xor  ecx, ecx               ; size of file in sectors
    push ecx
    push edi

.FIND_FILE:

    push bx
;=====================================================HOTFIX============
;    push bp
push es
;=====================================================HOTFIX============

    call FindFile               ; find our file. ES:SI contains our filename
    cmp ax, -1
    jne .LOAD_IMAGE_PRE

;=====================================================HOTFIX============
;    pop bp
pop es
;=====================================================HOTFIX============
    pop bx
    pop edi
    pop ecx
    mov ax, -1
    ret

.LOAD_IMAGE_PRE:
    sub edi, ROOT_OFFSET
    sub eax, ROOT_OFFSET

    ; get starting cluster
    push word ROOT_SEG              ;root segment loc
    pop es
    mov dx, WORD [es:di + 0x001A]    ; DI points to file entry in root directory table. Refrence the table...
    mov WORD [cluster], dx          ; file's first cluster
    pop bx                          ; get location to write to so we dont screw up the stack
    pop es
    call LoadFAT

.LOAD_IMAGE:
    ; load the cluster
    mov ax, WORD [cluster]          ; cluster to read
    call Convert_Cluster_to_LBA
    xor cx, cx
    mov cl, BYTE [SecPerClus]

    pop ebx
    call ReadSectors

    pop ecx
    inc ecx
    push ecx
    push ebx
    mov ax, FAT_SEG                 ; start reading from fat
    mov es, ax
    xor bx, bx

    ; get next cluster
    mov ax, WORD [cluster]          ; identify current cluster
    mov cx, ax                      ; copy current cluster
    mov dx, ax                      ; copy current cluster
    shr dx, 1                       ; divide by two
    add cx, dx                      ; sum for (3/2)
    xor bx, bx                      ; location of fat in memory
    add bx, cx
    mov dx, WORD [es:bx]
    test ax, 1                      ; test for odd or even cluster
    jnz .ODD_CLUSTER

.EVEN_CLUSTER:
    and dx, 0000111111111111b       ; take low 12 bits
    jmp .DONE

.ODD_CLUSTER:
    shr dx, 4                       ; take high 12 bits

.DONE:
    mov WORD [cluster], dx
    cmp dx, 0x0ff0                  ; test for EOF
    jb .LOAD_IMAGE

.SUCCESS:
    pop edi
    pop ecx
    xor ax, ax
    ret

%endif
