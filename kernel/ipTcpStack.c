/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/


// http://www.rfc-editor.org/rfc/rfc793.txt <--- TRANSMISSION CONTROL PROTOCOL

#include "rtl8139.h"
#include "video/console.h"
#include "ipTcpStack.h"

void ipTcpStack_recv(void* data, uint32_t length)
{
    // first we cast our data pointer into a pointer at our Ethernet-Frame
    ethernet_t* eth = (ethernet_t*)data;

    textColor(0x0E);
    if (((eth->type_len[0] << 8) | eth->type_len[1]) > 1500) { printf("Ethernet 2. "); }
    else                                                     { printf("Ethernet 1. "); }

    // now we set our arp/ip pointer to the Ethernet-payload
    arp_t* arp = (arp_t*)((uintptr_t)eth + sizeof(ethernet_t));
    ip_t* ip   = (ip_t*) ((uintptr_t)eth + sizeof(ethernet_t));

    // to decide if it is an ip or an arp packet we just look at the ip-version
    if ((ip->version >> 4) == 4)
    {
        printf("IPv4. ");
    }
    else if ((ip->version >> 4) == 6)
    {
        printf("IPv6. ");
    }
    else
    {
        // check for ipv4 ARP packet
        if ((((arp->hardware_addresstype[0] << 8) | arp->hardware_addresstype[1]) ==    1) &&
            (((arp->protocol_addresstype[0] << 8) | arp->protocol_addresstype[1]) == 2048) &&
              (arp->hardware_addresssize == 6) &&
              (arp->protocol_addresssize == 4))
        {
            printf("ARP. ");

            // extract the operation
            switch ((arp->operation[0] << 8) | arp->operation[1])
            {
            case 1: // ARP-Request
                printf("Operation: Request\n");

                textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
                for (uint8_t i = 0; i < 6; i++) { printf("%y ", arp->source_mac[i]); }

                textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
                for (uint8_t i = 0; i < 4; i++) { printf("%u", arp->source_ip[i]); if (i<3) printf("."); }

                textColor(0x0D); printf("\nMAC Searched:   "); textColor(0x07);
                for (uint8_t i = 0; i < 6; i++) { printf("%y ", arp->dest_mac[i]);   }

                textColor(0x0D); printf("  IP Searched:   "); textColor(0x03);
                for (uint8_t i = 0; i < 4; i++) { printf("%u", arp->dest_ip[i]);   if (i<3) printf("."); }

                /// TEST
                     printf("\n Tx prepared:");
                     arpPacket_t reply;
                     for (uint32_t i = 0; i < 6; i++)
                     {
                        reply.eth.recv_mac[i]   = arp->source_mac[i];
                        if (i == 0) reply.eth.send_mac[i]   = 0x00;
                        if (i != 0) reply.eth.send_mac[i]   = 0x12;
                     }
                     reply.eth.type_len[0] = 0x08; reply.eth.type_len[1] = 0x06; 

                     for (uint32_t i = 0; i < 2; i++)
                     {
                         reply.arp.hardware_addresstype[i] = arp->hardware_addresstype[i]; 
                         reply.arp.protocol_addresstype[i] = arp->protocol_addresstype[i];                         
                     }
                     reply.arp.operation[0]         = 2; // reply
                     reply.arp.operation[1]         = 0;

                     reply.arp.hardware_addresssize = arp->hardware_addresssize;
                     reply.arp.protocol_addresssize = arp->protocol_addresssize;
                     
                     for (uint32_t i = 0; i < 6; i++)
                     {
                        reply.arp.dest_mac[i]   = arp->source_mac[i];
                        if (i == 0) reply.arp.source_mac[i]   = 0x00;
                        if (i != 0) reply.arp.source_mac[i]   = 0x12;
                     }                     

                     for (uint32_t i = 0; i < 4; i++)
                     {
                        reply.arp.dest_ip[i]   = arp->source_ip[i];
                        reply.arp.source_ip[i] = arp->dest_ip[i];
                     }

                     ipTcpStack_send((void*)&reply, eth->type_len[0] << 8 | eth->type_len[1] );
                /// TEST
                break;

            case 2: // ARP-Reply
                printf("Operation: Response\n");

                textColor(0x0D); printf("\nMAC Replying:   "); textColor(0x03);
                for (uint8_t i = 0; i < 6; i++) { printf("%y ", arp->source_mac[i]); }

                textColor(0x0D); printf("  IP Replying:   "); textColor(0x03);
                for (uint8_t i = 0; i < 4; i++) { printf("%u", arp->source_ip[i]); if (i<3) printf("."); }

                textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
                for (uint8_t i = 0; i < 6; i++) { printf("%y ", arp->dest_mac[i]);   }

                textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
                for (uint8_t i = 0; i < 4; i++) { printf("%u", arp->dest_ip[i]);   if (i<3) printf("."); }
                break;
            }
        }
        else
        {
            // TODO
        }
    }
    printf("\n");
    textColor(0x0F);
}


bool ipTcpStack_send(void* data, uint32_t length)
{
	/*
    if (length > 0x700) 
    {
        return false; 
    }
    */

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

    return transferDataToTxBuffer(data, length);     
}

/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
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
