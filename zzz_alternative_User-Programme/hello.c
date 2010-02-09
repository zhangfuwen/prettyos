#include "userlib.h"

int main()
{
    settextcolor(11,0);
    puts("--------------------------------------------------------------------\n");
    puts("Hello, Pretty Operating System World!\n");
    puts("This piece of software was loaded by PrettyOS from your floppy disk.\n");
    puts("It runs as a separate process in the user space. Have fun!\n");
    puts("--------------------------------------------------------------------\n");
    settextcolor(15,0);
    return 0;
}
