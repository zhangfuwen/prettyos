/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/


// http://www.rfc-editor.org/rfc/rfc793.txt <--- TRANSMISSION CONTROL PROTOCOL

#include "network/network.h"
#include "video/console.h"
#include "ethernet.h"
#include "arp.h"
#include "ipv4.h"
#include "icmp.h"
#include "tcp.h"
#include "udp.h"
#include "util.h"


void EthernetRecv(network_adapter_t* adapter, void* data, uint32_t length)
{
    ethernet_t* eth = (ethernet_t*)data;

    uint16_t ethernetType = (eth->type_len[0] << 8) + eth->type_len[1]; // Big Endian

    // output ethernet packet

    textColor(0x0D); printf("\tLength: ");
    textColor(0x03); printf("%d", length);

    textColor(0x0D); printf("\nMAC Receiver: "); textColor(0x03);
    for (uint8_t i = 0; i < 6; i++)
    {
        printf("%y ", eth->recv_mac[i]);
    }

    textColor(0x0D); printf("MAC Transmitter: "); textColor(0x03);
    for (uint8_t i = 0; i < 6; i++)
    {
        printf("%y ", eth->send_mac[i]);
    }

    textColor(0x0D);
    printf("\nEthernet: ");

    textColor(0x03);
    if (ethernetType <= 1500) { printf("type 1, "); }
    else                      { printf("type 2, "); }

    textColor(0x0D);
    if (ethernetType <= 1500) { printf("Length: "); }
    else                      { printf("Type: ");   }

    textColor(0x03);
    for (uint8_t i = 0; i < 2; i++)
    {
        printf("%y ", eth->type_len[i]);
    }

    uint32_t printlength = max(length, 80);
    printf("\n");

    for (uint32_t i = sizeof(ethernet_t); i <= printlength; i++)
    {
        printf("%y ", ((uint8_t*)data)[i]);
    }
    textColor(0x0F);
    printf("\n");


    void* udpData; // TODO like tcppacket    

    textColor(0x0E);
    if (((eth->type_len[0] << 8) | eth->type_len[1]) > 1500)
    {
        printf("Ethernet 2. ");
        // cf. http://en.wikipedia.org/wiki/EtherType
        // and http://www.cavebear.com/archive/cavebear/Ethernet/type.html

        // now we look for IPv4, IPv6, or ARP
        if ((eth->type_len[0] == 0x08) && (eth->type_len[1] == 0x00)) // IP
        {
            printf("Ethernet type: IP. ");
            ip_t*  ip  = (ip_t*) ((uintptr_t)eth + sizeof(ethernet_t));

            // IPv4 protocol is parsed here and distributed in switch/case
            uint32_t ipHeaderLengthBytes = 4 * ip->ipHeaderLength; // is given as number of 32 bit pieces (4 byte)
            printf(" IP version: %u, IP Header Length: %u byte", ip->version, ipHeaderLengthBytes);
            switch(ip->protocol)
            {
                case 1: // icmp
                    printf("ICMP. ");
                    ICMPAnswerPing(adapter, data, length);
                    icmpDebug(data, length);
                    break;
                case 4: // ipv4
                    printf("IPv4. ");
                    /*
                    tcpheader_t tcp;
                    tcp.sourcePort = 1025;
                    tcp.destinationPort = 80;
                    tcp.sequence_number = 1;
                    tcp.ACK = 1;
                    tcpDebug(&tcp);
                    */
                    break;
                case 6: // tcp
                    printf("\nTCP. ");
                    tcppacket_t* packet = data;                    
                    tcpDebug(&(packet->tcp));
                    break;
                case 17: // udp
                    printf("UDP. ");
                    udpData = (void*)((uintptr_t)data + sizeof(ethernet_t) + ipHeaderLengthBytes);
                    UDPRecv(udpData, length - ipHeaderLengthBytes, *(uint32_t*)ip->sourceIP, *(uint32_t*)ip->destIP);
                    break;
                case 41: // ipv6 ?? cf. below
                    printf("IPv6. ");
                    break;
                default:
                    printf("other protocol based on IP. ");
                    break;
            }
        } // end of IPv6

        else if ((eth->type_len[0] == 0x86) && (eth->type_len[1] == 0xDD)) // IPv6
        {
            printf("Ethernet type: IPv6. Currently, not further analyzed. ");
            // TODO analyze IPv6 
        }

        else if ((eth->type_len[0] == 0x08) && (eth->type_len[1] == 0x06)) // ARP
        {
            printf("Ethernet type: ARP. ");
            arp_received(adapter, (void*)eth);
        }
        else
        {
            printf("Neither IP nor ARP\n");
            // TODO
        }
    } // end of ethernet 2
    else
    {
        printf("Ethernet 1. ");
    }

    printf("\n");
    textColor(0x0F);
}


bool EthernetSend(network_adapter_t* adapter, void* data, uint32_t length)
{
    if (length > 0x700)
    {
        printf("\nerror: EthernetSend: length: %u. This is more than (1792) 0x700",length);
        return false;
    }
    else
    {
        printf("\nEthernetSend: length: %u.",length);
    }

    // TODO: check whether Tx buffer is already occupied

    ethernet_t* eth = (ethernet_t*)data;
    textColor(0x0C);

    if (((eth->type_len[0] << 8) | eth->type_len[1]) > 1500)
    {
        printf("\nPacket now sent with Ethernet 2. ");
    }
    else
    {
        printf("\nPacket now sent with Ethernet 1. ");
    }
    textColor(0x0F);

    return network_sendPacket(adapter, data, length);
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
