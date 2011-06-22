#include "userlib.h"
#include "stdio.h"

int main()
{
    puts("'Keysound' by Cuervo\n\n\n");
    puts("This Application generates a beep sound whenever you press a key (A-Z)\n\n");
    puts("You may now switch to an other console. Press ESC to quit.");

    while (!keyPressed(KEY_ESC))
    {
        for (KEY_t key = KEY_A; key <= KEY_Z; key++)
        {
            if (keyPressed(key))
                beep(300 + key*20, 30);
        }
    }

    return(0);
}