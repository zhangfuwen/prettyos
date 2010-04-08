/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "pci.h"
#include "paging.h"
#include "ipTcpStack.h"

extern pciDev_t pciDev_Array[50];

extern uint8_t network_buffer[8192+16]; /// OK?
extern uint32_t BaseAddressRTL8139_IO;
extern uint32_t BaseAddressRTL8139_MMIO;

void rtl8139_handler(registers_t* r)
{
    /// TODO: ring buffer, we get always the first received data!

    // read bytes 003Eh bis 003Fh, Interrupt Status Register
    uint16_t val = *((uint16_t*)(BaseAddressRTL8139_MMIO + 0x3E));

    char str[80];
    strcpy(str,"");

    if ((val & 0x1) == 0x1)
    {
        strcpy(str,"Receive OK,");
    }
    if ((val & 0x4) == 0x4)
    {
        strcpy(str,"Transfer OK,");
    }
    settextcolor(3,0);
    printf("\n--------------------------------------------------------------------------------");
    settextcolor(14,0);
    printf("\nRTL8139 IRQ: %y, %s  ", val,str);
    settextcolor(3,0);

    // reset interrupts by writing 1 to the bits of offset 003Eh bis 003Fh, Interrupt Status Register
    *((uint16_t*)(BaseAddressRTL8139_MMIO + 0x3E)) = val;

    strcat(str,"   Receiving Buffer content:\n");
    printf(str);

    int32_t length, ethernetType;
    length = network_buffer[3]*0x100 + network_buffer[2]; // Little Endian
    if (length>300) length = 300;
    ethernetType = network_buffer[16]*0x100 + network_buffer[17]; // Big Endian

    // output receiving buffer
    settextcolor(13,0);
    printf("Flags: ");
    settextcolor(3,0);
    for (int8_t i=0;i<2;i++) {printf("%y ",network_buffer[i]);}

    // settextcolor(13,0); printf("\tLength: "); settextcolor(3,0);
    // for (i=2;i<4;i++) {printf("%y ",network_buffer[i]);}
    uint32_t paket_length = (network_buffer[3] << 8) | network_buffer[2];

    settextcolor(13,0);
    printf("\tLength: ");    settextcolor(3,0);    printf("%d", paket_length);

    settextcolor(13,0); printf("\nMAC Receiver: "); settextcolor(3,0);
    for (int8_t i=4;i<10;i++) {printf("%y ",network_buffer[i]);}

    settextcolor(13,0); printf("MAC Transmitter: "); settextcolor(3,0);
    for (int8_t i=10;i<16;i++) {printf("%y ",network_buffer[i]);}

    settextcolor(13,0);
    printf("\nEthernet: ");
    settextcolor(3,0);
    if (ethernetType<=0x05DC){  printf("type 1, "); }
    else                    {  printf("type 2, "); }

    settextcolor(13,0);
    if (ethernetType<=0x05DC){  printf("Length: "); }
    else                    {  printf("Type: ");   }
    settextcolor(3,0);
    for (int8_t i=16;i<18;i++) {printf("%y ",network_buffer[i]);}

    printf("\n");
    for (int32_t i=18;i<=length;i++) {printf("%y ",network_buffer[i]);}
    printf("\n--------------------------------------------------------------------------------\n");

    settextcolor(15,0);

    // call to the IP-TCP Stack
    ipTcpStack_recv((void*)(&(network_buffer[4])), paket_length);
}


void install_RTL8139(uint32_t number)
{
    #ifdef _DIAGNOSIS_
        settextcolor(3,0);
        printf("RTL8139 MMIO: %X\n",BaseAddressRTL8139_MMIO);
        settextcolor(15,0);
    #endif

    /// idendity mapping of BaseAddressRTL8139_MMIO
    bool USE_VIRTUAL_APPROACH = true;
    if (USE_VIRTUAL_APPROACH)
    {
        BaseAddressRTL8139_MMIO = (uint32_t) paging_acquire_pcimem(BaseAddressRTL8139_MMIO);
        printf("BaseAddressRTL8139_MMIO mapped to virtual address %X\n", BaseAddressRTL8139_MMIO);
    }
    else
    {
        int retVal = paging_do_idmapping(BaseAddressRTL8139_MMIO);
        if (retVal == true)
        {
            printf("\n");
        }
        else
        {
            printf("\npaging_do_idmapping(...) error.\n");
        }
    }

    // "power on" the card
    *((uint8_t*)(BaseAddressRTL8139_MMIO + 0x52)) = 0x00;

    // carry out reset of network card: set bit 4 at offset 0x37 (1 Byte)
    *((uint8_t*)(BaseAddressRTL8139_MMIO + 0x37)) = 0x10;

    // wait for the reset of the "reset flag"
    uint32_t k=0;
    while (true)
    {
        sleepMilliSeconds(10);
        if (!(*((volatile uint8_t*)(BaseAddressRTL8139_MMIO + 0x37)) & 0x10)) //
        {
            #ifdef _DIAGNOSIS_
                settextcolor(3,0);
                printf("\nwaiting successful(%d).\n", k);
                settextcolor(15,0);
            #endif
            break;
        }
        k++;
        if (k>100)
        {
            printf("\nWaiting not successful! Finished by timeout.\n");
            break;
        }
    }
    #ifdef _DIAGNOSIS_
        settextcolor(3,0);
        printf("mac address: %y-%y-%y-%y-%y-%y\n",
                *((uint8_t*)(BaseAddressRTL8139_MMIO)+0), *((uint8_t*)(BaseAddressRTL8139_MMIO)+1),
                *((uint8_t*)(BaseAddressRTL8139_MMIO)+2), *((uint8_t*)(BaseAddressRTL8139_MMIO)+3),
                *((uint8_t*)(BaseAddressRTL8139_MMIO)+4), *((uint8_t*)(BaseAddressRTL8139_MMIO)+5));
        settextcolor(15,0);
    #endif

    // now we set the RE and TE bits from the "Command Register" to Enable Reciving and Transmission
    /*
    Aktivieren des Transmitters und des Receivers: Setze Bits 2 und 3 (TE bzw. RE) im Befehlsregister (0x37, 1 Byte).
    Dies darf angeblich nicht erst später geschehen, da die folgenden Befehle ansonsten ignoriert würden.
    */
    *((uint8_t*)(BaseAddressRTL8139_MMIO + 0x37)) = 0x0C; // 1100b

    /*
    TCR (Transmit Configuration Register, 0x40, 4 Bytes) und RCR (Receive Configuration Register, 0x44, 4 Bytes) setzen.
    An dieser Stelle nicht weiter kommentierter Vorschlag: TCR = 0x03000700, RCR = 0x0000070a
    */
    *((uint32_t*)(BaseAddressRTL8139_MMIO + 0x40)) = 0x03000700; //TCR
    *((uint32_t*)(BaseAddressRTL8139_MMIO + 0x44)) = 0x0000070a; //RCR
    //*((uint32_t*)(BaseAddressRTL8139_MMIO + 0x44)) = 0xF;        //RCR pci.c in rev. 108 ??
    /*0xF means AB+AM+APM+AAP*/

    // first 65536 bytes are our sending buffer and the last bytes are our receiving buffer
    // set RBSTART to our buffer for the Receive Buffer
    /*
    Puffer für den Empfang (evtl auch zum Senden, das kann aber auch später passieren) initialisieren.
    Wir brauchen bei den vorgeschlagenen Werten 8K + 16 Bytes für den Empfangspuffer und einen ausreichend großen Sendepuffer.
    Was ausreichend bedeutet, ist dabei davon abhängig, welche Menge wir auf einmal absenden wollen.
    Anschließend muss die physische(!) Adresse des Empfangspuffers nach RBSTART (0x30, 4 Bytes) geschrieben werden.
    */
    *((uint32_t*)(BaseAddressRTL8139_MMIO + 0x30)) = (uint32_t)network_buffer /* + 8192+16 */ ;

    // Sets the TOK (interrupt if tx ok) and ROK (interrupt if rx ok) bits high
    // this allows us to get an interrupt if something happens...
    /*
    Interruptmaske setzen (0x3C, 2 Bytes). In diesem Register können die Ereignisse ausgewählt werden,
    die einen IRQ auslösen sollen. Wir nehmen der Einfachkeit halber alle und setzen 0xffff.
    */
    *((uint16_t*)(BaseAddressRTL8139_MMIO + 0x3C)) = 0xFF; // all interrupts
    //*((uint16_t*)(BaseAddressRTL8139_MMIO + 0x3C)) = 0x5; // only TOK and ROK

    irq_install_handler(32 + pciDev_Array[number].irq, rtl8139_handler);
}


/*
* Copyright (c) 2009 The PrettyOS Project. All rights reserved.
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
