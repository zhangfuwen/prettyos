[bits 16]
[section .text]
org 0x100

; switch video mode
mov ax, 0013h
int 10h

mov ax, 0xa000
mov ds, ax

; first row
mov word [700], 14 ; yellow pixel at x=60 and y=3
mov word [701], 14
mov word [702], 14
mov word [703], 14
mov word [704], 14
mov word [717], 14
mov word [721], 14
mov word [732], 14
mov word [733], 14
mov word [734], 14
mov word [735], 14
mov word [737], 14
mov word [738], 14
mov word [739], 14
mov word [740], 14

; second row
mov word [320+701], 14
mov word [320+704], 14
mov word [320+717], 14
mov word [320+721], 14
mov word [320+732], 14
mov word [320+735], 14
mov word [320+737], 14

; third row
mov word [640+701], 14
mov word [640+704], 14
mov word [640+706], 14
mov word [640+708], 14
mov word [640+709], 14
mov word [640+711], 14
mov word [640+712], 14
mov word [640+713], 14
mov word [640+714], 14
mov word [640+716], 14
mov word [640+717], 14
mov word [640+718], 14
mov word [640+720], 14
mov word [640+721], 14
mov word [640+722], 14
mov word [640+724], 14
mov word [640+727], 14
mov word [640+732], 14
mov word [640+735], 14
mov word [640+737], 14

; fourth row
mov word [960+701], 14
mov word [960+702], 14
mov word [960+703], 14
mov word [960+704], 14
mov word [960+706], 14
mov word [960+707], 14
mov word [960+708], 14
mov word [960+711], 14
mov word [960+714], 14
mov word [960+717], 14
mov word [960+721], 14
mov word [960+724], 14
mov word [960+727], 14
mov word [960+732], 14
mov word [960+735], 14
mov word [960+737], 14
mov word [960+738], 14
mov word [960+739], 14
mov word [960+740], 14

; fifth row
mov word [1280+701], 14
mov word [1280+706], 14
mov word [1280+711], 14
mov word [1280+712], 14
mov word [1280+713], 14
mov word [1280+714], 14
mov word [1280+717], 14
mov word [1280+721], 14
mov word [1280+724], 14
mov word [1280+727], 14
mov word [1280+732], 14
mov word [1280+735], 14
mov word [1280+740], 14

; sixth row
mov word [1600+701], 14
mov word [1600+706], 14
mov word [1600+711], 14
mov word [1600+717], 14
mov word [1600+721], 14
mov word [1600+724], 14
mov word [1600+727], 14
mov word [1600+732], 14
mov word [1600+735], 14
mov word [1600+740], 14

; seventh row
mov word [1920+701], 14
mov word [1920+706], 14
mov word [1920+711], 14
mov word [1920+714], 14
mov word [1920+717], 14
mov word [1920+721], 14
mov word [1920+724], 14
mov word [1920+727], 14
mov word [1920+732], 14
mov word [1920+735], 14
mov word [1920+740], 14

; eightth row
mov word [2240+701], 14
mov word [2240+706], 14
mov word [2240+711], 14
mov word [2240+712], 14
mov word [2240+713], 14
mov word [2240+714], 14
mov word [2240+717], 14
mov word [2240+718], 14
mov word [2240+721], 14
mov word [2240+722], 14
mov word [2240+724], 14
mov word [2240+725], 14
mov word [2240+726], 14
mov word [2240+727], 14
mov word [2240+732], 14
mov word [2240+733], 14
mov word [2240+734], 14
mov word [2240+735], 14
mov word [2240+737], 14
mov word [2240+738], 14
mov word [2240+739], 14
mov word [2240+740], 14

;ninth row
mov word [2560+727], 14

;tenth row
mov word [2880+725], 14
mov word [2880+727], 14

;eleventh row
mov word [3200+725], 14
mov word [3200+726], 14
mov word [3200+727], 14

call waiting_loop

xor ax, ax
mov ds, ax

; set back to 80x50 text mode and 8x8 font
mov ax, 0x1112
xor bl, bl
int 0x10

hlt ; is translated as exit() at vm86.c
jmp $ ; endless loop

waiting_loop:                   
       mov dl,0x0009  
L3:	   mov bl,0x00FF  
L2:    mov cx,0xFFFF
L1:    dec cx                   
       jnz L1
       dec bl
       jnz L2
	   dec dl
	   jnz L3
       ret                

