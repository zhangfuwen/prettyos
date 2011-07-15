#ifndef SERIAL_H
#define SERIAL_H

#include "os.h"
#include "util.h"


void    serial_init();
void    serial_write(uint8_t com, char a);
char    serial_received(uint8_t com);
char    serial_read(uint8_t com);
int32_t serial_isTransmitEmpty(uint8_t com);

/*
   Most emulators implement serial interface with output to a file, e.g. called "serial1.txt" (qemu: -serial file:serial1.txt)
   Outputs are used in vm86.c and tcp.c
*/
static inline void serial_log(uint8_t com, const char* msg, ...)
{
  #ifdef _SERIAL_LOG_
    va_list ap;
    va_start(ap, msg);
    size_t length = strlen(msg) + 100;
    char array[length]; // HACK: Should be large enough.
    vsnprintf(array, length, msg, ap);
    for(size_t i = 0; i < length && array[i] != 0; i++)
    {
        serial_write(com, array[i]);
    }
  #endif
}


#endif
