/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "rtl8139.h"
#include "util.h"
#include "timer.h"
#include "paging.h"
#include "video/console.h"
#include "kheap.h"


#define RTL8139_RX_BUFFER_SIZE 0x2000 // 8 KiB
#define RTL8139_TX_BUFFER_SIZE 4096


static RTL8139_networkAdapter_t* device;


void rtl8139_handler(registers_t* data)
{
    #ifdef _NETWORK_DIAGNOSIS_
    textColor(0x03);
    printf("\n--------------------------------------------------------------------------------");
    #endif

    // read bytes 003Eh bis 003Fh, Interrupt Status Register
    volatile uint16_t val = *((uint16_t*)(device->device->MMIO_base + RTL8139_INTRSTATUS));
    #ifdef _NETWORK_DIAGNOSIS_
    textColor(0x0E);
    printf("\nRTL8139 Interrupt Status: %yh, ", val);
    textColor(0x03);
    if      (val & RTL8139_INT_RX_OK)           { puts("Receive OK"); }
    else if (val & RTL8139_INT_RX_ERR)          { puts("Receive Error"); }
    else if (val & RTL8139_INT_TX_OK)           { puts("Transmit OK"); }
    else if (val & RTL8139_INT_TX_ERR)          { puts("Transmit Error"); }
    else if (val & RTL8139_INT_RXBUF_OVERFLOW)  { puts("Rx Buffer Overflow");}
    else if (val & RTL8139_INT_RXFIFO_UNDERRUN) { puts("Packet Underrun / Link change");}
    else if (val & RTL8139_INT_RXFIFO_OVERFLOW) { puts("Rx FIFO Overflow");}
    else if (val & RTL8139_INT_CABLE)           { puts("Cable Length Change");}
    else if (val & RTL8139_INT_TIMEOUT)         { puts("Time Out");}
    else if (val & RTL8139_INT_PCIERR)          { puts("System Error");}
    #endif

    // reset interrupts by writing 1 to the bits of offset 003Eh to 003Fh, Interrupt Status Register
    *((uint16_t*)(device->device->MMIO_base + RTL8139_INTRSTATUS)) = val;

    if (!(val & RTL8139_INT_RX_OK))
    {
        return;
    }

    uint32_t length = (device->RxBuffer[device->RxBufferPointer+3] << 8) + device->RxBuffer[device->RxBufferPointer+2]; // Little Endian

    // Display RTL8139 specific data
    #ifdef _NETWORK_DATA_
    textColor(0x0D);
    printf("\nFlags: ");
    textColor(0x03);
    for (uint8_t i = 0; i < 2; i++)
    {
        printf("%yh ", device->RxBuffer[device->RxBufferPointer+i]);
    }
    #endif

    // Inform network interface about the packet
    network_receivedPacket(device->device, &device->RxBuffer[device->RxBufferPointer]+4, length);

    // Increase RxBufferPointer
    // packets are DWORD aligned
    device->RxBufferPointer += length + 4;
    device->RxBufferPointer = (device->RxBufferPointer + 3) & ~0x3; // ~0x3 = 0xFFFFFFFC

    // handle wrap-around
    device->RxBufferPointer %= RTL8139_RX_BUFFER_SIZE;

    // set read pointer
    *((uint16_t*)(device->device->MMIO_base + RTL8139_RXBUFTAIL)) = device->RxBufferPointer - 0x10; // 0x10 = 16

    #ifdef _NETWORK_DIAGNOSIS_
    printf("RXBUFTAIL: %u", *((uint16_t*)(device->device->MMIO_base + RTL8139_RXBUFTAIL)));
    #endif
}


void install_RTL8139(network_adapter_t* dev)
{
    device = malloc(sizeof(RTL8139_networkAdapter_t), 0, "RTL8139");
    device->device = dev;
    dev->data = device;

    device->RxBuffer = malloc(RTL8139_RX_BUFFER_SIZE, 4, "RTL8139-RxBuf");
    device->RxBufferPointer = 0;
    memset(device->RxBuffer, 0, RTL8139_RX_BUFFER_SIZE); // clear receiving buffer

    device->TxBuffer = malloc(RTL8139_TX_BUFFER_SIZE, 4, "RTL8139-TxBuf");
    device->TxBufferIndex = 0;

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

    kdebug(3, "\nRTL8139 MMIO: %Xh", dev->MMIO_base);
    dev->MMIO_base = paging_acquirePciMemory((uint32_t)dev->MMIO_base, 1);
    printf("\nMMIO base mapped to virt. addr. %Xh", dev->MMIO_base);

    // "power on" the card
    *((uint8_t*)(dev->MMIO_base + RTL8139_CONFIG1)) = 0x00;

    // carry out reset of network card: set bit 4 at offset 0x37 (1 Byte)
    *((uint8_t*)(dev->MMIO_base + RTL8139_CHIPCMD)) = RTL8139_CMD_RESET;

    // wait for the reset of the "reset flag"
    uint32_t k=0;
    while (true)
    {
        delay(10000);
        if (!(*((volatile uint8_t*)(dev->MMIO_base + RTL8139_CHIPCMD)) & RTL8139_CMD_RESET))
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

    // now we set the RE and TE bits from the "Command Register" to Enable Receiving and Transmission
    // activate transmitter and receiver: Set bit 2 (TE) and 3 (RE) in control register 0x37 (1 byte).
    *((uint8_t*)(dev->MMIO_base + RTL8139_CHIPCMD)) = RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE;

    // set TCR (transmit configuration register, 0x40, 4 byte)
    *((uint32_t*)(dev->MMIO_base + RTL8139_TXCONFIG)) = 0x03000700;       // TCR

    // set RCR (receive configuration register, RTL8139_RXCONFIG, 4 byte)
    /*
    bit4 AR  - Accept Runt
    bit3 AB  - Accept Broadcast:      Accept broadcast packets sent to mac FF:FF:FF:FF:FF:FF
    bit2 AM  - Accept Multicast:      Accept multicast packets.
    bit1 APM - Accept Physical Match: Accept packets send to NIC's MAC address.
    bit0 AAP - Accept All Packets
    */

    // bit 12:11 defines the size of the Rx ring buffer length
    // 00b:  8K + 16 byte       01b: 16K + 16 byte
    // 10b: 32K + 16 byte       11b: 64K + 16 byte

    *((uint32_t*)(dev->MMIO_base + RTL8139_RXCONFIG)) = 0x0000071A; // 11100011010  // RCR

    // physical address of the receive buffer has to be written to RBSTART (0x30, 4 byte)
    *((uint32_t*)(dev->MMIO_base + RTL8139_RXBUF)) = paging_getPhysAddr((void*)device->RxBuffer);

    // set interrupt mask
    *((uint16_t*)(dev->MMIO_base + RTL8139_INTRMASK)) = 0xFFFF; // all interrupts

    for (uint8_t i = 0; i < 6; i++)
    {
        dev->MAC_address[i] =  *(uint8_t*)(dev->MMIO_base + RTL8139_IDR0 + i);
    }
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
bool rtl8139_send(network_adapter_t* adapter, uint8_t* data, size_t length)
{
    RTL8139_networkAdapter_t* rAdapter = adapter->data;

    memcpy(rAdapter->TxBuffer, data, length); // tx buffer
    if(length < 60) // Fill buffer to a minimal length of 60
    {
        memset(device->TxBuffer+length, 0, 60-length);
        length = 60;
    }
    printf("\n\n>>> Transmission starts <<<\nPhysical Address of Tx Buffer = %Xh\n", paging_getPhysAddr(rAdapter->TxBuffer));

    // set address and size of the Tx buffer
    // reset OWN bit in TASD (REG_TRANSMIT_STATUS) starting transmit
    // set transmit FIFO threshhold to 48*32 = 1536 bytes to avoid tx underrun
    *((uint32_t*)(adapter->MMIO_base + RTL8139_TXADDR0   + 4 * rAdapter->TxBufferIndex)) = paging_getPhysAddr(rAdapter->TxBuffer);
    *((uint32_t*)(adapter->MMIO_base + RTL8139_TXSTATUS0 + 4 * rAdapter->TxBufferIndex)) = length | (48 << 16);

    rAdapter->TxBufferIndex++;
    rAdapter->TxBufferIndex %= 4;

    printf("packet sent.\n");
    return true;
}

/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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
