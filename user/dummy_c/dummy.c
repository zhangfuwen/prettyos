#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"

int main()
{
    setScrollField(0, 43); // The train should not be destroyed by the output, so we shrink the scrolling area...
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

    for (;;)
    {
        showInfo(1);
    }
    return(0);
}
