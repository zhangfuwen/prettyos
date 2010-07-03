#include "userlib.h"

#define MAX 2000

unsigned short timeout=MAX, row, col[10]; 
bool point[80][42], result, chance;
unsigned short fighterPosition;


void Sleep(unsigned int Milliseconds) 
{
	unsigned int Stoptime = getCurrentSeconds()*1000 + Milliseconds;
	while(getCurrentSeconds()*1000 < Stoptime) {}
}

void clearLine(unsigned int line)
{
    gotoxy(0,line);
    for (unsigned short i=0; i<79; i++)
    {        
        putch(' ');
    }
}

void setWeapon(unsigned short x, unsigned short y)
{
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
    
    for (unsigned short i=0; i<1; i++)
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
    for (unsigned short i=0; i<1; i++)
    {        
        col[i] = fighterPosition + 1 + rand()%(79-fighterPosition);
        point[col[i]][42] = false;
        for (unsigned short j=42; j>0; j--)
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
    if (keyPressed('A'))
    {
        gotoxy(fighterPosition,43);
        putch(' ');
        fighterPosition--;
        if (fighterPosition>=80)
        {
            fighterPosition = 0;
        }
        gotoxy(fighterPosition,43);
        putch(1);
    }
    if(keyPressed('D'))
    {
        gotoxy(fighterPosition,43);
        putch(' ');
        fighterPosition++;
        if (fighterPosition>79)
        {
            fighterPosition = 79;
        }
        gotoxy(fighterPosition,43);
        putch(1);
    }
    if(keyPressed('S') && (fighterPosition<79))
    {
        timeout--;
        if ((timeout>=0) && (timeout<MAX))
        {
            deleteWeapons();
            gotoxy(0,0);
            printf("delete triers: %u /%u", MAX-timeout, MAX);
        }
    }
}

bool checkCrash()
{
    if (point[fighterPosition][42] == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool checkWin()
{
    if (fighterPosition == 79)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int main()
{
    setScrollField(0, 43); // The train should not be destroyed by the output, so we shrink the scrolling area...
    srand(getCurrentSeconds()); // initialize random generator
    
    clearScreen(0);
    generateWeapons();
    generateFighter(); 
    setWeapon(10,42);
    setWeapon(15,42);
    setWeapon(20,42);
    setWeapon(30,42);
    setWeapon(40,42);
    setWeapon(70,42);

    while(true)
    {
        gotoxy(25,0);
        printf("\"ARROW ATTACK\" rev. 0.1 E. Henkes A/D=left/right, S=del"); 
        generateWeapons();
        for (unsigned short line=1; line<42; line++)
        {
            for (unsigned short column=0; column<79; column++)
            {
                chance = rand()%6;
                if ( point[column][line] == true && chance == false)
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
            for (unsigned int i=0; i<200; i++){getCurrentSeconds();}
        }
        moveFighter();
        if (checkCrash())
        {
            result = false;
            break; 
        }
        if (checkWin())
        {
            result = true;
            break;
        }
    }
    gotoxy(39,42);
    if (result == false)
    {
        printf("GAME OVER - PLAYER LOST");
    }
    else
    {
        printf("PLAYER WINS");
    }   
    
    Sleep(5000);
    return(0);
}
