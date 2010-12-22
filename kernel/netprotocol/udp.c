/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// http://www.rfc-editor.org/rfc/rfc768.txt <---  User Datagram Protocol

#include "video/console.h"
#include "udp.h"
#include "ethernet.h"

void UDPRecv(void* data, uint32_t length, uint32_t sourceIP, uint32_t destIP)
{
    // sourceIP is big endian!

    // destIP is big endian!

    // udpheader_t* udp = (udpheader_t*)data;
    // printf("\nsource port: %u dest.  port: %u\n", ntohs(udp->sourcePort), ntohs(udp->destPort));

    // TODO: evaluate UDP data

    // uint32_t udpDataLength = ipLength - sizeof(ip_t) - sizeof(udpheader_t);
    // uint8_t pkt[sizeof(udpheader_t) + udpDataLength];

    UDPDebug(data, length);
}

void UDPSend(void* data, uint32_t length)
{
    // transferDataToTxBuffer(void* data, uint32_t length);
}

void UDPDebug(void* data, uint32_t length)
{
    udpheader_t* udp = (udpheader_t*)data;

    printf("\n");
    printf("UDP Header information:\n");
    textColor(0x0E);
    printf("+--------------+----------------+\n");
    printf("|      %u      |      %u      | (source port, destination port)\n", ntohs(udp->sourcePort), ntohs(udp->destPort));
    printf("+-------------------------------+\n");
    printf("|      %u      |      %x    | (length, checksum)\n", ntohs(udp->length), ntohs(udp->checksum));
    printf("+-------------------------------+\n");

    // http://www.iana.org/assignments/port-numbers
    // http://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers
    switch(ntohs(udp->destPort))
    {
    case 20:
        printf("FTP - data transfer\n");
        break;
    case 21:
        printf("FTP - control (command)\n");
        break;
    case 22:
        printf("Secure Shell (SSH)\n");
        break;
    case 53:
        printf("Domain Name System (DNS)\n");
        break;
    case 67:
        printf("UDP Bootstrap Protocol (BOOTP) Server, also used by DHCP Server\n");
        break;
    case 68:
        printf("UDP Bootstrap Protocol (BOOTP) Client, also used by DHCP Client\n");
        break;
    case 80:
        printf("HTTP\n");
        break;
    case 137:
        printf("NetBIOS Name Service\n");
        break;
    case 138:
        printf("NetBIOS Datagram Service\n");
        break;
    case 139:
        printf("NetBIOS Session Service\n");
        break;
    case 143:
        printf("Internet Message Access Protocol (IMAP)\n");
        break;
    case 546:
        printf("DHCPv6 Client\n");
        break;
    case 547:
        printf("DHCPv6 Server\n");
        break;
    case 1257:
        printf("shockwave2\n");
        break;
    case 1900:
        printf("Simple Service Discovery Protocol (ssdp)\n");
        break;
    case 3544:
        printf("Teredo Tunneling\n");
        break;
    case 5355:
        printf("Link-Local Multicast Name Resolution (llmnr)\n");
        break;
    }
}

/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
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
