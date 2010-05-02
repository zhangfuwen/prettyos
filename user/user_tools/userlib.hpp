#ifndef USERLIB_HPP
#define USERLIB_HPP

#define __bool_true_false_are_defined 1 // bool is a builtin type in C++

#include "types.h"

#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isalnum(c) (isdigit(c) || isalpha(c))
#define isupper(character) ((character) >= 'A' && (character) <= 'Z')
#define islower(character) ((character) >= 'a' && (character) <= 'z')

extern "C" { // Functions from C-Userlib
    // syscalls
    void textColor(uint8_t color);
    void putch(unsigned char val);
    void puts(const char* pString);
    unsigned char getch();
    int floppy_dir();
    void printLine(const char* message, unsigned int line, unsigned char attribute);
    unsigned int getCurrentSeconds();
    int floppy_format(char* volumeLabel);
    int floppy_load(const char* name, const char* ext);
    void exit();
    void settaskflag(int i);
    void beep(unsigned int frequency, unsigned int duration);
    void clearScreen(unsigned char backgroundColor);
    void gotoxy(unsigned char x, unsigned char y);
    void* grow_heap(unsigned increase);
    void setScrollField(uint8_t top, uint8_t bottom);

    // user functions
	void* memset(void* dest, int8_t val, size_t count);
	void* memcpy(void* dest, const void* src, size_t count);
	void printf (const char *args, ...);
	void sprintf (char *buffer, const char *args, ...);

	char toLower(char c);
	char toUpper(char c);
	char* toupper(char* s);
	char* tolower(char* s);

	unsigned int strlen(const char* str);
	int strcmp(const char* s1, const char* s2);
	char* strcpy(char* dest, const char* src);
	char* strncpy(char* dest, const char* src, size_t n);
	char* strcat(char* dest, const char* src);
	char* strchr(char* str, int character);

	char* gets(char* s);

	void reverse(char* s);
	void itoa(int n, char* s);
	void ftoa(float f, char* buffer);
	int atoi(const char* s);
	float atof(const char* s);

	void i2hex(uint32_t val, char* dest, int32_t len);

	void showInfo(signed char val);

	void* malloc(size_t size);
	void free(void* mem);

    // math functions
    #define NAN (__builtin_nanf (""))
    #define pi 3.1415926535897932384626433832795028841971693993

    double cos(double x);
    double sin(double x);
    double tan(double x);

    double acos(double x);
    double asin(double x);
    double atan(double x);
    double atan2(double x, double y); 

    double sqrt(double x);

    double fabs(double x);
}

#endif
