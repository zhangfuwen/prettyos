#ifndef UTIL_H
#define UTIL_H

#include "os.h"

#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isalnum(c) (isdigit(c) || isalpha(c))
#define isupper(character) ((character) >= 'A' && (character) <= 'Z')
#define islower(character) ((character) >= 'a' && (character) <= 'z')

#define FORM_SHORT(a,b) (((b)<<8)|a)
#define FORM_LONG(a,b,c,d) (((d)<<24)|((c)<<16)|((b)<<8)|(a))
#define BYTE1(a) ( (a)       & 0xFF)
#define BYTE2(a) (((a)>> 8 ) & 0xFF)
#define BYTE3(a) (((a)>>16 ) & 0xFF)
#define BYTE4(a) (((a)>>24 ) & 0xFF)

#define ASSERT(b) ((b) ? (void)0 : panic_assert(__FILE__, __LINE__, #b))
void panic_assert(const char* file, uint32_t line, const char* desc);  // why char ?


uint8_t  inportb (uint16_t port);
uint16_t inportw (uint16_t port);
uint32_t inportl (uint16_t port);
void     outportb(uint16_t port, uint8_t val);
void     outportw(uint16_t port, uint16_t val);
void     outportl(uint16_t port, uint32_t val);

uint32_t fetchESP();
uint32_t fetchEBP();
uint32_t fetchSS();
uint32_t fetchCS();
uint32_t fetchDS();

uint64_t rdtsc();

void      memshow(void* start, size_t count);
void*     memset(void* dest, int8_t val, size_t count);
uint16_t* memsetw(uint16_t* dest, uint16_t val, size_t count);
uint32_t* memsetl(uint32_t* dest, uint32_t val, size_t count);
void*     memcpy(void* dest, const void* src, size_t count);

void    sprintf (char *buffer, const char *args, ...);
size_t  strlen(const char* str);
int32_t strcmp( const char* s1, const char* s2 );
char*   strcpy(char* dest, const char* src);
char*   strncpy(char* dest, const char* src, size_t n);
char*   strcat(char* dest, const char* src);

void reboot();

void bootscreen();

void cli();
void sti();
void nop();

int8_t ctoi(char c);
float  atof(const char* s);
void   itoa(int32_t value, char* valuestring);
void   ftoa(float f, char* buffer);
void   i2hex(uint32_t val, char* dest, int32_t len);

uint8_t PackedBCD2Decimal(uint8_t PackedBCDVal);

uint32_t alignUp( uint32_t val, uint32_t alignment );
uint32_t alignDown( uint32_t val, uint32_t alignment );

uint32_t max(uint32_t a, uint32_t b);
uint32_t min(uint32_t a, uint32_t b);
int32_t  power(int32_t base, int32_t n);
double   fabs(double x);

#endif