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
    OBJDIR= ..\..\object_files\user\dummy_c
else
    USERTOOLS= ../user_tools
    STDLIBC= ../stdlibc
    OBJDIR= ../../object_files/user/dummy_c
endif

# Compiler-/Linker-Flags
NASMFLAGS= -Ox -f elf
GCCFLAGS= -c -march=i386 -Wshadow -mtune=i386 -m32 -Werror -Wall -s -O -ffreestanding -nostdinc -fno-pic -fno-builtin -fno-stack-protector -fno-common -Iinclude
LDFLAGS= -nostdlib --warn-common -nmagic

TARGET= CPPDUMMY.ELF

# targets to build one asm or c-file to an object file
vpath %.o $(OBJDIR)
%.o: %.c
	$(GCC) $< $(GCCFLAGS) -std=c99 -I $(STDLIBC) -I $(USERTOOLS) -o $(OBJDIR)/$@
%.o: %.cpp
	$(GCC) $< $(GCCFLAGS) -fno-exceptions -std=c++0x -fno-rtti -lgcc -I $(STDLIBC) -I $(USERTOOLS) -o $(OBJDIR)/$@
%.o: %.asm
	$(NASM) $< $(NASMFLAGS) -o $(OBJDIR)/$@

# dependancies
TARGETOBJ := $(patsubst %.cpp, %.o, $(wildcard *.cpp $(USERTOOLS)/*.cpp)) $(patsubst %.c, %.o, $(wildcard *.c $(USERTOOLS)/*.c $(STDLIBC)/*.c)) $(patsubst %.asm, %.o, $(wildcard $(USERTOOLS)/start.asm))

# targets to build the userprog
.PHONY: clean all

all: $(TARGET)

$(TARGET): $(TARGETOBJ)
	$(LD) $(addprefix $(OBJDIR)/,$(TARGETOBJ)) -T $(USERTOOLS)/user.ld $(LDFLAGS) -o $(TARGET)
	$(STRIP) $(TARGET)

clean:
ifeq ($(OS),WINDOWS)
	$(RM) $(OBJDIR)\*.o
else
	$(RM) $(OBJDIR)/*.o
endif
