USERTOOLS=../user_tools

ifeq ($(OS),WINDOWS)
    RM=del
    NASM= nasm
    GCC= i586-elf-gcc
    LD= i586-elf-ld
else
    RM=rm -f
    NASM=nasm
    GCC=gcc
    LD=ld
endif

main:
	$(NASM) -O32 -f elf $(USERTOOLS)/start.asm -o start.o
	$(GCC) -c $(USERTOOLS)/userlib.c -std=c99 -march=i386 -Wshadow -mtune=i386 -m32 -fno-pic -Werror -Wall -O -ffreestanding -fleading-underscore -nostdinc -fno-builtin -fno-stack-protector -fno-common -Iinclude
	$(GCC) -c *.c -std=c99 -march=i386 -Wshadow -mtune=i386 -m32 -fno-pic -Werror -Wall -O -ffreestanding -fleading-underscore -nostdinc -fno-builtin -fno-stack-protector -fno-common -Iinclude -I$(USERTOOLS)
	$(LD) *.o -T $(USERTOOLS)/user.ld -Map user.map -nostdlib -nmagic --warn-common -o HELLO.ELF
	$(RM) *.o
	