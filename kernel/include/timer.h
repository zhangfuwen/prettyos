#ifndef FLPYDSK_H
#define FLPYDSK_H

#include "os.h"

void timer_install(uint16_t sysfreq);
void timer_handler(registers_t* r);
void timer_wait(uint32_t ticks);
uint32_t timer_getSeconds();
uint32_t timer_getMilliseconds();
void sleepSeconds(uint32_t seconds);
void sleepMilliSeconds(uint32_t ms);
void systemTimer_setFrequency(uint32_t freq);
uint16_t systemTimer_getFrequency();
void delay(uint32_t microsec);
void timer_uninstall();

#endif
