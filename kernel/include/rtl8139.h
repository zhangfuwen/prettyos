#ifndef RTL8139_H
#define RTL8139_H

#include "os.h"

void rtl8139_handler(registers_t* r);
void install_RTL8139(uint32_t number);

#endif
