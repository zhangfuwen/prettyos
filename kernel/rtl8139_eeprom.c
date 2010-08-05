/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// http://www.bralug.de/wiki/Eprom_mit_Netzwerkkarte_flashen
// Serial EEPROM section

#include "rtl8139_eeprom.h"
#include "util.h"

int read_eeprom(long ioaddr, int location, int addr_len)
{
  int i;
  unsigned retval = 0;
  long ee_addr = ioaddr + Cfg9346;
  int read_cmd = location | (EE_READ_CMD << addr_len);

  // outp(ee_addr, EE_ENB & ~EE_CS);
  // outp(ee_addr, EE_ENB);
  outportb(ee_addr, EE_ENB & ~EE_CS);
  outportb(ee_addr, EE_ENB);
  
  // Shift the read command bits out
  for (i = 4 + addr_len; i >= 0; i--) 
  {
    int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
    outportb(ee_addr, EE_ENB | dataval);
	// outp(ee_addr, EE_ENB | dataval);
    eeprom_delay();
    // outp(ee_addr, EE_ENB | dataval | EE_SHIFT_CLK);
    outportb(ee_addr, EE_ENB | dataval | EE_SHIFT_CLK);
	eeprom_delay();
  }
  // outp(ee_addr, EE_ENB);
  outportb(ee_addr, EE_ENB);
  eeprom_delay();

  for (i = 16; i > 0; i--) 
  {
    // outp(ee_addr, EE_ENB | EE_SHIFT_CLK);
	outportb(ee_addr, EE_ENB | EE_SHIFT_CLK);
    eeprom_delay();
    // retval = (retval << 1) | ((inp(ee_addr) & EE_DATA_READ) ? 1 : 0);
	retval = (retval << 1) | ((inportb(ee_addr) & EE_DATA_READ) ? 1 : 0);
	
    // outp(ee_addr, EE_ENB);
	outportb(ee_addr, EE_ENB);
    eeprom_delay();
  }

  // Terminate the EEPROM access
  // outp(ee_addr, ~EE_CS);
  outportb(ee_addr, ~EE_CS);
  return retval;
}

int write_eeprom(long ioaddr, int location, int addr_len)
{
	return 0;
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
