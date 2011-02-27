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
    STDLIBC= ..\stdlibc
    OBJDIR= ..\..\object_files\user\user_tools
else
    STDLIBC= ../stdlibc
    OBJDIR= ../../object_files/user/user_tools
endif

# Compiler-/Linker-Flags
NASMFLAGS= -Ox -f elf
GCCFLAGS= -c -std=c99 -march=i386 -Wshadow -m32 -Werror -Wall -s -O -ffreestanding -nostdinc -fno-pic -fno-builtin -fno-stack-protector -fno-common -Iinclude
GPPFLAGS= -c -std=c++0x -march=i386 -Wshadow -m32 -Werror -Wall -s -O -ffreestanding -nostdinc -fno-pic -fno-builtin -fno-stack-protector -fno-common -Iinclude -fno-exceptions -fno-rtti -lgcc
ARFLAGS= rs

CTARGET= prettylib.a
CPPTARGET= prettylib++.a

# targets to build one asm or c-file to an object file
vpath %.o $(OBJDIR)
%.o: %.c
	$(GCC) $< $(GCCFLAGS) -I $(STDLIBC) -o $(OBJDIR)/$@
%.o: %.cpp
	$(GCC) $< $(GPPFLAGS) -I $(STDLIBC) -o $(OBJDIR)/$@
%.o: %.asm
	$(NASM) $< $(NASMFLAGS) -o $(OBJDIR)/$@

# dependancies
CTARGETOBJ := $(patsubst %.c, %.o, $(wildcard *.c)) $(patsubst %.asm, %.o, $(wildcard *.asm))
CPPTARGETOBJ := $(patsubst %.cpp, %.o, $(wildcard *.cpp))

# targets to build the userprog
.PHONY: clean all

all: $(CTARGET) $(CPPTARGET)

$(CTARGET): $(CTARGETOBJ)
	$(AR) $(ARFLAGS) $(CTARGET) $(addprefix $(OBJDIR)/,$(CTARGETOBJ))

$(CPPTARGET): $(CPPTARGETOBJ)
	$(AR) $(ARFLAGS) $(CPPTARGET) $(addprefix $(OBJDIR)/,$(CPPTARGETOBJ))

clean:
ifeq ($(OS),WINDOWS)
	$(RM) $(OBJDIR)\*.o
else
	$(RM) $(OBJDIR)/*.o
endif