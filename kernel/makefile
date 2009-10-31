SOURCES = kernel.asm $(filter-out kernel.asm ,$(wildcard *.asm *.c))
OBJECTS = $(addsuffix .o,$(basename $(SOURCES)))

ASFLAGSOBJ= -O32 -f elf
NASM = ..\_nasm\nasmw

CFLAGS= -std=c99 -march=i386 -mtune=i386 -Werror -Wall -O -ffreestanding -fleading-underscore -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Iinclude
CC= i586-elf-gcc

LDFLAGS= -T kernel.ld -Map kernel.map -nostdinc
LD= i586-elf-ld

all: ckernel.sys
	mingw32-make -s image 	

%.o: %.asm
	$(NASM) $(ASFLAGSOBJ) $< -o $@

ckernel.sys: $(OBJECTS)
	$(LD) $(LDFLAGS) $+ -o $@ 

data.asm: initrd.dat	
	
image:
	del *.o
	copy CKERNEL.SYS A:\CKERNEL.SYS
	

#    $<		Erste Abhängigkeit
#    $+		Liste aller Abhängigkeiten	
#    $@		Name des Targets