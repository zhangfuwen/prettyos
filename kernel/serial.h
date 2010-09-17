#ifndef SERIAL_H
#define SERIAL_H

#include "os.h"

void     serial_init();
void     serial_write(uint8_t com, char a);
int32_t  serial_received(uint8_t com);
char     serial_read(uint8_t com);
int32_t  serial_isTransmitEmpty(uint8_t com);

static inline void serial_log(const char* msg)
{
    #ifdef _SERIAL_LOG_
    while(*msg != 0)
    {
        serial_write(1, *msg);
        msg++;
    }
    #endif
}

#endif
