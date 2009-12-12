..\nasm\nasmw -f bin -I..\stage1_bootloader\ ..\stage1_bootloader\boot.asm -o ..\stage1_bootloader\boot.bin
..\nasm\nasmw -f bin -I..\stage2_bootloader\ ..\stage2_bootloader\boot2.asm -o ..\stage2_bootloader\BOOT2.SYS
..\mingw32-make\mingw32-make --makefile=Windows_makefile
