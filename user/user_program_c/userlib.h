#ifndef USERLIB_H
#define USERLIB_H

#define true		1
#define false		0

// syscalls
void settextcolor(unsigned int foreground, unsigned int background);
void putch(unsigned char val);
void puts(char* pString);
unsigned char getch();
int floppy_dir();
void printLine(char* message, unsigned int line, unsigned char attribute);



// user functions
int strcmp( const char* s1, const char* s2 );

void showInfo(signed char val);

#endif
