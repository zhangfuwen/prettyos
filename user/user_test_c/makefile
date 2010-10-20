# Define OS-dependant Tools
ifeq ($(OS),WINDOWS)
    RM= - del
    NASM= nasm
    GCC= i586-elf-gcc
    LD= i586-elf-ld
    STRIP= i586-elf-strip
else
    RM= rm -f
    NASM= nasm
    ifeq ($(OS),MACOSX)
        GCC= i586-elf-gcc
        LD= i586-elf-ld
        STRIP= i586-elf-strip
    else
        GCC= gcc
        LD= ld
        STRIP= strip
    endif
endif

# Folders
USERTOOLS=../user_tools
STDLIBC=../stdlibc

# dependancies

# Compiler-/Linker-Flags
NASMFLAGS= -Ox -f elf
GCCFLAGS= -c -std=c99 -march=i386 -Wshadow -mtune=i386 -m32 -Werror -Wall -s -O -ffreestanding -fleading-underscore -nostdinc -fno-pic -fno-builtin -fno-stack-protector -fno-common -Iinclude -I $(STDLIBC)
LDFLAGS= -nostdlib --warn-common -nmagic

# targets to build one asm or c-file to an object file

# targets to build the userprog
.PHONY: clean main

main:
	$(NASM) $(USERTOOLS)/start.asm $(NASMFLAGS) -o start.o
	$(GCC) -c $(STDLIBC)/*.c $(GCCFLAGS)
	$(GCC) -c $(USERTOOLS)/userlib.c $(GCCFLAGS)
	$(GCC) -c *.c $(GCCFLAGS) -I $(USERTOOLS)
	$(LD) *.o -T $(USERTOOLS)/user.ld -Map user.map $(LDFLAGS) -o HELLO.ELF
	$(STRIP) HELLO.ELF
	$(RM) *.o

clean:
	$(RM) *.o