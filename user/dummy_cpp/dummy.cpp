#include "userlib.hpp"

int main()
{
    setScrollField(0, 43); // The train should not be destroyed by the output, so we shrink the scrolling area...
    printLine("================================================================================", 0, 0x0B);
    printLine("                               C++ - Testprogramm!                              ", 2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);
    printLine("                                 ! Hello World !                                ", 7, 0x0C);
    for(;;)
    {
        showInfo(1);
    }
    return(0);
}
