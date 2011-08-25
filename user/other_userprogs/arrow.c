#include "userlib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define MAX 2000

uint16_t timeout=MAX;
bool point[79][42];
uint8_t fighterPosition = 0;

static void clearLine(uint8_t line)
{
    iSetCursor(0,line);
    for (uint8_t i=0; i<79; i++)
    {
        putchar(' ');
    }
}

static void setWeapon(uint8_t x, uint8_t y)
{
    point[x][y-1] = true;
    iSetCursor(x,y-1);
    putchar('|');
    iSetCursor(x,y);
    putchar('v');
}

static void generateWeapons()
{
    clearLine(1);
    clearLine(2);

    if ((rand()%10 == 0))
    {
        setWeapon(rand()%79,2);
    }
}

static void deleteWeapons()
{
    int i = fighterPosition + 1 + rand()%(79-fighterPosition);
    for (int8_t j=41; j>=0; j--)
    {
        if(point[i][j])
        {
            point[i][j] = false;
            iSetCursor(i, j-1);
            putchar(' ');
            iSetCursor(i, j);
            putchar(' ');
            iSetCursor(i, j+1);
            putchar(' ');
            break;
        }
    }
}

static void moveFighter()
{
    if (keyPressed(KEY_A) && fighterPosition > 0)
    {
        fighterPosition--;
        iSetCursor(fighterPosition,43);
        putchar(1); putchar(' ');
    }
    if (keyPressed(KEY_D) && fighterPosition<79)
    {
        iSetCursor(fighterPosition,43);
        putchar(' '); putchar(1);
        fighterPosition++;
    }
    if (keyPressed(KEY_S) && fighterPosition<78 && timeout>0)
    {
        timeout--;
        deleteWeapons();
        iSetCursor(0,0);
        printf("trials: %u/%u", MAX-timeout, MAX);
    }
    if (keyPressed(KEY_ESC))
    {
        exit();
    }
}

bool checkCrash()
{
    return(point[fighterPosition][41]);
}

bool checkWin()
{
    return(fighterPosition >= 79);
}

int main()
{
    srand(getCurrentSeconds()); // initialize random generator
    textColor(0x0F);

    memset(point, 0, sizeof(**point)*79*42);

    iSetCursor(fighterPosition,43);
    putchar(1);

    for (uint8_t i = 0; i < 6; i++)
    {
        setWeapon(5 + rand()%75, 42);
    }

    while (true)
    {
        iSetCursor(25,0);
        printf("\"ARROW ATTACK\" 0.15 A=left, D=right, S=delete arrows");
        generateWeapons();
        for (uint8_t line=0; line<41; line++)
        {
            for (uint8_t column=0; column<79; column++)
            {
                if (point[column][line] == true && rand()%6 == 0)
                {
                    iSetCursor(column,line);
                    putchar(' ');
                    iSetCursor(column,line+1);
                    putchar('|');
                    iSetCursor(column,line+2);
                    putchar('v');
                    point[column][line] = false;
                    point[column][line+1] = true;
                }
            }
            for (uint16_t i=0; i<250; i++){getCurrentSeconds();}
        }
        moveFighter();
        if (checkCrash())
        {
            iSetCursor(9,44);
            printf("GAME OVER - PLAYER LOST!");
            break;
        }
        if (checkWin())
        {
            iSetCursor(14,44);
            printf("PLAYER WINS!");
            break;
        }
    }

    printf("   Press ESC to quit.");
    while (!keyPressed(KEY_ESC));
    return(0);
}
