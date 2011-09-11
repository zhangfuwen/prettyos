#ifndef SERIAL_H
#define SERIAL_H

#include "os.h"


// Most emulators implement serial interface with output to a file, e.g. called "serial1.txt" (qemu: -serial file:serial1.txt)


void serial_init();
void serial_write(uint8_t com, char a);
bool serial_received(uint8_t com);
char serial_read(uint8_t com);
bool serial_isTransmitEmpty(uint8_t com);
#ifdef _SERIAL_LOG_
void serial_log(uint8_t com, const char* msg, ...);
#else
static inline void serial_log(uint8_t com, const char* msg, ...)
{
}
#endif


#endif
