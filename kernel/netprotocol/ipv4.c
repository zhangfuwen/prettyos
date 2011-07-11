/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "ipv4.h"
#include "tcp.h"
#include "udp.h"
#include "icmp.h"
#include "arp.h"
#include "ethernet.h"
#include "video/console.h"
#include "kheap.h"
#include "util.h"


bool isSubnet(IP_t IP, IP_t myIP, IP_t subnet)
{
    return((IP.iIP & subnet.iIP) == (myIP.iIP & subnet.iIP));
}

extern Packet_t lastPacket; // network.c

static const IP_t broadcast_IP = {.iIP = 0xFFFFFFFF};
static const IP_t broadcast_IP2 = {.iIP = 0};
static const uint8_t broadcast_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void ipv4_received(struct network_adapter* adapter, ipv4Packet_t* packet, uint32_t length)
{
    lastPacket.ipLength = length;
  #ifdef _NETWORK_DATA_
    textColor(HEADLINE);
    printf("\nIPv4:");
    textColor(IMPORTANT);
    printf(" %I\t<== %I", packet->destIP, packet->sourceIP);
    textColor(TEXT);
    if(packet->destIP.iIP != adapter->IP.iIP && packet->destIP.iIP != broadcast_IP.iIP)
    {
        printf("\nWe are not the addressee.");
        return;
    }
  #endif
    memcpy(lastPacket.IP.IP, packet->sourceIP.IP, 4); // save sender IP

    // IPv4 protocol is parsed here and distributed in switch/case
    uint32_t ipHeaderLengthBytes = 4 * packet->ipHeaderLength; // is given as number of 32 bit pieces (4 byte)
    switch(packet->protocol)
    {
        case 1:  // icmp
            icmp_Receive(adapter, (void*)packet+ipHeaderLengthBytes, ntohs(packet->length), packet->sourceIP);
            break;
        case 6:  // tcp
            tcp_receive(adapter, (void*)packet+ipHeaderLengthBytes, packet->sourceIP, ntohs(packet->length)-ipHeaderLengthBytes);
            break;
        case 17: // udp
            UDPRecv(adapter, (void*)packet+ipHeaderLengthBytes, ntohs(packet->length)-ipHeaderLengthBytes);
            break;
        default:
            textColor(IMPORTANT);
            printf("\nUnexpected protocol after IP packet: %u", packet->protocol);
            textColor(TEXT);
            break;
    }
}

void ipv4_send(network_adapter_t* adapter, void* data, uint32_t length, IP_t IP, int protocol)
{
    ipv4Packet_t* packet = malloc(sizeof(ipv4Packet_t)+length, 0, "ipv4 packet");

    memcpy(packet+1, data, length);

    packet->destIP.iIP = IP.iIP;
    packet->sourceIP.iIP = adapter->IP.iIP;
    packet->version        = 4;
    packet->ipHeaderLength = sizeof(ipv4Packet_t) / 4;
    packet->typeOfService  = 0;
    packet->length         = htons( sizeof(ipv4Packet_t) + length );
    packet->identification = 0;
    packet->fragmentation  = htons(0x4000); // do not fragment
    packet->ttl            = 128;
    packet->protocol       = protocol;
    packet->checksum       = 0;
    packet->checksum       = htons(internetChecksum(packet, sizeof(ipv4Packet_t), 0)); // util.c


    /*
    Todo: Tell routing table to route the ip address
    */

    if(IP.iIP == broadcast_IP.iIP || IP.iIP == broadcast_IP2.iIP || isSubnet(IP, adapter->IP, adapter->Subnet)) // IP is in LAN
    {
		printf("\nIP: %I adapter->IP: %I adapter->Subnet: %I", IP, adapter->IP, adapter->Subnet);
		printf("\nIP is in LAN");
  
        arpTableEntry_t* entry = arp_findEntry(&adapter->arpTable, IP);
        if(entry == 0) // Try to find IP by ARP request
        {
            printf("\nWe try to find %I", IP);
            arp_sendRequest(adapter, IP);
            if(arp_waitForReply(adapter, IP) == false)
            {
                printf("\nThe requested IP is in LAN, but was not found.");
                entry = arp_findEntry(&adapter->arpTable, adapter->Gateway_IP);
				printf("\nWe try to deliver the packet to the gateway %I (%M)", adapter->Gateway_IP, entry->MAC);
				EthernetSend(adapter, packet, length+sizeof(ipv4Packet_t), entry->MAC, 0x0800);
			}
            entry = arp_findEntry(&adapter->arpTable, IP);
        }

        EthernetSend(adapter, packet, length+sizeof(ipv4Packet_t), entry->MAC, 0x0800);
    }
    else // IP is not in LAN. Send packet to server
    {
  #ifdef QEMU_HACK
        uint8_t gatewayMAC[6] = {GW_MAC_1, GW_MAC_2, GW_MAC_3, GW_MAC_4, GW_MAC_5, GW_MAC_6}; // HACK for TCP with qemu
        //textColor(GRAY);
        //printf("\nqemu hack: We try to deliver the packet to the gateway %M", gatewayMAC);
        //textColor(TEXT);
        EthernetSend(adapter, packet, length+sizeof(ipv4Packet_t), gatewayMAC, 0x0800);
  #else
        arpTableEntry_t* entry = arp_findEntry(&adapter->arpTable, adapter->Gateway_IP);
        if(entry == 0) // Try to find Server by ARP request
        {
            printf("\nWe try to find %I", adapter->Gateway_IP);
            arp_sendRequest(adapter, adapter->Gateway_IP);
            if(arp_waitForReply(adapter, adapter->Gateway_IP) == false)
            {
                printf("\nThe server was not found");
                return;
            }
            entry = arp_findEntry(&adapter->arpTable, adapter->Gateway_IP);
        }

        printf("\nWe try to deliver the packet to the gateway %I (%M)", adapter->Gateway_IP, entry->MAC);


        EthernetSend(adapter, packet, length+sizeof(ipv4Packet_t), entry->MAC, 0x0800);
  #endif
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
