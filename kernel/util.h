#ifndef UTIL_H
#define UTIL_H

#include "os.h"


#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isalnum(c) (isdigit(c) || isalpha(c))
#define isupper(c) ((c) >= 'A' && (c) <= 'Z')
#define islower(c) ((c) >= 'a' && (c) <= 'z')

#define BYTE1(a) ( (a)      & 0xFF)
#define BYTE2(a) (((a)>> 8) & 0xFF)
#define BYTE3(a) (((a)>>16) & 0xFF)
#define BYTE4(a) (((a)>>24) & 0xFF)

#define max(a, b) ((a) >= (b) ? (a) : (b))
#define min(a, b) ((a) <= (b) ? (a) : (b))

#define NAN (__builtin_nanf(""))
#define NULL 0

#define BIT(n) (1<<(n))
#define IS_BIT_SET(value, pos) ((value)&BIT(pos))
#define offsetof(st, m) ((size_t) ( (char*)&((st *)(0))->m - (char*)0 ))
#define getField(addr, byte, shift, len) ((((uint8_t*)(addr))[byte]>>(shift)) & (BIT(len)-1))

#define ASSERT(b) ((b) ? (void)0 : panic_assert(__FILE__, __LINE__, #b))
void panic_assert(const char* file, uint32_t line, const char* desc);


static inline void nop() { __asm__ volatile ("nop"); } // Do nothing
static inline void hlt() { __asm__ volatile ("hlt"); } // Wait until next interrupt
static inline void sti() { __asm__ volatile ("sti"); } // Enable interrupts
static inline void cli() { __asm__ volatile ("cli"); } // Disable interrupts
static inline uint64_t rdtsc()
{
    uint64_t val;
    __asm__ volatile ("rdtsc" : "=A"(val)); // "=A" for getting 64 bit value
    return val;
}

static inline uint8_t inportb(uint16_t port)
{
    uint8_t ret_val;
    __asm__ volatile ("inb %%dx,%%al" : "=a"(ret_val) : "d"(port));
    return ret_val;
}

static inline uint16_t inportw(uint16_t port)
{
    uint16_t ret_val;
    __asm__ volatile ("inw %%dx,%%ax" : "=a" (ret_val) : "d"(port));
    return ret_val;
}

static inline uint32_t inportl(uint16_t port)
{
    uint32_t ret_val;
    __asm__ volatile ("inl %%dx,%%eax" : "=a" (ret_val) : "d"(port));
    return ret_val;
}

static inline void outportb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %%al,%%dx" :: "a"(val), "d"(port));
}

static inline void outportw(uint16_t port, uint16_t val)
{
    __asm__ volatile ("outw %%ax,%%dx" :: "a"(val), "d"(port));
}

static inline void outportl(uint16_t port, uint32_t val)
{
    __asm__ volatile ("outl %%eax,%%dx" : : "a"(val), "d"(port));
}

void      memshow(const void* start, size_t count, bool alpha);
void*     memset(void* dest, int8_t val, size_t bytes);
uint16_t* memsetw(uint16_t* dest, uint16_t val, size_t words);
uint32_t* memsetl(uint32_t* dest, uint32_t val, size_t dwords);
void*     memcpy(void* dest, const void* src, size_t count);
void*     memmove(void* destination, const void* source, size_t size);
int32_t   memcmp(const void* s1, const void* s2, size_t n);

size_t  vsnprintf(char *buffer, size_t length, const char *args, va_list ap);
size_t  snprintf (char *buffer, size_t length, const char *args, ...);
size_t  strlen(const char* str);
int32_t strcmp(const char* s1, const char* s2);
int32_t strncmp(const char* s1, const char* s2, size_t n);
char*   strcpy(char* dest, const char* src);
char*   strncpy(char* dest, const char* src, size_t n);
char*   strncpyandfill(char* dest, const char* src, size_t n, char val);
char*   strcat(char* dest, const char* src);
char*   strncat(char* dest, const char* src, size_t n);
char*   strchr(char* str, int character);

char  toLower(char c);
char  toUpper(char c);
char* toupper(char* s);
char* tolower(char* s);

char* gets(char* s);
void  waitForKeyStroke();

void systemControl(SYSTEM_CONTROL todo); // Reboot, Shutdown, ...

void bootscreen();

void   reverse(char* s);
int8_t ctoi(char c);
int    atoi(const char* s);
float  atof(const char* s);
char*  itoa(int32_t n,  char* s);
char*  utoa(uint32_t n, char* s);
void   ftoa(float f, char* buffer);
void   i2hex(uint32_t val, char* dest, int32_t len);

uint8_t BCDtoDecimal(uint8_t packedBCDVal);

uint32_t alignUp(uint32_t val, uint32_t alignment);
uint32_t alignDown(uint32_t val, uint32_t alignment);

uint32_t abs(int32_t arg);
double   fabs(double x);
double   sqrt(double x);

void srand(uint32_t val);
uint32_t rand();


#endif
