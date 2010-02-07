main: hello.c userlib.c userlib.h
	nasmw -O32 -f elf start.asm -o start.o
	i586-elf-gcc -c userlib.c -std=c99 -march=i386 -mtune=i386 -m32 -fno-pic -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Iinclude
	i586-elf-gcc -c hello.c -std=c99 -march=i386 -mtune=i386 -m32 -fno-pic -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Iinclude
	i586-elf-ld *.o -T user.ld -Map user.map -nostdinc -o HELLO.ELF
	rm -f *.o