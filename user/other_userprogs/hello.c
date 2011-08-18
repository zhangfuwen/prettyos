#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"

int main(size_t argc, char* argv[])
{
    console_setProperties(CONSOLE_AUTOREFRESH|CONSOLE_AUTOSCROLL|CONSOLE_FULLSCREEN);
    //setScrollField(0, 43); // The train should not be destroyed by the output, so we shrink the scrolling area...
    printLine("================================================================================", 0, 0x0B);
    printLine("                                C - Testprogramm!",                                2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);
    printLine("                                 ! Hello World !",                                 7, 0x0C);

    iSetCursor(0, 10);

    srand(getCurrentSeconds());
    for (uint16_t i = 0; i < 100; i++)
    {
        printf("%u\t", rand());
    }

    printf("\n\n\nCommand line arguments:\n");
    for (size_t i = 0; i < argc; i++)
    {
        printf("\n\t%s", argv[i]);
    }

    for (;;)
    {
        showInfo(1);
    }
    return(0);
}
