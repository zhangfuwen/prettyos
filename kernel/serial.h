#ifndef SERIAL_H
#define SERIAL_H

#include "os.h"

void init_serial(uint16_t comport);
void write_serial(uint16_t comport, char a);
int serial_recieved(uint16_t comport);
char read_serial(uint16_t comport);
int is_transmit_empty(uint16_t comport);

#endif