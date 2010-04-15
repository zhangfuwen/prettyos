#ifndef CDI_CMOS_H
#define CDI_CMOS_H

#include "os.h"

/* Performs CMOS reads
   index:  Index within CMOS RAM to read from.
   return: Result of reading CMOS RAM at specified index. */
uint8_t cdi_cmos_read(uint8_t index);

/* Performs CMOS writes
   index: Index within CMOS RAM to write to.
   value: Value to write to the CMOS */
void cdi_cmos_write(uint8_t index, uint8_t value);

#endif
