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
ifeq ($(OS),WINDOWS)
    USERTOOLS= ..\user_tools
    STDLIBC= ..\stdlibc
    PRETTYLIB= $(USERTOOLS)\prettylib.a
    STDLIBCA= $(STDLIBC)\stdlibc.a
    OBJDIR= ..\..\object_files\user\other_userprogs
else
    USERTOOLS= ../user_tools
    STDLIBC= ../stdlibc
    PRETTYLIB= $(USERTOOLS)/prettylib.a
    STDLIBCA= $(STDLIBC)/stdlibc.a
    OBJDIR= ../../object_files/user/other_userprogs
endif

# Compiler-/Linker-Flags
NASMFLAGS= -Ox -f elf
GCCFLAGS= -c -std=c99 -march=i386 -Wshadow -m32 -Werror -Wall -s -O -ffreestanding -nostdinc -fno-pic -fno-builtin -fno-stack-protector -fno-common -Iinclude
LDFLAGS= -u _start -nostdlib --warn-common -nmagic

# targets to build one asm or c-file to an object file
vpath %.o $(OBJDIR)
%.o: %.c
	$(GCC) $< $(GCCFLAGS) -I $(STDLIBC) -I $(USERTOOLS) -o $(OBJDIR)/$@
%.ELF: %.o $(PRETTYLIB) $(STDLIBCA)
	$(LD) $< $(USERTOOLS)/prettylib.a $(STDLIBC)/stdlibc.a -T $(USERTOOLS)/user.ld $(LDFLAGS) -o $@
	$(STRIP) $@

# dependancies
TARGETOBJ := $(patsubst %.c, %.o, $(wildcard *.c))
TARGETEXE := $(patsubst $(OBJDIR)/%.o, %.ELF, $(wildcard $(OBJDIR)/*.o))

# targets to build the userprog
.PHONY: clean all

all: $(TARGETOBJ) $(TARGETEXE)

clean:
ifeq ($(OS),WINDOWS)
	$(RM) $(OBJDIR)\*.o
else
	$(RM) $(OBJDIR)/*.o
endif