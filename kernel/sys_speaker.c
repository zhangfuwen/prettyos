/*
   simulation in qemu: -soundhw pcspk
*/


#include "sys_speaker.h"

void sound(uint32_t frequency)
{
    outportb(0x43, 0xB6); //sending magic byte
    //writing frequency
    frequency = 1193180/frequency; // PIT (programable interrupt timer)
    outportb(0x42, frequency & 0xFF);
    outportb(0x42, frequency>>8);
    //Sound on
    outportb(0x61, inportb(0x61)|3 );
}

void noSound()
{
    outportb(0x61, inportb(0x61) & ~3 );
}

void msgbeep()
{
	beep(440,1000);
}

void beep(uint32_t freq, uint32_t duration)
{
	sound(freq);
	sti();
	sleepMilliSeconds(duration);
	noSound();
}
