#ifndef SERIAL_H
#define SERIAL_H

#include "os.h"

void serial_init();
void write_serial(uint8_t com, char a);
int serial_recieved(uint8_t com);
char read_serial(uint8_t com);
int is_transmit_empty(uint8_t com);

#endif
