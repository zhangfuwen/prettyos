#ifndef SERIAL_H
#define SERIAL_H

#include "os.h"

void     serial_init();
void     serial_write(uint8_t com, char a);
int32_t  serial_received(uint8_t com);
char     serial_read(uint8_t com);
int32_t  serial_isTransmitEmpty(uint8_t com);

/*
   VBox: implement serial interface with output to a file, e.g. called "serial1.txt"
   Outputs are used in vm86
*/
static inline void serial_log(uint8_t com, const char* msg)
{
    #ifdef _SERIAL_LOG_
    while(*msg)
    {
        serial_write(com, *msg);
        msg++;
    }
    #endif
}

#endif
