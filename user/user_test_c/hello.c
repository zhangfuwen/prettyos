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
		if(tictactoe[i] == Leer) {
			putch(' ');
		}
		if(tictactoe[i] == X) {
			putch('X');
		}
		if(tictactoe[i] == O) {
			putch('O');
		}
		puts(" | ");
	}
	puts("\n*************\n| ");
	for(int i = 3; i < 6; i++) {
		if(tictactoe[i] == Leer) {
			putch(' ');
		}
		if(tictactoe[i] == X) {
			putch('X');
		}
		if(tictactoe[i] == O) {
			putch('O');
		}
		puts(" | ");
	}
	puts("\n*************\n| ");
	for(int i = 6; i < 9; i++) {
		if(tictactoe[i] == Leer) {
			putch(' ');
		}
		if(tictactoe[i] == X) {
			putch('X');
		}
		if(tictactoe[i] == O) {
			putch('O');
		}
		puts(" | ");
	}
	puts("\n*************\n\n");
}

void gewinnen () {
	if((tictactoe[0] == tictactoe[1] && tictactoe[0] == tictactoe[2] && tictactoe[0] == X) ||
	   (tictactoe[3] == tictactoe[4] && tictactoe[3] == tictactoe[5] && tictactoe[3] == X) ||
	   (tictactoe[6] == tictactoe[7] && tictactoe[6] == tictactoe[8] && tictactoe[6] == X) ||
	   (tictactoe[0] == tictactoe[3] && tictactoe[0] == tictactoe[6] && tictactoe[0] == X) ||
	   (tictactoe[1] == tictactoe[4] && tictactoe[1] == tictactoe[7] && tictactoe[1] == X) ||
	   (tictactoe[2] == tictactoe[5] && tictactoe[2] == tictactoe[8] && tictactoe[2] == X) ||
	   (tictactoe[0] == tictactoe[4] && tictactoe[0] == tictactoe[8] && tictactoe[0] == X) ||
	   (tictactoe[2] == tictactoe[4] && tictactoe[2] == tictactoe[6] && tictactoe[2] == X)) {
			puts("Player X wins\n\n");
			ende = true;
	}
	if((tictactoe[0] == tictactoe[1] && tictactoe[0] == tictactoe[2] && tictactoe[0] == O) ||
	   (tictactoe[3] == tictactoe[4] && tictactoe[3] == tictactoe[5] && tictactoe[3] == O) ||
	   (tictactoe[6] == tictactoe[7] && tictactoe[6] == tictactoe[8] && tictactoe[6] == O) ||
	   (tictactoe[0] == tictactoe[3] && tictactoe[0] == tictactoe[6] && tictactoe[0] == O) ||
	   (tictactoe[1] == tictactoe[4] && tictactoe[1] == tictactoe[7] && tictactoe[1] == O) ||
	   (tictactoe[2] == tictactoe[5] && tictactoe[2] == tictactoe[8] && tictactoe[2] == O) ||
	   (tictactoe[0] == tictactoe[4] && tictactoe[0] == tictactoe[8] && tictactoe[0] == O) ||
	   (tictactoe[2] == tictactoe[4] && tictactoe[2] == tictactoe[6] && tictactoe[2] == O)) {
			puts("Player O wins!\n\n");
			ende = true;
	}
	else {
		if(tictactoe[0] != Leer && tictactoe[1] != Leer && tictactoe[2] != Leer &&
		   tictactoe[3] != Leer && tictactoe[4] != Leer && tictactoe[5] != Leer &&
		   tictactoe[6] != Leer && tictactoe[7] != Leer && tictactoe[8] != Leer) {
			   puts("Remis!\n\n");
			ende = true;
		}
	}
}

int ConvertToInt(char c) {
	return((int)(c) - '1' + 1);
}

char x = 0;

void Zug(int Player) {
	putch('\n');
	helper();
	char x = 0;
	putch('\n');
	for(; ; x = *gets(&x)) {
		if(!isdigit(x) || x == '9') {
			puts("Please type in a number betwen 0 and 8.\n\n");
		}
		else if(tictactoe[ConvertToInt(x)] != Leer) {
			puts("You cannot type in an number which has been used already.\n\n");
		}
		else {
			break;
		}
	}
	tictactoe[ConvertToInt(x)] = Player;
	printField();
	gewinnen();
}

int main()
{
    clearScreen(1);
    settextcolor(11,0);
    puts("================================================================================\n");
    puts("                            Mr.X TicTacToe 3x3  v0.31                           \n");
    puts("--------------------------------------------------------------------------------\n");

	Zug(X);

	for(int i = 0; i < 4 && !ende; ++i) {
		Zug(O);
		if(ende) {
			break;
		}
		Zug(X);
	}
    settextcolor(15,0);
	return 0;
}
