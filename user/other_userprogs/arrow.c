#include "userlib.h"
#include "stdio.h"
#include "stdlib.h"

#define MAX 2000

uint16_t timeout=MAX, col[10];
bool point[80][43];
uint8_t fighterPosition = 0;

void clearLine(uint8_t line)
{
    iSetCursor(0,line);
    for (uint8_t i=0; i<79; i++)
    {
        putchar(' ');
    }
}

void setWeapon(uint8_t x, uint8_t y)
{
    textColor(0x0F);
    point[x][y] = true;
    iSetCursor(x,y-1);
    putchar('|');
    iSetCursor(x,y);
    putchar('v');
}

void generateWeapons()
{
    clearLine(1);
    clearLine(2);

    for (uint8_t i=0; i<1; i++)
    {
        col[i] = rand()%79;
        if ((rand()%10 == 0))
        {
            setWeapon(col[i],2);
        }
    }
}

void deleteWeapons()
{
    for (uint8_t i=0; i<1; i++)
    {
        col[i] = fighterPosition + 1 + rand()%(79-fighterPosition);
        point[col[i]][42] = false;
        for (uint8_t j=42; j>0; j--)
        {
            iSetCursor(col[i],j);
            putchar(' ');
        }
    }
}

void generateFighter()
{
    clearLine(43);
    fighterPosition = 0;
    iSetCursor(fighterPosition,43);
    putchar(1);
}

void moveFighter()
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
    if (keyPressed(KEY_S) && fighterPosition<79 && timeout>0)
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
    return(point[fighterPosition][42]);
}

bool checkWin()
{
    return(fighterPosition >= 79);
}

int main()
{
    srand(getCurrentSeconds()); // initialize random generator
    clearScreen(0);
    textColor(0x0F);

    for (uint8_t i = 0; i < 80; i++)
    {
        for (uint8_t j = 0; j < 43; j++)
        {
            point[i][j] = false;
        }
    }

    generateWeapons();
    generateFighter();

    for (uint8_t i = 0; i < 6; i++)
    {
        setWeapon(5 + rand()%75, 42);
    }

    while (true)
    {
        iSetCursor(25,0);
        printf("\"ARROW ATTACK\" 0.14 A=left, D=right, S=delete arrows");
        generateWeapons();
        for (uint8_t line=1; line<42; line++)
        {
            for (uint8_t column=0; column<79; column++)
            {
                if (point[column][line] == true && rand()%6 == 0)
                {
                    iSetCursor(column,line-2);
                    putchar(' ');
                    iSetCursor(column,line-1);
                    putchar(' ');
                    iSetCursor(column,line);
                    putchar('|');
                    iSetCursor(column,line+1);
                    putchar('v');
                    point[column][line]   = false;
                    point[column][line+1] = true;
                }
            }
            for (uint8_t i=0; i<200; i++){getCurrentSeconds();}
        }
        moveFighter();
        if (checkCrash())
        {
            iSetCursor(9,42);
            printf("GAME OVER - PLAYER LOST!  ");
            break;
        }
        if (checkWin())
        {
            iSetCursor(14,42);
            printf("PLAYER WINS!  ");
            break;
        }
    }

    printf("Do you want to quit (Q)");
    while (!keyPressed(KEY_Q));
    return(0);
}
