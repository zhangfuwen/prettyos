cls
tools\mingw32-make OS=WINDOWS

@echo off
:Loop
IF [%1]==[] GOTO Continue
	IF [%1]==[bochs] (
		cmd /c tools\bochs.bxrc
	)
	IF [%1]==[qemu] (
		echo ""
	)
	IF [%1]==[disc] (
		tools\dd if=stage1_bootloader/boot.bin of=\\.\A: bs=512 count=1 --progress
		copy stage2_bootloader\boot2.bin A:\boot2.bin
		copy kernel\kernel.bin A:\kernel.bin
	)
SHIFT
GOTO Loop
:Continue