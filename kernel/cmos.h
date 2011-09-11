#ifndef CMOS_H
#define CMOS_H

#include "os.h"

#define CMOS_SECOND      0x00
#define CMOS_ALARMSECOND 0x01
#define CMOS_MINUTE      0x02
#define CMOS_ALARMMINUTE 0x03
#define CMOS_HOUR        0x04
#define CMOS_ALARMHOUR   0x05
#define CMOS_WEEKDAY     0x06
#define CMOS_DAYOFMONTH  0x07
#define CMOS_MONTH       0x08
#define CMOS_YEAR        0x09
#define CMOS_CENTURY     0x32
#define CMOS_FLOPPYTYPE  0x10
#define CMOS_DEVICES     0x14


uint8_t cmos_read(uint8_t off); // Read byte from CMOS
void cmos_write(uint8_t off, uint8_t val); // Write byte to CMOS


#endif
