#ifndef STDIO_H
#define STDIO_H

#include "stdarg.h"
#include "stddef.h"

#define EOF -1
#define FILENAME_MAX
#define TMP_MAX

typedef enum
{
    SEEK_SET, SEEK_CUR, SEEK_END
} SEEK_ORIGIN;

typedef struct file
{ // HACK; Should be filled
} file_t;

typedef file_t FILE;
typedef unsigned int fpos_t;


extern FILE* stderr;
extern FILE* stdin;
extern FILE* stdout;


#ifdef _cplusplus
extern "C" {
#endif

FILE* fopen(const char* path, const char* mode);
FILE* tmpfile();
FILE* freopen(const char* filename, const char* mode, FILE* file);
int fclose(FILE* file);
int remove(const char* path);
int rename(const char* oldpath, const char* newpath);
int fputc(char c, FILE* file);
int putc(char c, FILE* file);
char fgetc(FILE* file);
char getc(FILE* file);
int ungetc(char c, FILE* file);
char* fgets(char* dest, size_t num, FILE* file);
int fputs(const char* src, FILE* file);
size_t fread(void* dest, size_t size, size_t count, FILE* file);
size_t fwrite(const void* src, size_t size, size_t count, FILE* file);
int fflush(FILE* file);
size_t ftell(FILE* file);
int fseek(FILE* file, size_t offset, SEEK_ORIGIN origin);
int rewind(FILE* file);
int feof(FILE* file);
int ferror(FILE* file);
void clearerr(FILE* file);
int fgetpos(FILE* file, fpos_t* position);
int fsetpos(FILE* file, const fpos_t* position);
int vfprintf(FILE* file, const char* format, va_list arg);
int fprintf(FILE* file, const char* format, ...);
int fscanf(FILE* file, const char* format, ...);
void setbuf(FILE* file, char* buffer);
int setvbuf(FILE* file, char* buffer, int mode, size_t size);
char* tmpnam(char* str);

void perror(const char* string);

char getchar();
char* gets(char* str);
int scanf(const char* format, ...);
int putchar(char c);
int puts(const char* str);
int vprintf(const char* format, va_list arg);
int printf(const char* format, ...);

int vsprintf(char* dest, const char* format, va_list arg);
int sprintf(char* dest, const char* format, ...);
int sscanf(const char* src, const char* format, ...);

#ifdef _cplusplus
}
#endif


#endif
