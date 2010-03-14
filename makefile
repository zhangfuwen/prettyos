STAGE1DIR= stage1_bootloader
STAGE2DIR= stage2_bootloader
KERNELDIR= kernel
USERRDDIR= user/init_rd_img
USERDIR= user/user_program_c
USERTEST= user/user_test_c
USERTOOLS= user/user_tools

ifeq ($(OS),WINDOWS)
    RM=del
    MV=cmd /c move/Y
    NASM= nasm
    CC= i586-elf-gcc
    LD= i586-elf-ld
else
    RM=rm -f
    MV=mv
    NASM=nasm
    CC=gcc
    LD=ld
endif


all: boot1 boot2 ckernel

boot1: $(wildcard $(STAGE1DIR)/*.asm $(STAGE1DIR)/*.inc)
	$(NASM) -f bin $(STAGE1DIR)/boot.asm -I$(STAGE1DIR)/ -o $(STAGE1DIR)/boot.bin

boot2: $(wildcard $(STAGE2DIR)/*.asm $(STAGE2DIR)/*.inc)
	$(NASM) -f bin $(STAGE2DIR)/boot2.asm -I$(STAGE2DIR)/ -o $(STAGE2DIR)/BOOT2.BIN

ckernel: $(wildcard $(KERNELDIR)/* $(KERNELDIR)/include/*) initrd
	$(RM) *.o
	$(CC) $(KERNELDIR)/*.c -c -I$(KERNELDIR)/include -m32 -std=c99 -Wshadow -march=i386 -mtune=i386 -m32 -fno-pic -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Iinclude
	$(CC) $(KERNELDIR)/cdi/*.c -c -I$(KERNELDIR)/include -m32 -std=c99 -Wshadow -march=i386 -mtune=i386 -m32 -fno-pic -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Iinclude
	$(NASM) -O32 -f elf $(KERNELDIR)/data.asm -I$(KERNELDIR)/ -o data.o
	$(NASM) -O32 -f elf $(KERNELDIR)/flush.asm -I$(KERNELDIR)/ -o flush.o
	$(NASM) -O32 -f elf $(KERNELDIR)/interrupts.asm -I$(KERNELDIR)/ -o interrupts.o
	$(NASM) -O32 -f elf $(KERNELDIR)/kernel.asm -I$(KERNELDIR)/ -o kernel.o
	$(NASM) -O32 -f elf $(KERNELDIR)/process.asm -I$(KERNELDIR)/ -o process.o
	$(LD) *.o -T $(KERNELDIR)/kernel.ld -Map $(KERNELDIR)/kernel.map -nostdinc -o $(KERNELDIR)/KERNEL.BIN
	$(RM) *.o 
	tools/CreateFloppyImage2 PrettyOS FloppyImage.bin $(STAGE1DIR)/boot.bin $(STAGE2DIR)/BOOT2.BIN $(KERNELDIR)/KERNEL.BIN $(USERTEST)/HELLO.ELF

initrd: $(wildcard $(USERDIR)/*)
	$(RM) *.o 
	$(NASM) -O32 -f elf $(USERDIR)/start.asm -I$(USERDIR)/ -o start.o
	$(CC) $(USERTOOLS)/userlib.c -c -I$(USERTOOLS) -m32 -std=c99 -Wshadow -march=i386 -mtune=i386 -m32 -fno-pic -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Iinclude
	$(CC) $(USERDIR)/*.c -c -I$(USERDIR) -I$(USERTOOLS) -m32 -std=c99 -Wshadow -march=i386 -mtune=i386 -m32 -fno-pic -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Iinclude
	$(LD) *.o -T $(USERTOOLS)/user.ld -Map $(USERDIR)/kernel.map -nostdinc -o $(USERDIR)/program.elf
	$(RM) *.o 
	tools/make_initrd $(USERRDDIR)/info.txt info $(USERDIR)/program.elf shell
	$(MV) initrd.dat $(KERNELDIR)/initrd.dat