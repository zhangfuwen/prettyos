#ifndef RTL8139_EEPROM_H
#define RTL8139_EEPROM_H

//  EEPROM_Ctrl bits

#define EE_SHIFT_CLK  0x04  // EEPROM shift clock
#define EE_CS         0x08  // EEPROM chip select
#define EE_DATA_WRITE 0x02  // EEPROM chip data in
#define EE_WRITE_0    0x00
#define EE_WRITE_1    0x02
#define EE_DATA_READ  0x01  // EEPROM chip data out
#define EE_ENB       (0x80 | EE_CS)

// Delay between EEPROM clock transitions.
// No extra delay is needed with 33Mhz PCI, but 66Mhz may change this.

#define eeprom_delay()  inportb(ee_addr) // inpd(ee_addr)

// The EEPROM commands include the alway-set leading bit

#define EE_WRITE_CMD  (5)
#define EE_READ_CMD   (6)
#define EE_ERASE_CMD  (7)

#define Cfg9346 0x50 // this is allready in rtl8139.h

int read_eeprom(long ioaddr, int location, int addr_len);
int write_eeprom(long ioaddr, int location, int addr_len);

#endif
