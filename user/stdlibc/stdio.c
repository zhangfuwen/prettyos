#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"


void  i2hex(uint32_t val, char* dest, uint32_t len); // -> Userlib
char* itoa(int32_t n, char* s); // -> Userlib
char* utoa(unsigned int n, char* s); // -> Userlib
void  ftoa(float f, char* buffer); // -> Userlib


FILE* stderr;
FILE* stdin;
FILE* stdout;


FILE* fopen(const char* path, const char* mode); // -> Syscall
FILE* tmpfile(); /// TODO
FILE* freopen(const char* filename, const char* mode, FILE* file); /// TODO
int fclose(FILE* file); // -> Syscall
int remove(const char* path); /// TODO
int rename(const char* oldpath, const char* newpath); /// TODO
int fputc(char c, FILE* file); // -> Syscall
int putc(char c, FILE* file); /// TODO
char fgetc(FILE* file); // -> Syscall
char getc(FILE* file); /// TODO
int ungetc(char c, FILE* file); /// TODO
char* fgets(char* dest, size_t num, FILE* file); /// TODO
int fputs(const char* src, FILE* file); /// TODO
size_t fread(void* dest, size_t size, size_t count, FILE* file); /// TODO
size_t fwrite(const void* src, size_t size, size_t count, FILE* file); /// TODO
int fflush(FILE* file); // -> Syscall
size_t ftell(FILE* file); /// TODO
int fseek(FILE* file, size_t offset, SEEK_ORIGIN origin); // -> Syscall
int rewind(FILE* file); /// TODO
int feof(FILE* file); /// TODO
int ferror(FILE* file); /// TODO
void clearerr(FILE* file); /// TODO
int fgetpos(FILE* file, fpos_t* position); /// TODO
int fsetpos(FILE* file, const fpos_t* position); /// TODO
int vfprintf(FILE* file, const char* format, va_list arg); /// TODO
int fprintf(FILE* file, const char* format, ...); /// TODO
int fscanf(FILE* file, const char* format, ...); /// TODO
void setbuf(FILE* file, char* buffer); /// TODO
int setvbuf(FILE* file, char* buffer, int mode, size_t size); /// TODO
char* tmpnam(char* str); /// TODO



void perror(const char* string); /// TODO



char getcharar(); // -> Syscall
char* gets(char* str)
{
    int32_t i = 0;
    char c;
    do
    {
        c = getchar();
        if (c=='\b')  // Backspace
        {
           if (i>0)
           {
              putchar(c);
              str[i-1]='\0';
              --i;
           }
        }
        else
        {
            if(c != '\n')
            {
                str[i] = c;
                i++;
            }
            putchar(c);
        }
    }
    while (c != '\n'); // Linefeed
    str[i]='\0';

    return str;
}

int scanf(const char* format, ...); /// TODO
int putchar(char c); // -> Syscall
int puts(const char* str)
{
    for (size_t i = 0; str[i] != 0; i++)
    {
        putchar(str[i]);
    }
    return(0);
}

int vprintf(const char* format, va_list arg)
{
    char buffer[32]; // Larger is not needed at the moment

    int pos = 0;
    for (; *format; format++)
    {
        switch (*format)
        {
        case '%':
            switch (*(++format))
            {
            case 'u':
                utoa(va_arg(arg, uint32_t), buffer);
                puts(buffer);
                pos += strlen(buffer);
                break;
            case 'f':
                ftoa(va_arg(arg, double), buffer);
                puts(buffer);
                pos += strlen(buffer);
                break;
            case 'i': case 'd':
                itoa(va_arg(arg, int32_t), buffer);
                puts(buffer);
                pos += strlen(buffer);
                break;
            case 'X':
                i2hex(va_arg(arg, int32_t), buffer,8);
                puts(buffer);
                pos += strlen(buffer);
                break;
            case 'x':
                i2hex(va_arg(arg, int32_t), buffer,4);
                puts(buffer);
                pos += strlen(buffer);
                break;
            case 'y':
                i2hex(va_arg(arg, int32_t), buffer,2);
                puts(buffer);
                pos += strlen(buffer);
                break;
            case 's':
            {
                char* temp = va_arg (arg, char*);
                puts(temp);
                pos += strlen(temp);
                break;
            }
            case 'c':
                putchar((int8_t)va_arg(arg, int32_t));
                pos++;
                break;
            case '%':
                putchar('%');
                pos++;
                break;
            default:
                --format;
                --pos;
                break;
            }
            break;
        default:
            putchar(*format);
            pos++;
            break;
        }
    }
    return(pos);
}

int printf(const char* format, ...)
{
    va_list arg;
    va_start(arg, format);
    int retval = vprintf(format, arg);
    va_end(arg);
    return(retval);
}




int vsprintf(char* dest, const char* format, va_list arg)
{
    int pos = 0;
    char m_buffer[32]; // Larger is not needed at the moment
    dest[0] = '\0';

    for (; *format; format++)
    {
        switch (*format)
        {
            case '%':
                switch (*(++format))
                {
                    case 'u':
                        utoa(va_arg(arg, uint32_t), m_buffer);
                        strcat(dest, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'f':
                        ftoa(va_arg(arg, double), m_buffer);
                        strcat(dest, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'i': case 'd':
                        itoa(va_arg(arg, int32_t), m_buffer);
                        strcat(dest, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'X':
                        i2hex(va_arg(arg, int32_t), m_buffer,8);
                        strcat(dest, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'x':
                        i2hex(va_arg(arg, int32_t), m_buffer,4);
                        strcat(dest, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'y':
                        i2hex(va_arg(arg, int32_t), m_buffer,2);
                        strcat(dest, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 's':
                        strcat(dest, va_arg(arg, char*));
                        pos = strlen(dest) - 1;
                        break;
                    case 'c':
                        dest[pos] = (int8_t)va_arg(arg, int32_t);
                        break;
                    case '%':
                        dest[pos] = '%';
                        break;
                    default:
                        --format;
                        --pos;
                        break;
                    }
                break;
            default:
                dest[pos] = (*format);
                break;
        }
        pos++;
        dest[pos] = '\0';
    }
    return(pos);
}

int sprintf(char* dest, const char* format, ...)
{
    va_list arg;
    va_start(arg, format);
    int retval = vsprintf(dest, format, arg);
    va_end(arg);
    return(retval);
}

int sscanf(const char* src, const char* format, ...); /// TODO
