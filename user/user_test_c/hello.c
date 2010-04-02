#include "userlib.h"

enum Feldstatus {Leer, X, O};

//cf. util.c
void* memset(void* dest, uint8_t val, size_t count)
{
    uint8_t* temp = (uint8_t*)dest;
    for (; count != 0; count--) *temp++ = val;
    return dest;
}

void SetField(uint16_t x, uint16_t y, uint8_t Player)
{
    gotoxy(x*4+2,y*2+15);
    if (Player == X){putch('X');}
    if (Player == O){putch('O');}
    gotoxy(0,24);
    puts("     \r");
}

void gewinnen (uint16_t* tictactoe, bool ende)
{
    if ((tictactoe[0] == tictactoe[1] && tictactoe[0] == tictactoe[2] && tictactoe[0] == X) ||
        (tictactoe[3] == tictactoe[4] && tictactoe[3] == tictactoe[5] && tictactoe[3] == X) ||
        (tictactoe[6] == tictactoe[7] && tictactoe[6] == tictactoe[8] && tictactoe[6] == X) ||
        (tictactoe[0] == tictactoe[3] && tictactoe[0] == tictactoe[6] && tictactoe[0] == X) ||
        (tictactoe[1] == tictactoe[4] && tictactoe[1] == tictactoe[7] && tictactoe[1] == X) ||
        (tictactoe[2] == tictactoe[5] && tictactoe[2] == tictactoe[8] && tictactoe[2] == X) ||
        (tictactoe[0] == tictactoe[4] && tictactoe[0] == tictactoe[8] && tictactoe[0] == X) ||
        (tictactoe[2] == tictactoe[4] && tictactoe[2] == tictactoe[6] && tictactoe[2] == X))
        {
            settextcolor(5,0);
            gotoxy(0,26);
            puts("Player X wins\n\n");
            settextcolor(15,0);
            ende = true;
        }
    else if ((tictactoe[0] == tictactoe[1] && tictactoe[0] == tictactoe[2] && tictactoe[0] == O) ||
        (tictactoe[3] == tictactoe[4] && tictactoe[3] == tictactoe[5] && tictactoe[3] == O) ||
        (tictactoe[6] == tictactoe[7] && tictactoe[6] == tictactoe[8] && tictactoe[6] == O) ||
        (tictactoe[0] == tictactoe[3] && tictactoe[0] == tictactoe[6] && tictactoe[0] == O) ||
        (tictactoe[1] == tictactoe[4] && tictactoe[1] == tictactoe[7] && tictactoe[1] == O) ||
        (tictactoe[2] == tictactoe[5] && tictactoe[2] == tictactoe[8] && tictactoe[2] == O) ||
        (tictactoe[0] == tictactoe[4] && tictactoe[0] == tictactoe[8] && tictactoe[0] == O) ||
        (tictactoe[2] == tictactoe[4] && tictactoe[2] == tictactoe[6] && tictactoe[2] == O))
        {
            settextcolor(5,0);
            gotoxy(0,26);
            puts("Player O wins!\n\n");
            settextcolor(15,0);
            ende = true;
        }
    else
    {
        if (tictactoe[0] != Leer && tictactoe[1] != Leer && tictactoe[2] != Leer &&
            tictactoe[3] != Leer && tictactoe[4] != Leer && tictactoe[5] != Leer &&
            tictactoe[6] != Leer && tictactoe[7] != Leer && tictactoe[8] != Leer)
            {
                settextcolor(5,0);
                gotoxy(0,26);
                puts("Remis!\n\n");
                settextcolor(15,0);
                ende = true;
            }
    }
}

void Zug(uint16_t Player, char* str, uint16_t* tictactoe, bool ende)
{
	memset((void*)str, 0, 80);

    for (; ; gets(str))
    {
        if (!isdigit(*str) || *str == '9')
        {
        }
        else if (tictactoe[atoi(str)] != Leer)
        {
            settextcolor(12,0);
            gotoxy(0,26);
            puts("Field already used. Please enter a valid number.\n\n");
            gotoxy(0,24);
            settextcolor(15,0);
        }
        else
        {
            break;
        }
		memset((void*)str, 0, 80);
    }
    gotoxy(0,26);
    puts("                                                         ");
    gotoxy(0,24);
    tictactoe[atoi(str)] = Player;
    SetField(atoi(str)%3, atoi(str)/3, Player);
    gewinnen(tictactoe, ende);
}

int32_t main()
{
    uint16_t tictactoe[9];
    bool ende = false;
    char str[80];

    memset((void*)tictactoe, 0, 18); // tictactoe has two bytes!
    clearScreen(0);
    settextcolor(11,0);
    puts("--------------------------------------------------------------------------------\n");
    puts("                            Mr.X TicTacToe 3x3  v0.55                           \n");
    puts("--------------------------------------------------------------------------------\n\n");
    gotoxy(0,6);
    settextcolor(15,0);
    puts("*************\n| 0 | 1 | 2 |\n*************\n| 3 | 4 | 5 |\n*************\n| 6 | 7 | 8 |\n*************\n\n");
    puts("*************\n|   |   |   |\n*************\n|   |   |   |\n*************\n|   |   |   |\n*************\n\n");
    settextcolor(11,0);
    gotoxy(0,22);
    puts("Please type in a number betwen 0 and 8.\n\n");
    settextcolor(15,0);

    Zug(X,str,tictactoe,ende);
    for (uint8_t i=0; i<4 && !ende; ++i)
    {
        Zug(O,str,tictactoe,ende);
        if (ende){ break; }
        Zug(X,str,tictactoe,ende);
    }
    settextcolor(15,0);
    gotoxy(0,28);
    return 0;
}
