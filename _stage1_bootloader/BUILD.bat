
..\_nasm\nasmw -f bin boot.asm -o boot.bin
dd if=boot.bin of=\\.\A: bs=512 count=1 --progress

pause
