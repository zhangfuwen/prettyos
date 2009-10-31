; data for ramdisk
global _file_data_start
global _file_data_end
_file_data_start:
incbin "initrd.dat"
_file_data_end:

