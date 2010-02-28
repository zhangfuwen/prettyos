USERLIB=../user_program_c/

ifeq ($(OS),WINDOWS)
    RM=del
    NASM= ../../tools/nasmw
    GCC= i586-elf-gcc
    LD= i586-elf-ld
else
    RM=rm -f
    NASM=nasm
    GCC=gcc
    LD=ld
endif

main:
	$(NASM) -O32 -f elf start.asm -o start.o
	$(GCC) -c $(USERLIB)\userlib.c -std=c99 -march=i386 -Wshadow -mtune=i386 -m32 -fno-pic -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Iinclude
	$(GCC) -c *.c -std=c99 -march=i386 -I$(USERLIB) -mtune=i386 -m32 -fno-pic -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Iinclude
	$(LD) *.o -T user.ld -Map user.map -nostdinc -o HELLO.ELF
	$(RM) *.o
	