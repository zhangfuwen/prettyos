
..\_nasm\nasmw -f bin boot.asm -o boot.bin
PARTCOPY boot.bin 0 3 -f0 0 
PARTCOPY boot.bin 3E 1C2 -f0 3E 

pause
