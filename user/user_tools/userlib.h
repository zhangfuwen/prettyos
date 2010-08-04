#ifndef USERLIB_H
#define USERLIB_H

#include "types.h"

#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isalnum(c) (isdigit(c) || isalpha(c))
#define isupper(character) ((character) >= 'A' && (character) <= 'Z')
#define islower(character) ((character) >= 'a' && (character) <= 'z')

// syscalls
FS_ERROR execute(const char* path);
void exit();
void taskSleep(uint32_t duration);
uint32_t getMyPID();

void* userheapAlloc(size_t increase);

file_t* fopen(const char* path, const char* mode);
char fgetc(file_t* file);
FS_ERROR fputc(char value, file_t* file);
FS_ERROR fseek(file_t* file, size_t offset, SEEK_ORIGIN origin);
FS_ERROR fflush(file_t* file);
FS_ERROR fclose(file_t* file);

uint32_t getCurrentMilliseconds();

void systemControl(SYSTEM_CONTROL todo);

void putch(char val);
void textColor(uint8_t color);
void setScrollField(uint8_t top, uint8_t bottom);
void setCursor(position_t pos);
position_t getCursor();
void clearScreen(uint8_t backgroundColor);

char getch();
bool keyPressed(VK key);

void beep(uint32_t frequency, uint32_t duration);

 // deprecated
int32_t floppy_dir();
int32_t floppy_format(char* volumeLabel);
void printLine(const char* message, uint32_t line, uint8_t attribute);

// user functions
void iSetCursor(uint16_t x, uint16_t y);
uint32_t getCurrentSeconds();

void* memset(void* dest, int8_t val, size_t count);
void* memcpy(void* dest, const void* src, size_t count);

void puts(const char* pString);
void printf(const char *args, ...);
void vprintf(const char* args, va_list ap);
void sprintf(char *buffer, const char *args, ...);
void vsprintf(char *buffer, const char *args, va_list ap);
void snprintf(char *buffer, size_t length, const char *args, ...);
void vsnprintf(char *buffer, size_t length, const char *args, va_list ap);

char toLower(char c);
char toUpper(char c);
char* toupper(char* s);
char* tolower(char* s);

size_t strlen(const char* str);
int32_t strcmp(const char* s1, const char* s2);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t n);
char* strchr(char* str, char character);

char* gets(char* s);

void    reverse(char* s);
char*   itoa(int32_t n, char* s);
char*   utoa(uint32_t n, char* s);
void    ftoa(float f, char* buffer);
int32_t atoi(const char* s);
float   atof(const char* s);
void    i2hex(uint32_t val, char* dest, uint32_t len);

void showInfo(uint8_t val);

void* malloc(size_t size);
void  free(void* mem);

void srand(uint32_t val);
uint32_t rand();

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
