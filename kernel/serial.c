/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/


//http://wiki.osdev.org/Serial_ports
// 'ported' and debugged by Cuervo


#include "serial.h"
#include "util.h"
#include "timer.h"


/*
COM Port	IO Port
COM1		3F8h
COM2		2F8h
COM3		3E8h
COM4		2E8h
*/


void init_serial(uint16_t comport) {
	outportb(comport + 1, 0x00);    // Disable all interrupts
	outportb(comport + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outportb(comport + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud (DO NOT TRY 0!!!)
	outportb(comport + 1, 0x00);    //                  (hi byte)
	outportb(comport + 3, 0x03);    // 8 bits, no parity, one stop bit
	outportb(comport + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outportb(comport + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}


int serial_recieved(uint16_t comport) {
	return inportb(comport + 5) & 1;
}

char read_serial(uint16_t comport) {
	while (serial_recieved(comport) == 0);
	
	return inportb(comport);
}


int is_transmit_empty(uint16_t comport) {
	return inportb(comport + 5) & 0x20;
}

void write_serial(uint16_t comport, char a) {
	while (is_transmit_empty(comport) == 0);
	
	outportb(comport,a);
}




/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
