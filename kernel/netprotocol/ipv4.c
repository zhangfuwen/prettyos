/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "ipv4.h"
#include "tcp.h"
#include "udp.h"
#include "icmp.h"
#include "ethernet.h"
#include "video/console.h"
#include "kheap.h"
#include "util.h"
#include "arp.h"


static const uint8_t broadcast_IP[4] = {0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t broadcast_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void ipv4_received(struct network_adapter* adapter, ipv4Packet_t* packet, uint32_t length)
{
    if(memcmp(packet->destIP, adapter->IP_address, 4) != 0 && memcmp(packet->destIP, broadcast_IP, 4) != 0)
    {
        printf("\nIPv4 packet received. We are not the addressee.");
        return;
    }

    // IPv4 protocol is parsed here and distributed in switch/case
    uint32_t ipHeaderLengthBytes = 4 * packet->ipHeaderLength; // is given as number of 32 bit pieces (4 byte)
    printf(" IP version: %u, IP Header Length: %u byte", packet->version, ipHeaderLengthBytes);
    switch(packet->protocol)
    {
        case 1: // icmp
            printf("\nICMP. ");
            ICMPAnswerPing(adapter, (void*)(packet+1), length-sizeof(ipv4Packet_t), packet->sourceIP);
            break;
        case 4: // ipv4
            printf("\nIPv4. ");
            break;
        case 6: // tcp
            printf("\nTCP. ");
            tcpPacket_t* tcpPacket = (void*)(packet+1);
            tcpDebug(tcpPacket);
            break;
        case 17: // udp
            printf("\nUDP. ");
            udpPacket_t* udpPacket = (void*)(packet+1);
            UDPRecv(adapter,udpPacket,length-sizeof(ipv4Packet_t));
            break;
        default:
            printf("\nother protocol based on IP. ");
            break;
    }
}

void ipv4_send(network_adapter_t* adapter, void* data, uint32_t length, uint8_t IP[4], int protocol)
{
    ipv4Packet_t* packet = malloc(sizeof(ipv4Packet_t)+length, 0, "ipv4 packet");

    memcpy(packet+1, data, length);

    memcpy(packet->destIP, IP, 4);
    memcpy(packet->sourceIP, adapter->IP_address, 4);
    packet->version        = 4;
    packet->ipHeaderLength = sizeof(ipv4Packet_t) / 4;
    packet->typeOfService  = 0;
    packet->length         = htons(sizeof(ipv4Packet_t) + length);
    packet->identification = 0;
    packet->fragmentation  = htons(0x4000);
    packet->ttl            = 128;
    packet->protocol       = protocol;
    packet->checksum       = 0;
    packet->checksum       = htons(internetChecksum(packet, sizeof(ipv4Packet_t)));


    /*
    Todo: Tell routing table to route the ip address
    */
    arpTableEntry_t * entry = arp_findEntry(&adapter->arpTable, IP);
    if(entry)
    {
        EthernetSend(adapter, packet, length+sizeof(ipv4Packet_t), entry->MAC, 0x0800);
    }
    else
    {
        arp_sendGratitiousRequest(adapter);
        entry = arp_findEntry(&adapter->arpTable, IP);
        if(entry)
        {
            EthernetSend(adapter, packet, length+sizeof(ipv4Packet_t), entry->MAC, 0x0800);
        }
        else
        {
            printf("\nThe requested IP was not found in the ARP table: %d.%d.%d.%d",IP[0],IP[1],IP[2],IP[3]);
        }
    }
    free(packet);
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
