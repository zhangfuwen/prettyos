#ifndef USERLIB_H
#define USERLIB_H

#define true		1
#define false		0

#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isalnum(c) (isdigit(c) || isalpha(c))
#define isupper(character) ((character) >= 'A' && (character) <= 'Z')
#define islower(character) ((character) >= 'a' && (character) <= 'z')

// syscalls
void settextcolor(unsigned int foreground, unsigned int background);
void putch(unsigned char val);
void puts(char* pString);
unsigned char getch();
int floppy_dir();
void printLine(char* message, unsigned int line, unsigned char attribute);
unsigned int getCurrentSeconds();
unsigned int getCurrentMilliseconds();
int floppy_format(char* volumeLabel);
int floppy_load(char* name, char* ext);
void exit();
void settaskflag(int i);
void beep(unsigned int frequency, unsigned int duration);

// user functions
unsigned int strlen(const char* str);
char toLower(char c);
char toUpper(char c);
char* toupper( char* s );
char* tolower( char* s );
int strcmp( const char* s1, const char* s2 );
char* gets(char* s);
void test();
void reverse(char* s);
void itoa(int n, char* s);
int atoi(char* s);
void showInfo(signed char val);


#endif
