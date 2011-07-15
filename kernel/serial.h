#ifndef SERIAL_H
#define SERIAL_H

#include "os.h"
#include "util.h"

void     serial_init();
void     serial_write(uint8_t com, char a);
int32_t  serial_received(uint8_t com);
char     serial_read(uint8_t com);
int32_t  serial_isTransmitEmpty(uint8_t com);

/*
   VBox, Qemu: implement serial interface with output to a file, e.g. called "serial1.txt" (qemu: -serial file:serial1.txt)
   Outputs are used in vm86.c and tcp.c
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

static inline void serial_logUINT(uint8_t com, uint32_t num)
{   
  #ifdef _SERIAL_LOG_
    char msg[20];
    utoa(num, msg);
    uint32_t i=0;
    while(*(msg+i))
    {
        serial_write(com, *(msg+i));
        i++;
    }
  #endif
}

static inline void serial_logINT(uint8_t com, int32_t num)
{   
  #ifdef _SERIAL_LOG_
    char msg[20];
    itoa(num, msg);
    uint32_t i=0;
    while(*(msg+i))
    {
        serial_write(com, *(msg+i));
        i++;
    }
  #endif
}

#endif
