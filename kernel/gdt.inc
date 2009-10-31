;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Global Descriptor Table (GDT) ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

NULL_Desc:
    dd    0
    dd    0
   
CODE_Desc:
    dw    0xFFFF	; segment length  bits 0-15 ("limit")    
    dw    0		; segment base    byte 0,1      
    db    0		; segment base    byte 2    
    db    10011010b     ; access rights
    db    11001111b     ; bit 7-4: 4 flag bits:  granularity, default operation size bit, 2 bits available for OS
                        ; bit 3-0: segment length bits 16-19 
    db    0             ; segment base    byte 3    

DATA_Desc:
    dw    0xFFFF        ; segment length  bits 0-15
    dw    0             ; segment base    byte 0,1
    db    0             ; segment base    byte 2
    db    10010010b     ; access rights
    db    11001111b     ; bit 7-4: 4 flag bits:  granularity, big bit (0=USE16-Segm., 1=USE32-Segm.), 2 bits avail.
                        ; bit 3-0: segment length bits 16-19
    db    0             ; segment base    byte 3       

gdtr:
Limit    dw 24          ; length of GDT
Base     dd NULL_Desc   ; base of GDT ( linear address: RM Offset + Seg<<4 )
