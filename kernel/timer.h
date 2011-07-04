#ifndef TIMER_H
#define TIMER_H

#include "os.h"

void     timer_install(uint16_t sysfreq);
void     timer_handler(registers_t* r);
void     timer_setFrequency(uint32_t freq);
uint16_t timer_getFrequency();
uint32_t timer_getSeconds();
uint32_t timer_getMilliseconds();
uint64_t timer_getTicks();
uint32_t timer_millisecondsToTicks(uint32_t milliseconds);

void timer_wait(uint32_t ticks);
void sleepSeconds(uint32_t seconds);
void sleepMilliSeconds(uint32_t ms);
void delay(uint32_t microsec);

#endif
