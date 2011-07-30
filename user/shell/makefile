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
    OBJDIR= ..\..\object_files\user\shell
else
    USERTOOLS= ../user_tools
    STDLIBC= ../stdlibc
    PRETTYLIB= $(USERTOOLS)/prettylib.a
    STDLIBCA= $(STDLIBC)/stdlibc.a
    OBJDIR= ../../object_files/user/shell
endif

# Compiler-/Linker-Flags
NASMFLAGS= -Ox -f elf
GCCFLAGS= -c -std=c99 -march=i386 -Wshadow -m32 -Werror -Wall -s -O -ffreestanding -nostdinc -fno-pic -fno-builtin -fno-stack-protector -fomit-frame-pointer -fno-common -Iinclude
LDFLAGS= -u _start -nostdlib --warn-common -nmagic

TARGET= SHELL.ELF

# targets to build one asm or c-file to an object file
vpath %.o $(OBJDIR)
%.o: %.c
	$(GCC) $< $(GCCFLAGS) -I $(STDLIBC) -I $(USERTOOLS) -o $(OBJDIR)/$@

# dependancies
TARGETOBJ := $(patsubst %.c, %.o, $(wildcard *.c))

# targets to build the userprog
.PHONY: clean all

all: $(TARGET)

$(TARGET): $(TARGETOBJ) $(PRETTYLIB) $(STDLIBCA)
	$(LD) $(addprefix $(OBJDIR)/,$(TARGETOBJ)) $(STDLIBC)/stdlibc.a $(USERTOOLS)/prettylib.a -T $(USERTOOLS)/user.ld -Map ../../documentation/shell.map $(LDFLAGS) -o $(TARGET)
	$(STRIP) $(TARGET)

clean:
	$(RM) *.ELF
ifeq ($(OS),WINDOWS)
	$(RM) $(OBJDIR)\*.o
else
	$(RM) $(OBJDIR)/*.o
endif