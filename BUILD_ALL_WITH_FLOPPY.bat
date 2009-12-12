	cls
		
	cd stage1_bootloader
	call BUILD.bat
	cd ..

	cd stage2_bootloader
	call BUILD.bat
	cd ..

	cd user\user_program_c
	call BUILD.bat
	cp -f program.elf ../init_rd_img/program.elf 
	cd ..
		
	cd init_rd_img
	call BUILD.bat
	cp -f initrd.dat ../../kernel/initrd.dat 
	
	cd ..\..\kernel
	call BUILD.bat
	cd ..
	
	pause