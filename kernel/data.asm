; --------------------- data for ramdisk ---------------------------------------
global _file_data_start
global _file_data_end
_file_data_start:
incbin "initrd.dat"
_file_data_end:
; --------------------- data for ramdisk ---------------------------------------

; --------------------- VM86 ---------------------------------------------------
global _vidswtch_com_start
global _vidswtch_com_end
global _apm_com_start
global _apm_com_end
_vidswtch_com_start:
incbin "user/vm86/VIDSWTCH.COM"
_vidswtch_com_end:
_apm_com_start:
incbin "user/vm86/APM.COM"
_apm_com_end:
; --------------------- VM86 ---------------------------------------------------

;---------------------- bmp ----------------------------------------------------
global _bmp_start
global _bmp_end
_bmp_start:
incbin "user/vm86/bootscr.bmp"
_bmp_end:
;---------------------- bmp ----------------------------------------------------

;---------------------- bmp cursor ---------------------------------------------
global _cursor_start
global _cursor_end
_cursor_start:
incbin "user/vm86/cursor.bmp"
_cursor_end:
;---------------------- bmp cursor ---------------------------------------------