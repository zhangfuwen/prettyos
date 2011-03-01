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
    OBJDIR= ..\..\object_files\user\dummy_cpp
else
    USERTOOLS= ../user_tools
    STDLIBC= ../stdlibc
    PRETTYLIB= $(USERTOOLS)/prettylib.a
    STDLIBCA= $(STDLIBC)/stdlibc.a
    OBJDIR= ../../object_files/user/dummy_cpp
endif

# Compiler-/Linker-Flags
NASMFLAGS= -Ox -f elf
GPPFLAGS= -c -std=c++0x -march=i386 -Wshadow -m32 -Werror -Wall -s -O -ffreestanding -nostdinc -fno-pic -fno-builtin -fno-stack-protector -fno-common -Iinclude -fno-exceptions -fno-rtti -lgcc
LDFLAGS= -u _start -nostdlib --warn-common -nmagic

TARGET= CPPDUMMY.ELF

# targets to build one asm or c-file to an object file
vpath %.o $(OBJDIR)
%.o: %.cpp
	$(GCC) $< $(GPPFLAGS) -I $(STDLIBC) -I $(USERTOOLS) -o $(OBJDIR)/$@

# dependancies
TARGETOBJ := $(patsubst %.cpp, %.o, $(wildcard *.cpp))

# targets to build the userprog
.PHONY: clean all

all: $(TARGET)

$(TARGET): $(TARGETOBJ) $(PRETTYLIB) $(STDLIBCA)
	$(LD) $(addprefix $(OBJDIR)/,$(TARGETOBJ)) $(USERTOOLS)/prettylib.a $(USERTOOLS)/prettylib++.a $(STDLIBC)/stdlibc.a -T $(USERTOOLS)/user.ld $(LDFLAGS) -o $(TARGET)
	$(STRIP) $(TARGET)

clean:
ifeq ($(OS),WINDOWS)
	$(RM) $(OBJDIR)\*.o
else
	$(RM) $(OBJDIR)/*.o
endif
