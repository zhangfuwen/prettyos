/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/

#include "pcnet.h"
#include "util.h"
#include "irq.h"
#include "timer.h"
#include "kheap.h"
#include "paging.h"
#include "video/console.h"


// RAP values to use register
#define BCR20 20

// Offsets to 16 bit IO-Ports
#define APROM0 0x00
#define APROM2 0x02
#define APROM4 0x04
#define RDP    0x10
#define RAP    0x12
#define RESET  0x14
#define BDP    0x16


static PCNet_card* device;


static void writeBCR(PCNet_card* dev, uint16_t value)
{
    outportw(dev->device->IO_base+RAP, BCR20); // Enable BCR20 register
    outportw(dev->device->IO_base+BDP, value); // Write value to BCR20 register
}
static void writeCSR(PCNet_card* dev, uint8_t csr, uint16_t value)
{
    outportw(dev->device->IO_base+RAP, csr);   // Enable CSR register
    outportw(dev->device->IO_base+RDP, value); // Write value to CSR register
}
static uint16_t readCSR(PCNet_card* dev, uint8_t csr)
{
    outportw(dev->device->IO_base+RAP, csr);   // Enable CSR register
    return inportw(dev->device->IO_base+RDP);  // Read value from CSR register
}

void install_AMDPCnet(network_adapter_t* dev)
{
    textColor(0x0E);
    printf("\nInstalling AMD PCNet Fast III:");
    textColor(0x0F);

    device = malloc(sizeof(PCNet_card), 16, "PCNet_card");
    device->initialized = false;
    device->device = dev;
    dev->data = device;

    printf("\nIO: %x", dev->IO_base);

    // Get MAC
    uint16_t temp = inportw(dev->IO_base + APROM0);
    dev->MAC_address[0] = temp;
    dev->MAC_address[1] = temp>>8;
    temp = inportw(dev->IO_base + APROM2);
    dev->MAC_address[2] = temp;
    dev->MAC_address[3] = temp>>8;
    temp = inportw(dev->IO_base + APROM4);
    dev->MAC_address[4] = temp;
    dev->MAC_address[5] = temp>>8;
    printf("\t\tMAC-address: %y-%y-%y-%y-%y-%y", dev->MAC_address[0], dev->MAC_address[1], dev->MAC_address[2],
                                                 dev->MAC_address[3], dev->MAC_address[4], dev->MAC_address[5]);

    // Reset
    inportw(dev->IO_base+RESET);
    outportw(dev->IO_base+RESET, 0); // Needed for NE2100LANCE adapters
    sleepMilliSeconds(10);
    writeBCR(device, 0x0102); // Enable 32-bit mode

    // Stop
    writeCSR(device, 0, 0x04); // STOP-Reset

    // Setup descriptors, Init send and receive buffers
    device->currentRecDesc = 0;
    device->currentTransDesc = 0;
    device->receiveDesc = malloc(8*sizeof(PCNet_descriptor), 16, "PCNet: RecDesc");//malloc(2048*2, 16, "PCNet descriptors");
    device->transmitDesc = malloc(8*sizeof(PCNet_descriptor), 16, "PCNet: TransDesc");//(void*)(((uintptr_t)device->receiveDesc)+2048);
    for(uint8_t i = 0; i < 8; i++)
    {
        void* buffer = malloc(2048, 16, "PCnet receive buffer");
        device->receiveBuf[i] = buffer;
        device->receiveDesc[i].address = paging_getPhysAddr(buffer);
        device->receiveDesc[i].flags = 0x80000000 | 0x7FF | 0x0000F000; // Descriptor OWN | Buffer length | ?
        device->receiveDesc[i].flags2 = 0;
        device->receiveDesc[i].avail = (uint32_t)buffer;

        buffer = malloc(2048, 16, "PCnet tramsmit buffer");
        device->transmitBuf[i] = buffer;
        device->transmitDesc[i].address = paging_getPhysAddr(buffer);
        device->transmitDesc[i].flags = 0;//0x0000F000;
        device->transmitDesc[i].flags2 = 0;
        device->transmitDesc[i].avail = (uint32_t)buffer;
    }

    // Fill and register initialization block
    PCNet_initBlock* initBlock = malloc(sizeof(PCNet_initBlock), 16, "PCNet init block");
    memset(initBlock, 0, sizeof(PCNet_initBlock));
    initBlock->mode = 0x8000; // Promiscuous mode
    initBlock->receive_length = 3;
    initBlock->transfer_length = 3;
    initBlock->physical_address = *(uint64_t*)dev->MAC_address;
    initBlock->receive_descriptor = paging_getPhysAddr(device->receiveDesc);
    initBlock->transmit_descriptor = paging_getPhysAddr(device->transmitDesc);
    uintptr_t phys_address = (uintptr_t)paging_getPhysAddr(initBlock);
    writeCSR(device, 1, phys_address); // Lower bits of initBlock address
    writeCSR(device, 2, phys_address>>16); // Higher bits of initBlock address

    // Init card
    writeCSR(device, 0, 0x0041); // Initialize card, activate interrupts
    if(!waitForIRQ(device->device->PCIdev->irq, 1000))
        printf("\nIRQ did not occur.\n");
    writeCSR(device, 4, 0x0C00 | readCSR(device, 4));

    // Activate card
    writeCSR(device, 0, 0x0042);
}

static void PCNet_receive()
{
    textColor(0x0E);
    printf("\nPCNet: received packet");
    textColor(0x0F);

    size_t size;
    while ((device->receiveDesc[device->currentRecDesc].flags & 0x80000000) == 0)
    {
        if (!(device->receiveDesc[device->currentRecDesc].flags & 0x40000000) &&
            (device->receiveDesc[device->currentRecDesc].flags & 0x03000000) == 0x03000000)
        {
            size = device->receiveDesc[device->currentRecDesc].flags2 & 0xFFFF;
            if (size > 64)
                size -= 4; // Do not copy CRC32

            network_receivedPacket(device->device, (void*)device->receiveDesc[device->currentRecDesc].avail, size);
        }
        device->receiveDesc[device->currentRecDesc].flags = 0x8000F7FF; // Set OWN-Bit and default values
        device->receiveDesc[device->currentRecDesc].flags2 = 0;

        device->currentRecDesc++; // Go to next descriptor
        if (device->currentRecDesc == 8)
            device->currentRecDesc = 0;
    }
}

bool PCNet_send(network_adapter_t* adapter, uint8_t* data, size_t length)
{
    printf("\nPCNET SEND PACKET");
    PCNet_card* pcnet = adapter->data;

    //Achtung, Beginn eines kritischen Abschnitts!
    cli();
    if (pcnet->currentTransDesc > 7)
        pcnet->currentTransDesc = 0;
    uint8_t current_descriptor = pcnet->currentTransDesc++;
    if (current_descriptor == 7) // pcnet->currentTransDesc ist also 8
        pcnet->currentTransDesc = 0;
    sti();
    //Ende des Abschnitts

    if (length > 1518) // Maximale L�nge eines Ethernetframes
        length = 1518;

    //ncard->linear_transmit_buffer enth�lt die lineare Adresse des Puffers
    memcpy(pcnet->transmitBuf[current_descriptor], data, length);
    //Die physische Adresse (also ncard->physical_transmit_buffer[current_descriptor]) sollte schon bei der
    //Initialisierung in das �address�-Feld geschrieben worden sein
    pcnet->transmitDesc[current_descriptor].flags2 = 0;
    pcnet->transmitDesc[current_descriptor].flags = 0x8030F000 | ((-length) & 0x7FF);
    writeCSR(pcnet, 0, 0x48);

    return(true);
}

void PCNet_handler(registers_t* data)
{
    uint16_t csr0 = readCSR(device, 0);
    if(device->initialized == false)
    {
        device->initialized = true;
        printf("\nPCNet: initialized.\n");
    }
    else
    {
        textColor(0x03);
        printf("\n--------------------------------------------------------------------------------");
        printf("\n--------------------------------------------------------------------------------");
        textColor(0x0C);

        if(csr0 & 0x8000) // Error
        {
            if(csr0 & 0x2000)
                printf("\nPCNet: Collision error");
            else if(csr0 & 0x1000)
                printf("\nPCNet: Missed frame error");
            else if(csr0 & 0x800)
                printf("\nPCNet: Memory error");
            else
                printf("\nPCNet: Undefined error.");
        }
        else if(csr0 & 0x00000200)
            printf("\nPCNet: transmit descriptor finished");
        else if(csr0 & 0x00000400)
            PCNet_receive();
    }
    writeCSR(device, 0, csr0);
}


/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
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