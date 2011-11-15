[BITS 32]
extern main
extern exit

global _start
global _syscall

_syscall dd syscall         ; Function pointer. Points to syscall per default
oldEsp dd 0                 ; Contains esp in case of sysenter

_start:
    push ecx                ; argv
    push eax                ; argc
    cmp edx, 0              ; Set to 1 if CPU supports sysenter
    je .done
        mov eax, sysenter   ; Set function pointer to sysenter, if its supported
        mov [_syscall], eax
    .done:
    call main               ; Start programm
    call exit               ; Cleanup. Deletes this task. (Syscall)
    jmp  $

syscall:
    int 0x7F                ; Call kernel by issueing an interrupt
    ret

sysenter:
    push ebp
    mov [oldEsp], esp       ; Save esp
    mov ebp, .done          ; Write return address to ebp
    sysenter                ; Call kernel by executing sysenter instruction
    .done:
    mov esp, [oldEsp]       ; Restore esp
    pop ebp
    ret
