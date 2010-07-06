#ifndef USERLIB_H
#define USERLIB_H

#include "types.h"

#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isalnum(c) (isdigit(c) || isalpha(c))
#define isupper(character) ((character) >= 'A' && (character) <= 'Z')
#define islower(character) ((character) >= 'a' && (character) <= 'z')

// syscalls
void textColor(uint8_t color);
void putch(unsigned char val);
void puts(const char* pString);
unsigned char getch();
int floppy_dir();
void printLine(const char* message, unsigned int line, unsigned char attribute);
unsigned int getCurrentSeconds();
int floppy_format(char* volumeLabel);
void exit();
bool keyPressed(VK key);
FS_ERROR execute(const char* path);
void beep(unsigned int frequency, unsigned int duration);
void clearScreen(unsigned char backgroundColor);
void gotoxy(unsigned char x, unsigned char y);
void* grow_heap(unsigned increase);
void setScrollField(uint8_t top, uint8_t bottom);
void systemControl(SYSTEM_CONTROL todo);

// user functions
void* memset(void* dest, int8_t val, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
void printf(const char *args, ...);
void vprintf(const char* args, va_list ap);
void sprintf(char *buffer, const char *args, ...);
void snprintf(char *buffer, size_t length, const char *args, ...);
void vsnprintf(char *buffer, size_t length, const char *args, va_list ap);

char toLower(char c);
char toUpper(char c);
char* toupper(char* s);
char* tolower(char* s);

size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t n);
char* strchr(char* str, int character);

char* gets(char* s);

void  reverse(char* s);
char* itoa(int32_t n, char* s);
char* utoa(uint32_t n, char* s);
void  ftoa(float f, char* buffer);
int   atoi(const char* s);
float atof(const char* s);
void  i2hex(uint32_t val, char* dest, int32_t len);

void showInfo(signed char val);

void* malloc(size_t size);
void free(void* mem);

void srand(unsigned int val);
unsigned int rand();

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

#endif
