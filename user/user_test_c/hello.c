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

char x = 0;
char t[1];

void Zug(int Player) {
	helper();
	for(;;) {
		do {
			x = *gets(&x);
		}
		while (!isdigit(x) && x != '9');
		if(tictactoe[ConvertToInt(x)] != Leer){
			puts("unmöglich\n");
		}
		else {
			tictactoe[ConvertToInt(x)] = Player;
			break;
		}
	}
	printField();
	gewinnen();
}

int main() {
    settextcolor(11,0);
    puts("================================================================================\n");
    puts("                            Mr.X TicTacToe 3x3  v0.2                            \n");
    puts("--------------------------------------------------------------------------------\n\n");

	Zug(X);

	for(int i = 0; i < 4 && !ende; i++) {
		Zug(O);
		if(ende) {
			break;
		}
		Zug(X);
	}
    settextcolor(15,0);
	return 0;
}