#ifndef CMOS_H
#define CMOS_H

#include "os.h"
#define CMOS_ADDRESS  0x70
#define CMOS_DATA     0x71

uint8_t cmos_read(uint8_t off); // Read byte from CMOS
void cmos_write(uint8_t off,uint8_t val); // Write byte to CMOS

#endif
