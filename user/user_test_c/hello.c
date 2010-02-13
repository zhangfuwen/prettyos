#include "userlib.h"

enum Feldstatus {Leer, X, O};

int tictactoe[9];
char ende = false;

void helper()
{
	puts("*************\n| 0 | 1 | 2 |\n*************\n| 3 | 4 | 5 |\n*************\n| 6 | 7 | 8 |\n*************\n\n");
}

void printField() {
	puts("*************\n| ");
	for(int i = 0; i < 3; i++) {
		if(tictactoe[i] == 0) {
			putch(' ');
		}
		if(tictactoe[i] == 1) {
			putch('X');
		}
		if(tictactoe[i] == 2) {
			putch('O');
		}
		puts(" | ");
	}
	puts("\n*************\n| ");
	for(int i = 3; i < 6; i++) {
		if(tictactoe[i] == 0) {
			putch(' ');
		}
		if(tictactoe[i] == 1) {
			putch('X');
		}
		if(tictactoe[i] == 2) {
			putch('O');
		}
		puts(" | ");
	}
	puts("\n*************\n| ");
	for(int i = 6; i < 9; i++) {
		if(tictactoe[i] == 0) {
			putch(' ');
		}
		if(tictactoe[i] == 1) {
			putch('X');
		}
		if(tictactoe[i] == 2) {
			putch('O');
		}
		puts(" | ");
	}
	puts("\n*************\n\n");
}

void gewinnen () {
	if((tictactoe[0] == tictactoe[1] && tictactoe[0] == tictactoe[2] && tictactoe[0] == 1) ||
	   (tictactoe[3] == tictactoe[4] && tictactoe[3] == tictactoe[5] && tictactoe[3] == 1) ||
	   (tictactoe[6] == tictactoe[7] && tictactoe[6] == tictactoe[8] && tictactoe[6] == 1) ||
	   (tictactoe[0] == tictactoe[3] && tictactoe[0] == tictactoe[6] && tictactoe[0] == 1) ||
	   (tictactoe[1] == tictactoe[4] && tictactoe[1] == tictactoe[7] && tictactoe[1] == 1) ||
	   (tictactoe[2] == tictactoe[5] && tictactoe[2] == tictactoe[8] && tictactoe[2] == 1) ||
	   (tictactoe[0] == tictactoe[4] && tictactoe[0] == tictactoe[8] && tictactoe[0] == 1) ||
	   (tictactoe[2] == tictactoe[4] && tictactoe[2] == tictactoe[6] && tictactoe[2] == 1)) {
			puts("Spieler 1 hat gewonnen!\n\n");
			ende = true;
	}
	if((tictactoe[0] == tictactoe[1] && tictactoe[0] == tictactoe[2] && tictactoe[0] == 2) ||
	   (tictactoe[3] == tictactoe[4] && tictactoe[3] == tictactoe[5] && tictactoe[3] == 2) ||
	   (tictactoe[6] == tictactoe[7] && tictactoe[6] == tictactoe[8] && tictactoe[6] == 2) ||
	   (tictactoe[0] == tictactoe[3] && tictactoe[0] == tictactoe[6] && tictactoe[0] == 2) ||
	   (tictactoe[1] == tictactoe[4] && tictactoe[1] == tictactoe[7] && tictactoe[1] == 2) ||
	   (tictactoe[2] == tictactoe[5] && tictactoe[2] == tictactoe[8] && tictactoe[2] == 2) ||
	   (tictactoe[0] == tictactoe[4] && tictactoe[0] == tictactoe[8] && tictactoe[0] == 2) ||
	   (tictactoe[2] == tictactoe[4] && tictactoe[2] == tictactoe[6] && tictactoe[2] == 2)) {
			puts("Spieler 2 hat gewonnen!\n\n");
			ende = true;
	}
	else {
		if(tictactoe[0] != 0 && tictactoe[1] != 0 && tictactoe[2] != 0 &&
		   tictactoe[3] != 0 && tictactoe[4] != 0 && tictactoe[5] != 0 &&
		   tictactoe[6] != 0 && tictactoe[7] != 0 && tictactoe[8] != 0) {
			   puts("Unentschieden!\n\n");
			ende = true;
		}
	}
}

int ConvertToInt(char c) {
	return((int)(c) - '1' + 1);
}

int main()
{
    settextcolor(11,0);
    puts("--------------------------------------------------------------------\n");
    puts("                      Mr.X TicTacToe 3x3  v0.1a                     \n");
    puts("--------------------------------------------------------------------\n\n");

    beep(1000,300);

	int z=0;
	char x = 0;
	char t[1];
	helper();

	do
	{
		x = *gets(t);
	}
	while (!isdigit(x));

	tictactoe[ConvertToInt(x)] = X;
	printField();

	for(int i = 0; ((i<4) && (!ende)); i++)
	{
	    settextcolor(11,0);
		helper();
		while(z == 0)
		{
			do
			{
				x = *gets(t);
			}
			while (!isdigit(x));

			if((tictactoe[ConvertToInt(x)] == X) || (tictactoe[ConvertToInt(x)] == O))
			{
				puts("unmoeglich\n");
			}
			else
			{
				tictactoe[ConvertToInt(x)] = O;
				z = 1;
			}
		}
		printField();
		z = 0;
		gewinnen();
		if(ende)
		{
			break;
		}
		helper();
		while(z == 0)
		{
			do
			{
				x = *gets(t);
			}
			while(!isdigit(x));
			if((tictactoe[ConvertToInt(x)] == 1) || (tictactoe[ConvertToInt(x)] == 2))
			{
				puts("unmoeglich\n");
			}
			else
			{
				tictactoe[ConvertToInt(x)] = X;
				z = 1;
			}
		}
		printField();
		z = 0;
		gewinnen();
	}
    settextcolor(15,0);
	puts("GAME OVER\n\n");
	return 0;
}
