#include "userlib.h"

int main() {
	clearScreen(0);
	puts("'Keysound' by Cuervo\n\n\n");
	puts("This Application generates a beep sound whenever you press a key (A-Z)\n\n");
	puts("You may now switch to an other console.\n");
	uint8_t c;
	for (;;) {
		for (c = 65; c <= 90; c++)
		{
			if (keyPressed((char)c)) {
				beep(1000,30);
			}
		}
	}
}