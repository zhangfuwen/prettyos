#ifndef USERLIB_H
#define USERLIB_H

#define TRUE		1
#define FALSE		0

// syscalls
void settextcolor(unsigned int foreground, unsigned int background);
void putch(char val);
void puts(char* pString);
char getch();
void floppy_dir();

// user functions
int strcmp( const char* s1, const char* s2 );

#endif
