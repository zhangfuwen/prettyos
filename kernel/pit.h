#ifndef PIT_H
#define PIT_H

#include "os.h"


/*
Ports of the PIT chip:
0x40         Channel 0 data port (read/write)
0x41         Channel 1 data port (read/write)
0x42         Channel 2 data port (read/write) // output connected to the PC speaker
0x43         Mode/Command register (write only)

Channel 2 gate is the only channel where the input can be controlled by IO port 0x61, bit 0.
*/

#define TIMECOUNTER_i8254_FREQU  1193182
#define CHANNEL_0_DATAPORT       0x40
#define CHANNEL_1_DATAPORT       0x41
#define CHANNEL_2_DATAPORT       0x42
#define COMMANDREGISTER          0x43
#define CHANNEL_2_CONTROLPORT    0x61


#endif
