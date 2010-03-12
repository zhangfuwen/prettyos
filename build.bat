cls
del FloppyImage.bin
tools\mingw32-make OS=WINDOWS

@echo off
:Loop
IF [%1]==[] GOTO Continue
	IF [%1]==[bochs] (
		cmd /c tools\bochs.bxrc
	)
	IF [%1]==[qemufloppy] (
		qemu-system-x86_64.exe -usbdevice mouse -fda \\.\a: -boot a -localtime
	)
	IF [%1]==[qemuimage] (
		qemu-system-x86_64.exe -usbdevice mouse -fda FloppyImage.bin -boot a -localtime
	)
	IF [%1]==[disc] (
		tools\dd if=stage1_bootloader/boot.bin of=\\.\A: bs=512 count=1 --progress
		copy stage2_bootloader\boot2.bin A:\boot2.bin
		copy kernel\kernel.bin A:\kernel.bin
		copy user\user_test_c\*.elf A:
	)
SHIFT
GOTO Loop
:Continue