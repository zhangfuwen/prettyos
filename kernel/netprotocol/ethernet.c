/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/


// http://www.rfc-editor.org/rfc/rfc793.txt <--- TRANSMISSION CONTROL PROTOCOL

#include "ethernet.h"
#include "video/console.h"
#include "arp.h"
#include "ipv4.h"
#include "kheap.h"
#include "network/netutils.h"
#include "util.h"

extern Packet_t lastPacket; // network.c

static const uint8_t broadcast_MAC_FF[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t broadcast_MAC_00[6] = {0, 0, 0, 0, 0, 0};

void EthernetRecv(network_adapter_t* adapter, ethernet_t* eth, uint32_t length)
{
    lastPacket.ethLength = length;

	textColor(LIGHT_BLUE);
    printf("\n\n>> Packet received. <<");
    textColor(HEADLINE);
    printf("\nEth: ");
    if (memcmp(eth->recv_mac, adapter->MAC, 6) != 0 && memcmp(eth->recv_mac, broadcast_MAC_FF, 6) != 0 && memcmp(eth->recv_mac, broadcast_MAC_00, 6) != 0)
    {
        textColor(TEXT);
        printf("Ethernet packet received. We are not the addressee.");
        return;
    }
    memcpy(lastPacket.MAC, eth->send_mac, 6); // save sender

    // output ethernet packet
  #ifdef _NETWORK_DATA_
    uint16_t ethernetType = (eth->type_len[0] << 8) + eth->type_len[1]; // Big Endian
    textColor(HEADLINE); printf("\nLength: ");
    textColor(TEXT); printf("%d ", length);
  #endif

    textColor(GRAY); printf(" %M\t<== %M", eth->recv_mac, eth->send_mac);

  #ifdef _NETWORK_DATA_
    textColor(LIGHT_MAGENTA);
    printf("\nEthernet: ");

    textColor(HEADLINE);
    if (ethernetType <= 1500) { printf("type 1, "); }
    else                      { printf("type 2, "); }

    textColor(TEXT);
    if (ethernetType <= 1500) { printf("Length: "); }
    else                      { printf("Type: ");   }

    /*
    textColor(DATA);
    for (uint8_t i = 0; i < 2; i++)
    {
        printf("%yh ", eth->type_len[i]);
    }

    uint32_t printlength = max(length, 80);
    printf("\n");
    for (uint32_t i = sizeof(ethernet_t); i <= printlength; i++)
    {
        printf("%y ", eth[i]);
    }
    */
  #endif

    textColor(TEXT);
    if (((eth->type_len[0] << 8) | eth->type_len[1]) > 1500)
    {
        // cf. http://en.wikipedia.org/wiki/EtherType
        // and http://www.cavebear.com/archive/cavebear/Ethernet/type.html

        // now we look for IPv4, IPv6, or ARP
        if ((eth->type_len[0] == 0x08) && (eth->type_len[1] == 0x00)) // IPv4
        {
            ipv4_received(adapter, (void*)(eth+1), length-sizeof(ethernet_t));
        }

        else if ((eth->type_len[0] == 0x86) && (eth->type_len[1] == 0xDD)) // IPv6
        {
          #ifdef _NETWORK_DATA_
            printf("Protocol type: IPv6. Currently, not further analyzed. ");
          #endif
            // TODO analyze IPv6
        }

        else if ((eth->type_len[0] == 0x08) && (eth->type_len[1] == 0x06)) // ARP
        {
            arp_received(adapter, (void*)(eth+1));
        }
        else
        {
          #ifdef _NETWORK_DATA_
            printf("Protocol type: Neither IP nor ARP.");
          #endif
        }
    } // end of ethernet 2
    else
    {
        printf("Ethernet 1. ");
    }
}

bool EthernetSend(network_adapter_t* adapter, void* data, uint32_t length, uint8_t MAC[6], uint16_t type)
{
    textColor(HEADLINE);
    printf("\nEth send:");
    textColor(GRAY);
    printf(" %M ==> %M", adapter->MAC, MAC);
    textColor(TEXT);
    printf("  length = %u.", sizeof(ethernet_t) + length);
    if (sizeof(ethernet_t) + length > 0x700)
    {
        textColor(ERROR);
        printf("\nError: This is more than (1792) 0x700\n", sizeof(ethernet_t) + length);
        return false;
    }

    ethernet_t* packet = malloc(sizeof(ethernet_t)+length, 0, "ethernet packet");

    memcpy(packet+1, data, length);
    memcpy(packet->recv_mac, MAC, 6);
    memcpy(packet->send_mac, adapter->MAC, 6);
    *(uint16_t*)packet->type_len = htons(type);

    bool retVal = network_sendPacket(adapter, (void*)packet, length + sizeof(ethernet_t));

    free(packet);
    return(retVal);
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
