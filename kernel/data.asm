; --------------------- data for ramdisk ---------------------------------------
global _file_data_start
global _file_data_end
_file_data_start:
incbin "initrd.dat"
_file_data_end:
; --------------------- data for ramdisk ---------------------------------------

; --------------------- VM86 ---------------------------------------------------
global _vm86_com_start
global _vm86_com_end
_vm86_com_start:
incbin "user/vm86/VIDSWTCH.COM"
_vm86_com_end:
; --------------------- VM86 ---------------------------------------------------

;---------------------- bmp ----------------------------------------------------
global _bmp_start
global _bmp_end
_bmp_start:
incbin "user/vm86/bootscr.bmp"
_bmp_end:
;---------------------- bmp ----------------------------------------------------

;---------------------- font ---------------------------------------------------
global _font_start
global _font_end
_font_start:
incbin "user/vm86/font.bmp"
_font_end:
;---------------------- font ---------------------------------------------------