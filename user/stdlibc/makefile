# Define OS-dependant Tools
ifeq ($(OS),WINDOWS)
    RM= - del
    NASM= nasm
    GCC= i586-elf-gcc
    AR= i586-elf-ar
else
    RM= rm -f
    NASM= nasm
    ifeq ($(OS),MACOSX)
        GCC= i586-elf-gcc
        AR= i586-elf-ar
    else
        GCC= gcc
        AR= ar
    endif
endif

# Folders
ifeq ($(OS),WINDOWS)
    OBJDIR= ..\..\object_files\user\stdlibc
else
    OBJDIR= ../../object_files/user/stdlibc
endif

# Compiler-/Linker-Flags
GCCFLAGS= -c -std=c99 -march=i386 -Wshadow -m32 -Werror -Wall -s -O -ffreestanding -nostdinc -fno-pic -fno-builtin -fno-stack-protector -fomit-frame-pointer -fno-common -Iinclude
ARFLAGS= rs

TARGET= stdlibc.a

# targets to build one asm or c-file to an object file
vpath %.o $(OBJDIR)
%.o: %.c
	$(GCC) $< $(GCCFLAGS) -o $(OBJDIR)/$@

# dependancies
TARGETOBJ := $(patsubst %.c, %.o, $(wildcard *.c))

# targets to build the userprog
.PHONY: clean all

all: $(TARGET)

$(TARGET): $(TARGETOBJ)
	$(AR) $(ARFLAGS) $(TARGET) $(addprefix $(OBJDIR)/,$(TARGETOBJ))

clean:
ifeq ($(OS),WINDOWS)
	$(RM) $(OBJDIR)\*.o
else
	$(RM) $(OBJDIR)/*.o
endif