/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// http://www.rfc-editor.org/rfc/rfc768.txt <---  User Datagram Protocol (UDP)

#include "types.h"
#include "video/console.h"
#include "kheap.h"
#include "util.h"
#include "ipv4.h"
#include "udp.h"
#include "dhcp.h"

void UDPRecv(network_adapter_t* adapter, udpPacket_t* packet, uint32_t length)
{
    // TODO: ...
    UDPDebug(adapter, packet);
}

void UDPSend(network_adapter_t* adapter, void* data, uint32_t length, uint16_t srcPort, uint8_t srcIP[4], uint16_t destPort, uint8_t destIP[4])
{
    udpPacket_t* packet = malloc(sizeof(udpPacket_t)+length, 0, "UDP packet");
    memcpy(packet+1, data, length);

    packet->sourcePort  = htons(srcPort);
    packet->destPort    = htons(destPort);
    packet->length      = htons(length + sizeof(udpPacket_t));
    packet->checksum    = 0; // 0 necessary for successful DHCP Process
    // packet->checksum = 0; // for checksum calculation
    // packet->checksum = htons(udptcpCalculateChecksum((void*)packet, length + sizeof(udpPacket_t), srcIP, destIP, 17));

    ipv4_send(adapter, packet, length + sizeof(udpPacket_t), destIP, 17);
    free(packet);
}

void UDPDebug(network_adapter_t* adapter, udpPacket_t* udp)
{
    printf("\n");
    printf("UDP Header information:\n");
    textColor(0x0E);
    printf("+--------------+----------------+\n");
    printf("|      %u      |      %u      | (source port, destination port)\n", ntohs(udp->sourcePort), ntohs(udp->destPort));
    printf("+-------------------------------+\n");
    printf("|      %u      |      %xh    | (length, checksum)\n", ntohs(udp->length), ntohs(udp->checksum));
    printf("+-------------------------------+\n");

    // http://www.iana.org/assignments/port-numbers
    // http://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers
    switch (ntohs(udp->destPort))
    {
        printf("Dest. Port: ");
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
        printf("DHCPv4 Server\n");
        break;
    case 68:
        printf("DHCPv4 Client\n");
        DHCP_AnalyzeServerMessage(adapter, (dhcp_t*)(udp+1));
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
    default:
        printf("%u\n", ntohs(udp->destPort));
        break;
    }
}


/*
* Copyright (c) 2010-2011 The PrettyOS Project. All rights reserved.
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
