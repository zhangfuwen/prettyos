#ifndef FLPYDSK_H
#define FLPYDSK_H

#include "os.h"

void timer_install(uint16_t sysfreq);
void timer_handler(registers_t* r);
void timer_wait(uint32_t ticks);
void sleepSeconds(uint32_t seconds);
void sleepMilliSeconds(uint32_t ms);
void systemTimer_setFrequency(uint32_t freq);
uint32_t getCurrentSeconds();
uint16_t systemTimer_getFrequency();
void timer_uninstall();
void delay(uint32_t microsec);

#endif
