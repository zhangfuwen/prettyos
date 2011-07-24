[map symbols documentation/apm.map] ; use this for ckernel.c addresses
[bits 16]

org 0x100

CheckAPM:
    mov ah, 0x53
    xor al, al
    xor bx, bx
    mov ds, bx
    int 0x15
    jc .error
        mov byte [0x1300], 0  ; APM is available
        mov word [0x1301], ax ; Save APM version
        mov word [0x1303], bx ; Should contain "PM"
        mov word [0x1305], cx ; APM flags
        jmp exitvm86
    .error:
        mov byte [0x1300], 1 ; APM is not available due to errors
        mov byte [0x1301], ah ; Error code
        jmp exitvm86

APMInstall:
    xor ax, ax
    mov ds, ax
    call APMDisconnect
    call APMConnect
    call APMVersion
    call APMEnable
    mov byte [0x1300], 0 ; No error
    jmp exitvm86


APMDisconnect:
    mov ah, 0x53
    mov al, 0x04
    int 0x15
    jc .poterror ; Successful, if carry flag is not set
        ret
    .poterror:
        cmp ah, 0x03
        jne .error ; Successful, if error is returned because interface was not connected
            ret
        .error:
            mov byte [0x1300], 1  ; Error occured while disconnecting
            mov byte [0x1301], ah ; Error code
            jmp exitvm86

APMConnect:
    mov ah, 0x53
    mov al, 0x01
    xor bx, bx
    int 0x15
    jc .error
        ret
    .error:
        mov byte [0x1300], 2  ; Error occured while connecting
        mov byte [0x1301], ah ; Error code
        jmp exitvm86

APMVersion:
    ; Indicate that we are an APM 1.2 driver
    mov ah, 0x53
    mov al, 0x0E
    xor bx, bx
    mov ch, 1
    mov cl, 2
    int 0x15
    jc .error
        ret
    .error:
        mov byte [0x1300], 3  ; Error occured while handling out APM version
        mov byte [0x1301], ah ; Error code
        jmp exitvm86

APMEnable:
    mov ah, 0x53
    mov al, 0x08
    mov bx, 1
    mov cx, 1
    int 0x15
    jc .error
        ret
    .error:
        mov byte [0x1300], 4  ; Error occured while enabling
        mov byte [0x1301], ah ; Error code
        jmp exitvm86


SetAPMState:
    xor ax, ax
    mov ds, ax
    mov ah, 0x53
    mov al, 0x07
    mov bx, 1
    mov cx, word [0x1300]
    int 0x15
    jc .error
        mov byte [0x1300], 1
        mov byte [0x1301], ah ; Error code
        jmp exitvm86
    .error:
        mov byte [0x1300], 0
        jmp exitvm86


; stop and leave vm86-task
exitvm86:
    hlt          ; is translated as exit() at vm86.c
    jmp exitvm86 ; endless loop
