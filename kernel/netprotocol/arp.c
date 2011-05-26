/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "arp.h"
#include "video/console.h"


void arp_received(network_adapter_t* adapter, arpPacket_t* packet)
{
    if ((((packet->arp.hardware_addresstype[0] << 8) | packet->arp.hardware_addresstype[1]) ==    1) &&
        (((packet->arp.protocol_addresstype[0] << 8) | packet->arp.protocol_addresstype[1]) == 2048) &&
        (packet->arp.hardware_addresssize == 6) &&
        (packet->arp.protocol_addresssize == 4))
    {
        // extract the operation
        switch ((packet->arp.operation[0] << 8) | packet->arp.operation[1])
        {
        case 1: // ARP-Request
            if ((packet->arp.sourceIP[0] == packet->arp.destIP[0]) &&
                (packet->arp.sourceIP[1] == packet->arp.destIP[1]) &&
                (packet->arp.sourceIP[2] == packet->arp.destIP[2]) &&
                (packet->arp.sourceIP[3] == packet->arp.destIP[3])) // IP requ. and searched is identical
            {
                printf("Operation: Gratuitous Request\n");
            }
            else
            {
                printf("Operation: Request\n");
            }

            textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->arp.source_mac[i]); }
            textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->arp.sourceIP[i]); if (i<3) printf("."); }
            textColor(0x0D); printf("\nMAC Searched:   "); textColor(0x07);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->arp.dest_mac[i]);   }
            textColor(0x0D); printf("  IP Searched:   "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->arp.destIP[i]);   if (i<3) printf("."); }

            // requested IP is our own IP?
            if (packet->arp.destIP[0] == adapter->IP_address[0] && packet->arp.destIP[1] == adapter->IP_address[1] &&
                packet->arp.destIP[2] == adapter->IP_address[2] && packet->arp.destIP[3] == adapter->IP_address[3])
            {
                printf("\n Tx prepared:");
                arpPacket_t reply;
                for (uint8_t i = 0; i < 6; i++)
                {
                    reply.eth.recv_mac[i] = packet->arp.source_mac[i];
                    reply.eth.send_mac[i] = adapter->MAC_address[i];
                }
                reply.eth.type_len[0] = 0x08; reply.eth.type_len[1] = 0x06;

                for (uint8_t i = 0; i < 2; i++)
                {
                    reply.arp.hardware_addresstype[i] = packet->arp.hardware_addresstype[i];
                    reply.arp.protocol_addresstype[i] = packet->arp.protocol_addresstype[i];
                }
                reply.arp.operation[0] = 0;
                reply.arp.operation[1] = 2; // reply

                reply.arp.hardware_addresssize = packet->arp.hardware_addresssize;
                reply.arp.protocol_addresssize = packet->arp.protocol_addresssize;

                for (uint8_t i = 0; i < 6; i++)
                {
                    reply.arp.dest_mac[i]   = packet->arp.source_mac[i];
                    reply.arp.source_mac[i] = adapter->MAC_address[i];
                }

                for (uint8_t i = 0; i < 4; i++)
                {
                    reply.arp.destIP[i]   = packet->arp.sourceIP[i];
                    reply.arp.sourceIP[i] = adapter->IP_address[i];
                }

                EthernetSend(adapter, (void*)&reply, sizeof(arpPacket_t));
            }
            break;
        case 2: // ARP-Reply
            printf("Operation: Response\n");

            textColor(0x0D); printf("\nMAC Replying:   "); textColor(0x03);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->arp.source_mac[i]); }

            textColor(0x0D); printf("  IP Replying:   "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->arp.sourceIP[i]); if (i<3) printf("."); }

            textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->arp.dest_mac[i]);   }

            textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->arp.destIP[i]);   if (i<3) printf("."); }
            break;
        } // switch
    } // if
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
