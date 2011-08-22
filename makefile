# Define OS-dependant Tools
NASM= nasm
ifeq ($(OS),WINDOWS)
	RM= - del /s
	MV= cmd /c move /Y
	CC= i586-elf-gcc
	LD= i586-elf-ld
	STRIP= i586-elf-strip
	FLOPPYIMAGE= tools/CreateFloppyImage2
	MKINITRD= tools/make_initrd
else
	RM= rm -f
	MV= mv
	ifeq ($(OS),MACOSX)
		CC= i586-elf-gcc
		LD= i586-elf-ld
		STRIP= i586-elf-strip
		FLOPPYIMAGE= tools/osx_CreateFloppyImage2
		MKINITRD= tools/osx_make_initrd
	else
		CC= gcc
		LD= ld
		STRIP= strip
		FLOPPYIMAGE= tools/linux_CreateFloppyImage2
		MKINITRD= tools/linux_make_initrd
	endif
endif

ifeq ($(COMPILER),CLANG)
	CC= clang
endif

# Folders
OBJDIR= object_files
STAGE1DIR= stage1_bootloader
STAGE2DIR= stage2_bootloader
KERNELDIR= kernel
USERDIR= user

ifeq ($(OS),WINDOWS)
	SHELLDIR= $(USERDIR)\shell
	USERPROGDIR= $(USERDIR)\other_userprogs
	USERRDDIR= $(USERDIR)\init_rd_img
	USERTOOLS= $(USERDIR)\user_tools
	STDLIBC= $(USERDIR)\stdlibc
else
	SHELLDIR= $(USERDIR)/shell
	USERPROGDIR= $(USERDIR)/other_userprogs
	USERRDDIR= $(USERDIR)/init_rd_img
	USERTOOLS= $(USERDIR)/user_tools
	STDLIBC= $(USERDIR)/stdlibc
endif

# dependancies
KERNEL_OBJECTS := $(patsubst %.c, %.o, $(wildcard $(KERNELDIR)/*.c $(KERNELDIR)/cdi/*.c $(KERNELDIR)/video/*.c $(KERNELDIR)/storage/*.c $(KERNELDIR)/filesystem/*.c $(KERNELDIR)/network/*.c $(KERNELDIR)/netprotocol/*.c $(KERNELDIR)/audio/*.c)) $(patsubst %.asm, %.o, $(wildcard $(KERNELDIR)/*.asm))

# Compiler-/Linker-Flags
NASMFLAGS= -Ox -f elf
ifeq ($(COMPILER),CLANG)
	ifeq ($(CONFIG),RELEASE)
		CCFLAGS= -c -std=c99 -march=i386 -Wshadow -m32 -Werror -Wall -O -ffreestanding -nostdinc -fno-strict-aliasing -fno-builtin -fno-stack-protector -fomit-frame-pointer -fno-common -Iinclude -Xclang -triple=i386-pc-unknown
	else
		CCFLAGS= -c -std=c99 -march=i386 -Wshadow -m32 -Werror -Wall -O -ffreestanding -nostdinc -fno-strict-aliasing -fno-builtin -fno-stack-protector -fno-omit-frame-pointer -fno-common -Iinclude -Xclang -triple=i386-pc-unknown
	endif
else
	ifeq ($(CONFIG),RELEASE)
		CCFLAGS= -c -std=c99 -march=i386 -Wshadow -m32 -Werror -Wall -s -O -ffreestanding -nostdinc -fno-pic -fno-strict-aliasing -fno-builtin -fno-stack-protector -fomit-frame-pointer -fno-common -Iinclude
	else
		CCFLAGS= -c -std=c99 -march=i386 -Wshadow -m32 -Werror -Wall -s -O -ffreestanding -nostdinc -fno-pic -fno-strict-aliasing -fno-builtin -fno-stack-protector -fno-omit-frame-pointer -fno-common -Iinclude
	endif
endif
LDFLAGS= -nostdlib --warn-common

# targets to build one asm or c-file to an object file
vpath %.o $(OBJDIR)
%.o: %.c
	$(CC) $< $(CCFLAGS) -I $(KERNELDIR) -o $(OBJDIR)/$@
%.o: %.asm
	$(NASM) $< $(NASMFLAGS) -I$(KERNELDIR)/ -o $(OBJDIR)/$@

# targets to build PrettyOS
.PHONY: clean all shell other_userprogs userlibs
.SILENT: shell other_userprogs userlibs
.NOTPARALLEL: $(KERNELDIR)/initrd.dat

all: FloppyImage.img

$(STAGE1DIR)/boot.bin: $(STAGE1DIR)/boot.asm $(STAGE1DIR)/*.inc
	$(NASM) -f bin $(STAGE1DIR)/boot.asm -I$(STAGE1DIR)/ -o $(STAGE1DIR)/boot.bin
$(STAGE2DIR)/BOOT2.BIN: $(STAGE2DIR)/boot2.asm $(STAGE2DIR)/*.inc
	$(NASM) -f bin $(STAGE2DIR)/boot2.asm -I$(STAGE2DIR)/ -o $(STAGE2DIR)/BOOT2.BIN

$(USERDIR)/vm86/VIDSWTCH.COM: $(USERDIR)/vm86/vidswtch.asm
	$(NASM) $(USERDIR)/vm86/vidswtch.asm -o $(USERDIR)/vm86/VIDSWTCH.COM
$(USERDIR)/vm86/APM.COM: $(USERDIR)/vm86/apm.asm
	$(NASM) $(USERDIR)/vm86/apm.asm -o $(USERDIR)/vm86/APM.COM

$(KERNELDIR)/KERNEL.BIN: $(KERNELDIR)/initrd.dat $(USERDIR)/vm86/VIDSWTCH.COM $(USERDIR)/vm86/APM.COM $(KERNEL_OBJECTS)
#	because changes in the Shell should change data.o we build data.o everytimes
	$(NASM) $(KERNELDIR)/data.asm $(NASMFLAGS) -I$(KERNELDIR)/ -o $(OBJDIR)/$(KERNELDIR)/data.o
	$(LD) $(LDFLAGS) $(addprefix $(OBJDIR)/,$(KERNEL_OBJECTS)) -T $(KERNELDIR)/kernel.ld -Map documentation/kernel.map -o $(KERNELDIR)/KERNEL.BIN
#	$(STRIP) $(KERNELDIR)/KERNEL.BIN

shell: userlibs
	$(MAKE) --no-print-directory -C $(SHELLDIR)

other_userprogs: userlibs
	$(MAKE) --no-print-directory -C $(USERPROGDIR)

userlibs:
	$(MAKE) --no-print-directory -C $(STDLIBC)
	$(MAKE) --no-print-directory -C $(USERTOOLS)

$(KERNELDIR)/initrd.dat: shell
	$(MKINITRD) $(USERRDDIR)/info.txt info $(SHELLDIR)/SHELL.ELF shell
	$(MV) initrd.dat $(KERNELDIR)/initrd.dat

FloppyImage.img: other_userprogs $(STAGE1DIR)/boot.bin $(STAGE2DIR)/BOOT2.BIN $(KERNELDIR)/KERNEL.BIN
	$(FLOPPYIMAGE) PRETTYOS FloppyImage.img $(STAGE1DIR)/boot.bin $(STAGE2DIR)/BOOT2.BIN $(KERNELDIR)/KERNEL.BIN $(wildcard $(USERPROGDIR)/*.ELF)

clean:
# OS-dependant code because of different interpretation of "/" in Windows and UNIX-Based OS (Linux and Mac OS X)
ifeq ($(OS),WINDOWS)
	$(RM) $(STAGE1DIR)\boot.bin
	$(RM) $(STAGE2DIR)\BOOT2.BIN
	$(RM) $(KERNELDIR)\KERNEL.BIN
	$(RM) $(KERNELDIR)\initrd.dat
	$(RM) $(OBJDIR)\$(KERNELDIR)\*.o
	$(RM) $(USERDIR)\vm86\VIDSWTCH.COM
	$(RM) $(USERDIR)\vm86\APM.COM
	$(RM) documentation\*.map
else
	$(RM) $(STAGE1DIR)/boot.bin
	$(RM) $(STAGE2DIR)/BOOT2.BIN
	$(RM) $(KERNELDIR)/KERNEL.BIN
	$(RM) $(KERNELDIR)/initrd.dat
	find $(OBJDIR)/$(KERNELDIR) -name '*.o' -delete
	$(RM) $(USERDIR)/vm86/VIDSWTCH.COM
	$(RM) $(USERDIR)/vm86/APM.COM
	$(RM) documentation/*.map
endif
	$(MAKE) --no-print-directory -C $(SHELLDIR) clean
	$(MAKE) --no-print-directory -C $(USERPROGDIR) clean
	$(MAKE) --no-print-directory -C $(STDLIBC) clean
	$(MAKE) --no-print-directory -C $(USERTOOLS) clean
