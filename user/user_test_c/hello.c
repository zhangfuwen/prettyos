#include "userlib.h"

enum Feldstatus {Leer, X, O};

int tictactoe[9];
char ende = false;


void SetField(unsigned int x, unsigned int y, int Player) {
	gotoxy(x*4+2, y*2+15);
	if(Player == X) {
		putch('X');
	}
	else if(Player == O) {
		putch('O');
	}
	gotoxy(0, 22);
	puts("  ");
	gotoxy(0, 22);
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
			settextcolor(5,0);
			gotoxy(0,26);
			puts("Player X wins\n\n");
			settextcolor(15,0);
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
			settextcolor(5,0);
			gotoxy(0,26);
			puts("Player O wins!\n\n");
			settextcolor(15,0);
			ende = true;
	}
	else {
		if(tictactoe[0] != Leer && tictactoe[1] != Leer && tictactoe[2] != Leer && 
			tictactoe[3] != Leer && tictactoe[4] != Leer && tictactoe[5] != Leer && 
			tictactoe[6] != Leer && tictactoe[7] != Leer && tictactoe[8] != Leer) {
				settextcolor(5,0);
				gotoxy(0,26);
				puts("Remis!\n\n");
				settextcolor(15,0);
				ende = true;
		}
	}
}

int ConvertToInt(char c) {
	return((int)(c) - '1' + 1);
}

char x = 0;

void Zug(int Player) {
	char x = 0;
	for(; ; x = *gets(&x)) {
		if(!isdigit(x) || x == '9') {
		}
		else if(tictactoe[ConvertToInt(x)] != Leer) {
			settextcolor(4,0);
			gotoxy(0, 25);
			puts("You cannot type in an number which has been used already.\n\n");
			gotoxy(0, 22);
			settextcolor(15,0);
		}
		else {
			break;
		}
	}
	gotoxy(0, 25);
	puts("                                                                                ");
	gotoxy(0, 23);
	tictactoe[ConvertToInt(x)] = Player;
	SetField(ConvertToInt(x)%3, ConvertToInt(x)/3, Player);
	gewinnen();
}

int main() {
	clearScreen(0);
    settextcolor(11,0);
    puts("================================================================================\n");
    puts("                            Mr.X TicTacToe 3x3  v0.4                            \n");
    puts("--------------------------------------------------------------------------------\n\n");
	gotoxy(0, 24);
	puts("Please type in a number betwen 0 and 8.\n\n");
	gotoxy(0, 6);
    settextcolor(15,0);

	puts("*************\n| 0 | 1 | 2 |\n*************\n| 3 | 4 | 5 |\n*************\n| 6 | 7 | 8 |\n*************\n\n");
	puts("*************\n|   |   |   |\n*************\n|   |   |   |\n*************\n|   |   |   |\n*************\n\n");
	Zug(X);

	for(int i = 0; i < 4 && !ende; ++i) {
		Zug(O);
		if(ende) {
			break;
		}
		Zug(X);
	}
    settextcolor(15,0);
	gotoxy(0,28);
	return 0;
}