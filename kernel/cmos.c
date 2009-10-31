#include "cmos.h"

// Read byte from CMOS
uint8_t cmos_read(uint8_t off)
{
     uint8_t tmp = inportb(CMOS_ADDRESS);
     outportb( CMOS_ADDRESS, (tmp & 0x80) | (off & 0x7F) );
     return inportb(CMOS_DATA);
}

// Write byte to CMOS
void cmos_write(uint8_t off,uint8_t val)
{
    uint8_t tmp = inportb(CMOS_ADDRESS);
    outportb(CMOS_ADDRESS, (tmp & 0x80) | (off & 0x7F));
    outportb(CMOS_DATA,val);
}
