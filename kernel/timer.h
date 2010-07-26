#ifndef FLPYDSK_H
#define FLPYDSK_H

#include "scheduler.h"

void     timer_install(uint16_t sysfreq);
void     timer_handler(registers_t* r);
void     timer_setFrequency(uint32_t freq);
uint16_t timer_getFrequency();
uint32_t timer_getSeconds();
uint32_t timer_getMilliseconds();
void     timer_uninstall();

bool timer_unlockTask(task_t* task);

void timer_wait(uint32_t ticks);
void sleepSeconds(uint32_t seconds);
void sleepMilliSeconds(uint32_t ms);
void delay(uint32_t microsec);

#endif
