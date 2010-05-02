#include "userlib.h"

int main() {
	clearScreen(0);
    textColor(0x0B);
    puts("================================================================================\n");
    puts("                              PrettyOS Infocenter                               \n");
    puts("--------------------------------------------------------------------------------\n\n");
    textColor(0x0F);
	puts("PrettyOS is an easily understandable Operating System to provide a base for OS-Dev Newbies.\n\n");
	puts("Here should be inserted a bit more content ;-)\n");
	puts("\nAt the moment, the developement-team consists of: Ehenkes (he founded PrettyOS), Badestrand (At the moment not developing), Cuervo and MrX.\n");
	puts("You can find us under: http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html\n");
	getch();
	return(0);
}
