; data for ramdisk
global _file_data_start
global _file_data_end
_file_data_start:
incbin "initrd.dat"
_file_data_end:

; --------------------- VM86 -------------------------------------------------------------------------------
global _vm86_com_start
global _vm86_com_end
_vm86_com_start:
incbin "user/vm86/vidswtch.com"
_vm86_com_end:
; --------------------- VM86 -------------------------------------------------------------------------------
