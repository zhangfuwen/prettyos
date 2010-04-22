# Define OS-dependant Tools
ifeq ($(OS),WINDOWS)
    RM= - del
    NASM= nasm
    GCC= i586-elf-gcc
    LD= i586-elf-ld
else
    RM= rm -f
    NASM= nasm
    ifeq ($(OS),MACOSX)
        GCC= i586-elf-gcc
        LD= i586-elf-ld
    else
        GCC= gcc
        LD= ld
    endif
endif

# Folders
USERTOOLS=../user_tools

# dependancies

# Compiler-/Linker-Flags
NASMFLAGS= -O32 -f elf
GCCFLAGS= -c -march=i386 -Wshadow -mtune=i386 -m32 -Werror -Wall -s -O -ffreestanding -fleading-underscore -nostdinc -fno-pic -fno-builtin -fno-stack-protector -fno-common -Iinclude
LDFLAGS= -nostdlib --warn-common -nmagic

# targets to build one asm or c-file to an object file

# targets to build the userprog
.PHONY: clean main

main:
	$(NASM) $(USERTOOLS)/start.asm $(NASMFLAGS) -o start.o
	$(GCC) -c $(USERTOOLS)/userlib.c -std=c99 $(GCCFLAGS)
	$(GCC) -c $(USERTOOLS)/userlib_cpp.cpp $(GCCFLAGS) -fno-exceptions -fno-rtti -lgcc
	$(GCC) -c *.cpp $(GCCFLAGS) -fno-exceptions -fno-rtti -lgcc -I$(USERTOOLS)
	$(LD) *.o -T $(USERTOOLS)/user.ld -Map user.map $(LDFLAGS) -o CPP.ELF
	$(RM) *.o

clean:
	$(RM) *.o