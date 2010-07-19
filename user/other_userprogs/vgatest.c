#include "userlib.h"
#include "os.h"
#include "vbe.h"

void Sleep(unsigned int Milliseconds) {
	unsigned int Stoptime = getCurrentSeconds()*1000 + Milliseconds;
	while(getCurrentSeconds()*1000 < Stoptime) {}
}

int main() {
/*
	// --------------------- VM86 ----------- TEST -----------------------------
	memcpy ((void*)0x100, &vm86_com_start, (uintptr_t)&vm86_com_end - (uintptr_t)&vm86_com_start);
    
#ifdef _VM_DIAGNOSIS_ 
    printf("\n\nvm86 binary code at 0x100: ");
	memshow((void*)0x100, (uintptr_t)&vm86_com_end - (uintptr_t)&vm86_com_start); // TEST
	waitForKeyStroke();
#endif

    memset((void*) 0xA0000, 0, 0xB8000 - 0xA0000);
    create_vm86_ctask(NULL, (void*)0x100, "vm86-video");
    waitForKeyStroke();

    initGraphics(320, 200, 8);

	line(20, 20, 20, 100, 0x0A);
	
    waitForKeyStroke();

    create_vm86_ctask(NULL, (void*)0x3F4, "vm86-text");
*/
    waitForKeyStroke();

    // --------------------- VM86 ------------ TEST ----------------------------
	return 0;
}