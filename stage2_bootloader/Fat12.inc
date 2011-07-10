%ifndef FAT12_INC
%define FAT12_INC

[BITS 16]

datasector  dw 0x0000
cluster     dw 0x0000

Sector      db 0x00
Head        db 0x00
Cylinder    db 0x00

BytesPerSec  dw 512
SecPerClus   db 1
ReservedSec  dw 1
NumFATs      db 2
RootEntries  dw 224
FATSize      dw 9
SecPerTrack  dw 18
NumHeads     dw 2
DriveNum     db 0

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
; FS:EDI  Buffer to read to
;******************************************************************************

ReadSectors:
    push    dx
    push    bx
    push    es
    mov     dx, 0x0F00                          ; enough space for up to 4096 byte sectors
    mov     es, dx
.MAIN:
        mov     bp, 0x0005                      ; five retries for error
        push    ax
        push    cx
        call    Convert_LBA_to_CHS              ; convert sector to CHS
    .SECTORLOOP:
            mov     ah, 0x02                    ; BIOS read sector
            mov     al, 0x01                    ; read one sector
            xor     bx, bx
            mov     ch, BYTE [Cylinder]         ; track
            mov     cl, BYTE [Sector]           ; sector
            mov     dh, BYTE [Head]             ; head
            mov     dl, BYTE [DriveNum]         ; drive
            int     0x13                        ; invoke BIOS
            jnc     .SUCCESS                    ; test for read error. If not, leave loop
            xor     ax, ax                      ; ERROR case: BIOS reset disk
            int     0x13                        ; invoke BIOS
            dec     bp                          ; decrement error counter
            jnz     .SECTORLOOP                 ; attempt to read again
        int     0x18                            ; Indicate BOOT ERROR.
    .SUCCESS:
        mov     cx, WORD [BytesPerSec]
        shr     cx, 2
        xor     si, si
        .COPY_TO_DEST:                          ; copy to destination buffer
            es lodsd                            ; Load dword from es:esi and increment esi
            mov     [fs:edi], eax
            add     edi, 4
            loop    .COPY_TO_DEST
        pop     cx
        pop     ax
        inc     ax                              ; queue next sector
        loop    .MAIN                           ; read next sector
    pop     es
    pop     bx
    pop     dx
    ret

;******************************************************************************
; Defines
;******************************************************************************

%define ROOT_OFFSET  0x02e00
%define FAT_DEST     0x02c00
%define FAT_SEG      0x002c0
%define ROOT_DEST    0x02e00
%define ROOT_SEG     0x002e0

;******************************************************************************
; LoadRoot
; Transfers Root Directory Table into memory at ROOT_DEST
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
    mov     al, BYTE [NumFATs]                  ; number of FATs
    mul     WORD [FATSize]                      ; sectors used by FATs
    add     ax, WORD [ReservedSec]
    mov     WORD [datasector], ax               ; base of root directory
    add     WORD [datasector], cx

    ; read root directory into ROOT_DEST
    mov     edi, ROOT_DEST
    call    ReadSectors                         ; read directory table
    popa
    ret

;******************************************************************************
; LoadFAT
; Loads FAT table into memory at FAT_DEST
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

    ; read FAT into memory
    mov edi, FAT_DEST
    call ReadSectors
    popa
    ret

;******************************************************************************
; FindFile
; Search for filename in root table
; DS:SI  File name
; AX     Return value: File index number in directory table (error: -1)
;******************************************************************************

FindFile:
    push cx                     ; store registers

    ; browse root directory for binary image
    mov cx, WORD [RootEntries]  ; load loop counter
    mov di, ROOT_OFFSET         ; locate first root entry at 1 MB mark
    cld                         ; clear direction flag

.LOOP2:
    push cx
    mov cx, 11                  ; eleven character name
    push si                     ; image name is in SI
    push di
    rep cmpsb                   ; test for entry match
    pop di
    pop si
    je .Found
    pop cx
    add di, 32                  ; queue next directory entry
    loop .LOOP2

.NotFound:
    pop cx                      ; restore registers and return
    mov ax, -1                  ; set error code
    ret

.Found:
    pop ax                      ; return value into AX contains entry of file
    pop cx                      ; restore registers and return
    ret

;******************************************************************************
; LoadFile
; ES:SI   File to load
; FS:EDI  Buffer to load file to
; AX      Return value: success 0, error -1
;******************************************************************************

LoadFile:
    push edi

.FIND_FILE:
    call FindFile                   ; find our file. ES:SI contains our filename
    cmp ax, -1
    jne .LOAD_IMAGE_PRE

    pop edi
    mov ax, -1                      ; Error
    ret

.LOAD_IMAGE_PRE:
    sub edi, ROOT_OFFSET

    ; get starting cluster
    mov dx, word ROOT_SEG           ; root segment loc
    mov es, dx
    mov dx, WORD [es:di + 0x001A]   ; DI points to file entry in root directory table. Reference the table...
    mov WORD [cluster], dx          ; file's first cluster
    call LoadFAT

.LOAD_IMAGE:
    ; load the cluster
    mov ax, WORD [cluster]          ; cluster to read
    call Convert_Cluster_to_LBA
    xor ch, ch
    mov cl, BYTE [SecPerClus]

    pop edi
    call ReadSectors

    push edi
    mov ax, FAT_SEG                 ; start reading from fat
    mov es, ax

    ; get next cluster
    mov ax, WORD [cluster]          ; identify current cluster
    mov cx, ax                      ; copy current cluster
    mov dx, ax                      ; copy current cluster
    shr dx, 1                       ; divide by two
    add cx, dx                      ; sum for (3/2)
    mov bx, cx                      ; location of fat in memory
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
    xor ax, ax                      ; No error.
    ret

%endif
