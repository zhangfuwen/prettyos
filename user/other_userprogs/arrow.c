#include "userlib.h"

#define MAX 2000

uint16_t timeout=MAX, col[10]; 
bool point[80][43];
uint8_t fighterPosition = 0;

void clearLine(uint8_t line)
{
    gotoxy(0,line);
    for (uint8_t i=0; i<79; i++)
    {        
        putch(' ');
    }
}

void setWeapon(uint8_t x, uint8_t y)
{
    textColor(0x0F);
    point[x][y] = true;
    gotoxy(x,y-1);
    putch('|'); 
    gotoxy(x,y);
    putch('v'); 
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
            gotoxy(col[i],j);
            putch(' '); 
        }        
    }
}

void generateFighter()
{
    clearLine(43); 
    fighterPosition = 0;
    gotoxy(fighterPosition,43);
    putch(1);
}

void moveFighter()
{
    if (keyPressed('A') && fighterPosition > 0)
    {
        fighterPosition--;
        gotoxy(fighterPosition,43);
        putch(1); putch(' ');
    }
    if(keyPressed('D') && fighterPosition<79)
    {
        gotoxy(fighterPosition,43);
        putch(' '); putch(1);
        fighterPosition++;
    }
    if(keyPressed('S') && fighterPosition<79 && timeout>0)
    {
        timeout--;
        deleteWeapons();
        gotoxy(0,0);
        printf("trials: %u/%u", MAX-timeout, MAX);
    }
    if(keyPressed(VK_ESCAPE))
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
    
    for(uint8_t i = 0; i < 80; i++)
    {
        for(uint8_t j = 0; j < 43; j++)
        {
            point[i][j] = false;
        }
    }

    generateWeapons();
    generateFighter();

    for(uint8_t i = 0; i < 6; i++)
    {
        setWeapon(rand()%75+5, 42);
    }

    while(true)
    {
        gotoxy(25,0);
        printf("\"ARROW ATTACK\" 0.1   E. Henkes   A=left, R=right, S=del"); 
        generateWeapons();
        for (uint8_t line=1; line<42; line++)
        {
            for (uint8_t column=0; column<79; column++)
            {
                if (point[column][line] == true && rand()%6 == 0)
                {
                    gotoxy(column,line-2);
                    putch(' ');
                    gotoxy(column,line-1);
                    putch(' ');
                    gotoxy(column,line);
                    putch('|');
                    gotoxy(column,line+1);
                    putch('v');
                    point[column][line]   = false;
                    point[column][line+1] = true;
                }
            }
            for (uint8_t i=0; i<200; i++){getCurrentSeconds();}
        }
        moveFighter();
        if (checkCrash())
        {
            gotoxy(29,42);
            printf("GAME OVER - PLAYER LOST");
            break; 
        }
        if (checkWin())
        {
            gotoxy(34,42);
            printf("PLAYER WINS");
            break;
        }
    }
    
    getch();
    return(0);
}
