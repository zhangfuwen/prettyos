..\_nasm\nasmw -f bin -I..\_stage1_bootloader\ ..\_stage1_bootloader\boot.asm -o ..\_stage1_bootloader\boot.bin
..\_nasm\nasmw -f bin -I..\_stage2_bootloader\ ..\_stage2_bootloader\boot2.asm -o ..\_stage2_bootloader\BOOT2.SYS
..\_mingw32-make\mingw32-make --makefile=Windows_makefile
