# Define OS-dependant Tools
ifeq ($(OS),WINDOWS)
    RM= - del
    NASM= nasm
    CC= i586-elf-gcc
    LD= i586-elf-ld
else
    RM= rm -f
    NASM= nasm
    ifeq ($(OS),MACOSX)
        CC= i586-elf-gcc
        LD= i586-elf-ld
    else
        CC= gcc
        LD= ld
    endif
endif

ifeq ($(COMPILER),CLANG)
	CC= clang
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
ifeq ($(COMPILER),CLANG)
	CCFLAGS= -c -std=c99 -march=i486 -Wshadow -m32 -Werror -Wall -O -ffreestanding -nostdinc -fno-builtin -fno-stack-protector -fomit-frame-pointer -fno-common -Iinclude -Xclang -triple=i386-pc-unknown
else
	CCFLAGS= -c -std=c99 -march=i486 -Wshadow -m32 -Werror -Wall -O -ffreestanding -nostdinc -fno-pic -fno-builtin -fno-stack-protector -fomit-frame-pointer -fno-common -Iinclude
endif
LDFLAGS= -u _start -nostdlib --warn-common -nmagic -gc-sections -s

# targets to build an executable from a source file
%.ELF: %.c $(STDLIBCA) $(PRETTYLIB)
	$(CC) $< $(CCFLAGS) -I $(STDLIBC) -I $(USERTOOLS) -o $(OBJDIR)/$<.o
	$(LD) $(OBJDIR)/$<.o $(STDLIBCA) $(PRETTYLIB) -T $(USERTOOLS)/user.ld $(LDFLAGS) -Map ../../documentation/user_$@.map -o $@

# dependancies
TARGETEXE := $(patsubst %.c, %.ELF, $(wildcard *.c))

# targets to build the userprog
.PHONY: clean all

all: $(TARGETEXE)

clean:
	$(RM) *.ELF
ifeq ($(OS),WINDOWS)
	$(RM) $(OBJDIR)\*.o
else
	$(RM) $(OBJDIR)/*.o
endif