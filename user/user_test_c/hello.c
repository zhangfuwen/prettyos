#include "userlib.h"

int main()
{
    setScrollField(0, 43); // The train should not be destroyed by the output, so we shrink the scrolling area...
    printf("\n================================================================================");
    printf("\n                                C - Testprogramm!                               ");
    printf("\n--------------------------------------------------------------------------------");
    printf("\n                                 ! Hello World !                                ");
    printf("\n");
    
    srand(getCurrentSeconds());
    for(int i=0; i<100; i++)
    {
        printf("%u\t",rand());
    }
    
    for(;;)
    {
        showInfo(1);
    }
    return(0);
}
