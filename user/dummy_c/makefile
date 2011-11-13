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
    OBJDIR= ..\..\object_files\user\dummy_c
else
    USERTOOLS= ../user_tools
    STDLIBC= ../stdlibc
    PRETTYLIB= $(USERTOOLS)/prettylib.a
    STDLIBCA= $(STDLIBC)/stdlibc.a
    OBJDIR= ../../object_files/user/dummy_c
endif

# Compiler-/Linker-Flags
ifeq ($(COMPILER),CLANG)
	CCFLAGS= -c -std=c99 -march=i486 -Wshadow -m32 -Werror -Wall -O -ffreestanding -nostdinc -fno-builtin -fno-stack-protector -fomit-frame-pointer -fno-common -Iinclude -Xclang -triple=i386-pc-unknown
else
	CCFLAGS= -c -std=c99 -march=i486 -Wshadow -m32 -Werror -Wall -O -ffreestanding -nostdinc -fno-pic -fno-builtin -fno-stack-protector -fomit-frame-pointer -fno-common -Iinclude
endif
LDFLAGS= -u _start -nostdlib --warn-common -nmagic -gc-sections -s

TARGET= CDUMMY.ELF

# targets to build one asm or c-file to an object file
vpath %.o $(OBJDIR)
%.o: %.c
	$(CC) $< $(CCFLAGS) -I $(STDLIBC) -I $(USERTOOLS) -o $(OBJDIR)/$@

# dependancies
TARGETOBJ := $(patsubst %.c, %.o, $(wildcard *.c))

# targets to build the userprog
.PHONY: clean all

all: $(TARGET)

$(TARGET): $(TARGETOBJ) $(PRETTYLIB) $(STDLIBCA)
	$(LD) $(addprefix $(OBJDIR)/,$(TARGETOBJ)) $(STDLIBCA) $(PRETTYLIB) -T $(USERTOOLS)/user.ld $(LDFLAGS) -o $(TARGET)

clean:
ifeq ($(OS),WINDOWS)
	$(RM) $(OBJDIR)\*.o
else
	$(RM) $(OBJDIR)/*.o
endif