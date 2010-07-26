#include "userlib.h"

char replaceBuf[1000];
char* replace(char* source, int Pos, int length, char* toBeInserted) {
	char end[500];
	memset(end, 0, 500);
	strcpy(end, source+Pos+length);
	char begin[500];
	memset(begin, 0, 500);
	strncpy(begin, source, Pos);
	memset(replaceBuf, 0, 1000);
	strcat(replaceBuf, begin);
	strcat(replaceBuf, toBeInserted);
	strcat(replaceBuf, end);
	return(replaceBuf);
}

char GetPrevNumBuf[100];
int getPrevNumber(unsigned int Pos, char* string) {
	memset(GetPrevNumBuf, 0, 100);
	for(int i = Pos-1; i >= 0; i--) {
		if(!isdigit(string[i])) {
			strncpy(GetPrevNumBuf, string+i+1, Pos-i+1);
			return(atoi(GetPrevNumBuf));
		}
	}
	strncpy(GetPrevNumBuf, string, Pos);
	return(atoi(GetPrevNumBuf));
}
int getPrevNumberPos(unsigned int Pos, char* string) {
	for(int i = Pos-1; i >= 0; i--) {
		if(!isdigit(string[i])) {
			return(i+1);
		}
	}
	return(0);
}
char GetNextNumBuf[100];
int getNextNumber(unsigned int Pos, char* string) {
	for(int i = Pos+1; ; i++) {
		if(!isdigit(string[i])) {
			memset(GetNextNumBuf, 0, 100);
			strncpy(GetNextNumBuf, string+Pos+1, i-Pos+1);
			return(atoi(GetNextNumBuf));
		}
	}
}
int getNextNumberPos(unsigned int Pos, char* string) {
	for(int i = Pos+1; ; i++) {
		if(!isdigit(string[i])) {
			return(i-1);
		}
	}
}

int find_first(char* string, char* search) {
	for(int i = 0; string[i] != 0; i++) {
		for(int j = 0; search[j] != 0; j++) {
			if(string[i] == search[j]) {
				return(i);
			}
		}
	}
	return(-1);
}

int32_t CalcTerm(char* term) {
	char temp[100];
	int point = 0;
	int erg = 0;
	while(find_first(term, "*/") != -1) {
		point = find_first(term, "*/");
		if(term[point] == '*') {
			erg = getPrevNumber(point, term)*getNextNumber(point, term);
		}
		else {
			erg = getPrevNumber(point, term)/getNextNumber(point, term);
		}
		memset(temp, 0, 100);
		itoa(erg, temp);
		term = replace(term, getPrevNumberPos(point, term), getNextNumberPos(point, term) - getPrevNumberPos(point, term)+1, temp);
		puts(term); putch('\n');
	}
	while(find_first(term, "+-") != -1) {
		point = find_first(term, "+-");
		if(term[point] == '+') {
			erg = getPrevNumber(point, term)+getNextNumber(point, term);
		}
		else {
			erg = getPrevNumber(point, term)-getNextNumber(point, term);
		}
		memset(temp, 0, 100);
		itoa(erg, temp);
		term = replace(term, getPrevNumberPos(point, term), getNextNumberPos(point, term) - getPrevNumberPos(point, term)+1, temp);
		puts(term); putch('\n');
	}
	return(atoi(term));
}

int main() {
    textColor(0x0B);
    puts("================================================================================\n");
    puts("                             Mr.X Calculator  v0.3.2                            \n");
    puts("--------------------------------------------------------------------------------\n\n");
	puts("Please type in your term. Valid operators are: +-*/. The Calculator does not support floats at the moment. If you type in \"exit\", the programm will close.\n");
	char term[100];
	for(;;) {
		memset(term, 0, 100);
		gets(term);
		if(term[0] == 'e' && term[1] == 'x' && term[2] == 'i' && term[3] == 't') {
			break;
		}
		int32_t Erg = CalcTerm(term);
		printf("The result of your term is: %i\n\n", Erg);
	}
    textColor(0x0F);
	return(0);
}
