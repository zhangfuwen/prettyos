#include "userlib.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"


enum Feldstatus {Leer, X, O};

uint8_t tictactoe[9];
bool ende = false;

void SetField(uint8_t x, uint8_t y, uint8_t Player)
{
    iSetCursor(x*4+2,y*2+15);
    if(Player == X)
	{
        putchar('X');
    }
    else if(Player == O)
	{
        putchar('O');
    }
}

void gewinnen()
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
        printLine("Player X wins!", 26, 0x05);
        ende = true;
    }
    else if((tictactoe[0] == tictactoe[1] && tictactoe[0] == tictactoe[2] && tictactoe[0] == O) ||
        (tictactoe[3] == tictactoe[4] && tictactoe[3] == tictactoe[5] && tictactoe[3] == O) ||
        (tictactoe[6] == tictactoe[7] && tictactoe[6] == tictactoe[8] && tictactoe[6] == O) ||
        (tictactoe[0] == tictactoe[3] && tictactoe[0] == tictactoe[6] && tictactoe[0] == O) ||
        (tictactoe[1] == tictactoe[4] && tictactoe[1] == tictactoe[7] && tictactoe[1] == O) ||
        (tictactoe[2] == tictactoe[5] && tictactoe[2] == tictactoe[8] && tictactoe[2] == O) ||
        (tictactoe[0] == tictactoe[4] && tictactoe[0] == tictactoe[8] && tictactoe[0] == O) ||
        (tictactoe[2] == tictactoe[4] && tictactoe[2] == tictactoe[6] && tictactoe[2] == O))
    {
        printLine("Player O wins!", 26, 0x05);
        ende = true;
    }
    else if(tictactoe[0] != Leer && tictactoe[1] != Leer && tictactoe[2] != Leer &&
        tictactoe[3] != Leer && tictactoe[4] != Leer && tictactoe[5] != Leer &&
        tictactoe[6] != Leer && tictactoe[7] != Leer && tictactoe[8] != Leer)
    {
        printLine("Remis!", 26, 0x05);
        ende = true;
    }
}

void Zug(uint16_t Player)
{
    char str[80];
	uint32_t input = 9;

    for(;;)
    {
		iSetCursor(0, 24);
		gets(str);
		if(isdigit(*str))
		{
			input = atoi(str);
		}
		else
		{
			input = 9; // String is empty -> Input not useful
		}

		printLine("                                                                                ", 24, 0x0F); // Clear Inputline
		printLine("                                                                                ", 26, 0x0F); // Clear Messageline

        if(input >= 9)
        {
            printLine("Your Input was not useful.", 26, 0x0C);
        }
        else if(tictactoe[input] != Leer)
        {
            printLine("Field already used.", 26, 0x0C);
        }
        else
        {
            break;
        }
    }

    tictactoe[input] = Player;
    SetField(input%3, input/3, Player);
    gewinnen();
}

int32_t main()
{
    memset(tictactoe, 0, sizeof(tictactoe));
    clearScreen(0);

    printLine("--------------------------------------------------------------------------------", 0, 0x0B);
    printLine("                           Mr.X TicTacToe 3x3  v0.6.3                           ", 2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);

    iSetCursor(0,6);
    textColor(0x0F);
    puts("*************\n| 0 | 1 | 2 |\n*************\n| 3 | 4 | 5 |\n*************\n| 6 | 7 | 8 |\n*************\n\n");
    puts("*************\n|   |   |   |\n*************\n|   |   |   |\n*************\n|   |   |   |\n*************\n\n");

    printLine("Please type in a number betwen 0 and 8.", 22, 0x0B);

    Zug(X);
    for(uint8_t i = 0; i < 4 && !ende; ++i)
	{
        Zug(O);
        if(ende)
		{
            break;
        }
        Zug(X);
    }

	printLine("Press a key to continue...", 28, 0x0F);
	getchar();

    return 0;
}
