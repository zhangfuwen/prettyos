/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "timer.h"
#include "irq.h"
#include "pci.h"
#include "paging.h"
#include "video/console.h"
#include "ipTcpStack.h"
#include "rtl8139.h"

uint32_t BaseAddressRTL8139_IO;
uint32_t BaseAddressRTL8139_MMIO;

// to set the WRAP bit, an 8K buffer must in fact be 8 KiB + 16 byte + 1.5 KiB
// Rx buffer + header + largest potentially overflowing packet, if WRAP is set
uint8_t   network_buffer[RTL8139_NETWORK_BUFFER_SIZE]; // WRAP not set
uintptr_t network_bufferPointer = 0;

// Transmit
uint8_t  curBuffer = 0; // Tx descriptor
uint8_t  Tx_network_buffer[0x1000]; 

void rtl8139_handler(registers_t* r)
{
    textColor(0x03);
    printf("\n--------------------------------------------------------------------------------");

    // read bytes 003Eh bis 003Fh, Interrupt Status Register
    uint16_t val = *((uint16_t*)(BaseAddressRTL8139_MMIO + RTL8139_INTRSTATUS));
    char str[80];
    strcpy(str,"");
    if (val & RTL8139_INT_RX_OK)            { strcpy(str,"Receive OK"); }
    if (val & RTL8139_INT_RX_ERR)           { strcpy(str,"Receive Error"); }
    if (val & RTL8139_INT_TX_OK)            { strcpy(str,"Transmit OK"); }
    if (val & RTL8139_INT_TX_ERR)           { strcpy(str,"Transmit Error"); }
    if (val & RTL8139_INT_RXBUF_OVERFLOW)   { strcpy(str,"Rx Buffer Overflow");}
    if (val & RTL8139_INT_RXFIFO_UNDERRUN)  { strcpy(str,"Packet Underrun / Link change");}
    if (val & RTL8139_INT_RXFIFO_OVERFLOW)  { strcpy(str,"Rx FIFO Overflow");}
    if (val & RTL8139_INT_CABLE)            { strcpy(str,"Cable Length Change");}
    if (val & RTL8139_INT_TIMEOUT)          { strcpy(str,"Time Out");}
    if (val & RTL8139_INT_PCIERR)           { strcpy(str,"System Error");}
    textColor(0x03);
    printf("\n--------------------------------------------------------------------------------");
    textColor(0x0E);
    printf("\nRTL8139 Interrupt Status: %y, %s  ", val, str);
    textColor(0x03);

    // reset interrupts by writing 1 to the bits of offset 003Eh bis 003Fh, Interrupt Status Register
    *((uint16_t*)(BaseAddressRTL8139_MMIO + RTL8139_INTRSTATUS)) = 0xFFFF;

    uint32_t length = (network_buffer[network_bufferPointer+3] << 8) + network_buffer[network_bufferPointer+2]; // Little Endian

    // --------------------------- adapt buffer pointer ---------------------------------------------------
    
    // packets are 32 bit aligned
    network_bufferPointer += length + 4; 
    network_bufferPointer = (network_bufferPointer + 3) & ~0x3;

    // handle wrap-around 
    network_bufferPointer %= RTL8139_NETWORK_BUFFER_SIZE;

    // set read pointer
    *((uint16_t*)(BaseAddressRTL8139_MMIO + RTL8139_RXBUFTAIL)) = network_bufferPointer - 0x10;

    // --------------------------- adapt buffer pointer ---------------------------------------------------
    
    printf("RXBUFTAIL: %u", *((uint16_t*)(BaseAddressRTL8139_MMIO + RTL8139_RXBUFTAIL)));
    
    waitForKeyStroke();
    // sleepSeconds(1);
    
    uint32_t ethernetType = (network_buffer[network_bufferPointer+16] << 8) + network_buffer[network_bufferPointer+17]; // Big Endian

    // output receiving buffer
    textColor(0x0D);
    printf("\nFlags: ");
    textColor(0x03);
    for (uint8_t i = 0; i < 2; i++)
    {
        printf("%y ",network_buffer[network_bufferPointer+i]);
    }

    textColor(0x0D); printf("\tLength: ");
    textColor(0x03); printf("%d", length);

    textColor(0x0D); printf("\nMAC Receiver: "); textColor(0x03);
    for (uint8_t i = 4; i < 10; i++)
    {
        printf("%y ", network_buffer[network_bufferPointer+i]);
    }

    textColor(0x0D); printf("MAC Transmitter: "); textColor(0x03);
    for (uint8_t i = 10; i < 16; i++)
    {
        printf("%y ", network_buffer[network_bufferPointer+i]);
    }

    textColor(0x0D);
    printf("\nEthernet: ");
    textColor(0x03);
    if (ethernetType <= 0x05DC) // 0x05DC == 1500
    {
        printf("type 1, ");
    }
    else
    {
        printf("type 2, ");
    }

    textColor(0x0D);
    if (ethernetType <= 0x05DC)
    {
        printf("Length: ");
    }
    else
    {
        printf("Type: ");
    }
    textColor(0x03);
    for (uint8_t i = 16; i < 18; i++)
    {
        printf("%y ", network_buffer[network_bufferPointer+i]);
    }

    uint32_t printlength;
    if (length<=80)
    {
        printlength = length;
    }
    else
    {
        printlength = 80;
    }
    printf("\n");
    
    for (uint32_t i = 18; i <= printlength; i++)
    {
        printf("%y ", network_buffer[network_bufferPointer+i]);
    }
    textColor(0x0F);
    printf("\n");
    ipTcpStack_recv((void*)(&(network_buffer[network_bufferPointer+4])), length);
}


void install_RTL8139(pciDev_t* device)
{
    // pci bus data
    uint8_t bus  = device->bus;
    uint8_t dev  = device->device;
    uint8_t func = device->func;

    // prepare PCI command register // offset 0x04
    // bit 9 (0x0200): Fast Back-to-Back Enable // not necessary
    // bit 2 (0x0004): Bus Master               // cf. http://forum.osdev.org/viewtopic.php?f=1&t=20255&start=0
    uint16_t pciCommandRegister = pci_config_read(bus, dev, func, 0x0204);
    pci_config_write_dword(bus, dev, func, 0x04, pciCommandRegister /*already set*/ | 1<<2 /* bus master */); // resets status register, sets command register

    for (uint8_t j=0;j<6;++j) // check network card BARs
    {
        device->bar[j].memoryType = device->bar[j].baseAddress & 0x01;

        if (device->bar[j].baseAddress) // check valid BAR
        {
            if (device->bar[j].memoryType == 0)
            {
                BaseAddressRTL8139_MMIO = device->bar[j].baseAddress &= 0xFFFFFFF0;

            }
            if (device->bar[j].memoryType == 1)
            {
                BaseAddressRTL8139_IO = device->bar[j].baseAddress &= 0xFFFC;
            }
        }
    }

    /*
    http://wiki.osdev.org/RTL8139

    Turning on the RTL8139:
    Send 0x00 to the CONFIG_1 register (0x52) to set the LWAKE + LWPTN to active high.
    this should essentially "power on" the device.

    Software Reset:
    Sending 0x10 to the Command register (0x37) will send the RTL8139 into a software reset.
    Once that byte is sent, the RST bit must be checked to make sure that the chip has finished the reset.
    If the RST bit is 1, then the reset is still in operation.

    Init Receive Buffer:
    Send the chip a memory location to use as its receive buffer start location.
    One way to do it, would be to define a buffer variable
    and send that variables memory location to the RBSTART register (0x30).
    */

    memset(network_buffer, 0x0, RTL8139_NETWORK_BUFFER_SIZE); //clear receiving buffer
    kdebug(3, "RTL8139 MMIO: %X\n", BaseAddressRTL8139_MMIO);
    BaseAddressRTL8139_MMIO = (uint32_t) paging_acquire_pcimem(BaseAddressRTL8139_MMIO);
    printf("BaseAddressRTL8139_MMIO mapped to virtual address %X\n", BaseAddressRTL8139_MMIO);

    // "power on" the card
    *((uint8_t*)(BaseAddressRTL8139_MMIO + RTL8139_CONFIG1)) = 0x00;

    // carry out reset of network card: set bit 4 at offset 0x37 (1 Byte)
    *((uint8_t*)(BaseAddressRTL8139_MMIO + RTL8139_CHIPCMD)) = RTL8139_CMD_RESET;

    // wait for the reset of the "reset flag"
    uint32_t k=0;
    while (true)
    {
        sleepMilliSeconds(10);
        if (!(*((volatile uint8_t*)(BaseAddressRTL8139_MMIO + RTL8139_CHIPCMD)) & RTL8139_CMD_RESET))
        {
            kdebug(3, "\nwaiting successful (%d).\n", k);
            break;
        }
        k++;
        if (k > 100)
        {
            printf("\nWaiting not successful! Finished by timeout.\n");
            break;
        }
    }
    
    // now we set the RE and TE bits from the "Command Register" to Enable Reciving and Transmission
    // activate transmitter and receiver: Set bit 2 (TE) and 3 (RE) in control register 0x37 (1 byte).
    *((uint8_t*)(BaseAddressRTL8139_MMIO + RTL8139_CHIPCMD)) = RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE; 

    // set TCR (transmit configuration register, 0x40, 4 byte)
    *((uint32_t*)(BaseAddressRTL8139_MMIO + RTL8139_TXCONFIG)) = 0x03000700;       // TCR

    // set RCR (receive configuration register, RTL8139_RXCONFIG, 4 byte)
    /*
    bit4 AR  - Accept Runt
    bit3 AB  - Accept Broadcast:      Accept broadcast packets sent to mac FF:FF:FF:FF:FF:FF
    bit2 AM  - Accept Multicast:      Accept multicast packets.
    bit1 APM - Accept Physical Match: Accept packets send to NIC's MAC address.
    bit0 AAP - Accept All Packets
    */
    // bit7 is the WRAP bit, 0xF is AB+AM+APM+AAP
    // bit 12:11 defines the size of the Rx ring buffer length
    // 00: 8K + 16 byte 01: 16K + 16 byte 10: 32K + 16 byte 11: 64K + 16 byte

    *((uint32_t*)(BaseAddressRTL8139_MMIO + RTL8139_RXCONFIG)) = 0x0000071A; // 11100011010  // RCR 
    //*((uint32_t*)(BaseAddressRTL8139_MMIO + RTL8139_RXCONFIG)) = 0xF /* | (1<<7) */; // RCR

    // physical address of the receive buffer has to be written to RBSTART (0x30, 4 byte)
    *((uint32_t*)(BaseAddressRTL8139_MMIO + RTL8139_RXBUF)) = paging_get_phys_addr(kernel_pd, (void*)network_buffer);

    // set interrupt mask
    *((uint16_t*)(BaseAddressRTL8139_MMIO + RTL8139_INTRMASK)) = 0xFFFF; // all interrupts
    
    irq_installHandler(device->irq, rtl8139_handler);

    printf("\nnetwork card, mac-address: %y-%y-%y-%y-%y-%y\n", 
        *(uint8_t*)(BaseAddressRTL8139_MMIO + RTL8139_IDR0 + 0), *(uint8_t*)(BaseAddressRTL8139_MMIO + RTL8139_IDR0 + 1), 
        *(uint8_t*)(BaseAddressRTL8139_MMIO + RTL8139_IDR0 + 2), *(uint8_t*)(BaseAddressRTL8139_MMIO + RTL8139_IDR0 + 3), 
        *(uint8_t*)(BaseAddressRTL8139_MMIO + RTL8139_IDR0 + 4), *(uint8_t*)(BaseAddressRTL8139_MMIO + RTL8139_IDR0 + 5));
    
    waitForKeyStroke();
}

/*
The process of transmitting a packet with RTL8139:
1: copy the packet to a physically continuous buffer in memory.
2: Write the descriptor which is functioning
  (1). Fill in Start Address(physical address) of this buffer.
  (2). Fill in Transmit Status: the size of this packet, the early transmit threshold, Clear OWN bit in TSD (this starts the PCI operation).
3: As the number of data moved to FIFO meet early transmit threshold, the chip start to move data from FIFO to line..
4: When the whole packet is moved to FIFO, the OWN bit is set to 1.
5: When the whole packet is moved to line, the TOK(in TSD) is set to 1.
6: If TOK(IMR) is set to 1 and TOK(ISR) is set then a interrupt is triggered.
7: Interrupt service routine called, driver should clear TOK(ISR) State Diagram: (TOK,OWN)
*/
bool transferDataToTxBuffer(void* data, uint32_t length)
{
    memcpy((void*)Tx_network_buffer, data, length); // tx buffer
    printf("Physical Address of Tx Buffer = %X\n", paging_get_phys_addr(kernel_pd, (void*)network_buffer));

    // set address and size of the Tx buffer
    // reset OWN bit in TASD (REG_TRANSMIT_STATUS) starting transmit
    *((uint32_t*)(BaseAddressRTL8139_MMIO + RTL8139_TXADDR0   + 4 * curBuffer)) = paging_get_phys_addr(kernel_pd, (void*)Tx_network_buffer); 
    *((uint32_t*)(BaseAddressRTL8139_MMIO + RTL8139_TXSTATUS0 + 4 * curBuffer)) = length;     
    
    curBuffer++;
    curBuffer %= 4;
 
    printf("packet sent.\n");
    return true;
}

/*
int rtl8139_set_mac_address(struct net_device *dev, void *p)
{
	struct rtl8139_private *tp = netdev_priv(dev);
	void __iomem *ioaddr = tp->mmio_addr;
	struct sockaddr *addr = p;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

	spin_lock_irq(&tp->lock);

	RTL_W8_F(Cfg9346, Cfg9346_Unlock);
	RTL_W32_F(MAC0 + 0, cpu_to_le32 (*(u32 *) (dev->dev_addr + 0)));
	RTL_W32_F(MAC0 + 4, cpu_to_le32 (*(u32 *) (dev->dev_addr + 4)));
	RTL_W8_F(Cfg9346, Cfg9346_Lock);

	spin_unlock_irq(&tp->lock);

	return 0;
}
*/

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
