#include "userlib.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"
#include "stdlib.h"


static void replace(const char* source, char* dest, int Pos, int length, const char* toBeInserted) {
    strncpy(dest, source, Pos);
    dest[Pos] = 0;
    strcat(dest, toBeInserted);
    strcat(dest, source+Pos+length);
}

static int getPrevNumber(unsigned int Pos, const char* string) {
    for (int i = Pos-1; i >= 0; i--) {
        if (!isdigit(string[i])) {
            return(atoi(string+i+1));
        }
    }
    return(atoi(string));
}
static int getPrevNumberPos(unsigned int Pos, const char* string) {
    for (int i = Pos-1; i >= 0; i--) {
        if (!isdigit(string[i])) {
            return(i+1);
        }
    }
    return(0);
}
static int getNextNumber(unsigned int Pos, const char* string) {
    for (int i = Pos+1; ; i++) {
        if (!isdigit(string[i])) {
            return(atoi(string+Pos+1));
        }
    }
}
static int getNextNumberPos(unsigned int Pos, const char* string) {
    for (int i = Pos+1; ; i++) {
        if (!isdigit(string[i])) {
            return(i-1);
        }
    }
}

static int find_first(const char* string, const char* search) {
    for (int i = 0; string[i] != 0; i++) {
        for (int j = 0; search[j] != 0; j++) {
            if (string[i] == search[j]) {
                return(i);
            }
        }
    }
    return(-1);
}

static int32_t CalcTerm(char* term) {
    char temp[100];
    int erg = 0;
    int point = 0;
    while ((point = find_first(term, "*/")) != -1) {
        if (term[point] == '*') {
            erg = getPrevNumber(point, term)*getNextNumber(point, term);
        }
        else {
            erg = getPrevNumber(point, term)/getNextNumber(point, term);
        }
        itoa(erg, temp);
        char temp2[strlen(term)+1];
        strcpy(temp2, term);
        replace(temp2, term, getPrevNumberPos(point, term), getNextNumberPos(point, term) - getPrevNumberPos(point, term)+1, temp);
        printf("%s\n", term); // Debug output. Shows how the calculator solves terms
    }
    while ((point = find_first(term, "+-")) != -1) {
        if (term[point] == '+') {
            erg = getPrevNumber(point, term)+getNextNumber(point, term);
        }
        else {
            erg = getPrevNumber(point, term)-getNextNumber(point, term);
        }
        itoa(erg, temp);
        char temp2[strlen(term)+1];
        strcpy(temp2, term);
        replace(temp2, term, getPrevNumberPos(point, term), getNextNumberPos(point, term) - getPrevNumberPos(point, term)+1, temp);
        printf("%s\n", term); // Debug output. Shows how the calculator solves terms
    }
    return(atoi(term));
}

int main() {
    textColor(0x0B);
    puts("================================================================================\n");
    puts("                             Mr.X Calculator  v0.3.3\n\n");
    puts("--------------------------------------------------------------------------------\n\n");
    puts("Please type in a term. Valid operators are: +-*/. The Calculator does not support floats at the moment. If you type in \"exit\", the programm will close.\n");
    char term[100];
    for (;;) {
        gets(term);
        if (strncmp(term, "exit", 4) == 0)
            break;

        int32_t Erg = CalcTerm(term);
        printf("The result of your term is: %i\n\n", Erg);
    }

    return(0);
}
