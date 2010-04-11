#include "userlib.h"

int main()
{
    setScrollField(0, 43); // We do not want to see scrolling output...
    puts("Hello World");
    for(;;)
    {
        showInfo(1);
    }
    return 0;
}
