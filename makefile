STAGE1DIR= stage1_bootloader
STAGE2DIR= stage2_bootloader
KERNELDIR= kernel
USERRDDIR= user/init_rd_img
USERDIR= user/user_program_c

ifeq ($(OS),WINDOWS)
    NASM= nasmw
    CC= i586-elf-gcc
    LD= i586-elf-ld
    #NASM= tools/nasmw
    #CC= tools/i586-elf/bin/i586-elf-gcc
    #LD= tools/i586-elf/bin/i586-elf-ld
else
    NASM=nasm
    CC=gcc
    LD=ld
endif


all: boot1 boot2 ckernel

boot1: $(wildcard $(STAGE1DIR)/*.asm $(STAGE1DIR)/*.inc)
	$(NASM) -f bin $(STAGE1DIR)/boot.asm -I$(STAGE1DIR)/ -o $(STAGE1DIR)/boot.bin

boot2: $(wildcard $(STAGE2DIR)/*.asm $(STAGE2DIR)/*.inc)
	$(NASM) -f bin $(STAGE2DIR)/boot2.asm -I$(STAGE2DIR)/ -o $(STAGE2DIR)/boot2.bin

ckernel: $(wildcard $(KERNELDIR)/* $(KERNELDIR)/include/*) initrd
	rm *.o -f
	$(CC) $(KERNELDIR)/*.c -c -I$(KERNELDIR)/include -std=c99 -march=i386 -mtune=i386 -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Iinclude
	$(NASM) -O32 -f elf $(KERNELDIR)/data.asm -I$(KERNELDIR)/ -o data.o
	$(NASM) -O32 -f elf $(KERNELDIR)/flush.asm -I$(KERNELDIR)/ -o flush.o
	$(NASM) -O32 -f elf $(KERNELDIR)/interrupts.asm -I$(KERNELDIR)/ -o interrupts.o
	$(NASM) -O32 -f elf $(KERNELDIR)/kernel.asm -I$(KERNELDIR)/ -o kernel.o
	$(NASM) -O32 -f elf $(KERNELDIR)/process.asm -I$(KERNELDIR)/ -o process.o
	$(LD) *.o -T $(KERNELDIR)/kernel.ld -Map $(KERNELDIR)/kernel.map -nostdinc -o $(KERNELDIR)/kernel.bin
	rm *.o -f
	tools/CreateFloppyImage2 PrettyOS FloppyImage.bin $(STAGE1DIR)/boot.bin $(STAGE2DIR)/boot2.bin $(KERNELDIR)/kernel.bin

initrd: $(wildcard $(USERDIR)/*)
	rm *.o -f
	$(NASM) -O32 -f elf $(USERDIR)/start.asm -I$(USERDIR)/ -o start.o
	$(CC) $(USERDIR)/*.c -c -I$(USERDIR) -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin
	$(NASM) -O32 -f elf $(USERDIR)/start.asm -o start.o
	$(LD) *.o -T $(USERDIR)/user.ld -Map $(USERDIR)/kernel.map -nostdinc -o $(USERDIR)/program.elf
	rm *.o -f
	tools/make_initrd $(USERRDDIR)/test1.txt file1 $(USERRDDIR)/test2.txt file2 $(USERRDDIR)/test3.txt file3 $(USERDIR)/program.elf shell
	mv initrd.dat $(KERNELDIR)/initrd.dat