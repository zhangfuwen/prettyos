USERTOOLS=../user_tools

ifeq ($(OS),WINDOWS)
    RM=del
    NASM= ../../tools/nasm
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
	$(GCC) -c $(USERTOOLS)/userlib.c -std=c99 -march=i386 -Wshadow -mtune=i386 -m32 -fno-pic -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Iinclude
	$(GCC) -c *.cpp -march=i386 -Wshadow -mtune=i386 -m32 -lgcc -fno-exceptions -fno-rtti -fno-pic -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Werror -Iinclude -I$(USERTOOLS)
	$(LD) *.o -T $(USERTOOLS)/user.ld -Map user.map -nostdinc -o CPP.ELF
	$(RM) *.o
	