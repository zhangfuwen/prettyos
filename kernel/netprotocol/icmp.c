/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "icmp.h"
#include "network/rtl8139.h"
#include "video/console.h"
#include "types.h"

extern uint8_t MAC_address[6];
extern uint8_t IP_address[4];

// Compute Internet Checksum for "count" bytes beginning at location "addr".
/*
void internetChecksum(uint16_t addr, uint32_t count)
{
    // register 
	uint32_t sum = 0;
	uint32_t count, checksum;
	uint16_t addr;
    while( count > 1 )  
    {
        // This is the inner loop 
        sum += * (uint16_t) addr++;
        count -= 2;
    }
    // Add left-over byte, if any 
    if( count > 0 )
    sum += * (uint8_t*) addr;
    // Fold 32-bit sum to 16 bits 
    while (sum>>16)
    sum = (sum & 0xffff) + (sum >> 16);
    checksum = ~sum;
}
*/

/*
void internetChecksum()
{
	register long sum = 0;

	while( count > 1 )  {
	   //  This is the inner loop 
		   sum += * (unsigned short) addr++;
		   count -= 2;
	}

	   // Add left-over byte, if any 
	if( count > 0 )
		   sum += * (unsigned char *) addr;

	   //  Fold 32-bit sum to 16 bits 
	while (sum>>16)
	   sum = (sum & 0xffff) + (sum >> 16);

	checksum = ~sum;
}
*/

void ICMPAnswerPing(void* data, uint32_t length)
{
	// icmpheader_t icmp;
	icmppacket_t icmp;
	
	// first we cast our data pointer into a pointer at our Ethernet-Frame
    ethernet_t* eth = (ethernet_t*)data;

    // now we set our ip pointer to the Ethernet-payload
    ip_t* ip   = (ip_t*) ((uintptr_t)eth + sizeof(ethernet_t));

	for (uint32_t i = 0; i < 6; i++)
    {
		icmp.eth.recv_mac[i]   = eth->recv_mac[i]; // arp->source_mac[i];
		icmp.eth.send_mac[i]   = MAC_address[i];
	}
    
	icmp.eth.type_len[0] = 0x08;
	icmp.eth.type_len[1] = 0x06;
/*
	icmp.ip.dest_ip[0]	 = 192;
	icmp.ip.dest_ip[1]	 = 168;
	icmp.ip.dest_ip[2]	 = 10;
	icmp.ip.dest_ip[3]	 = 5;
*/
	for (uint32_t i = 0; i < 4; i++)
	{
		// reply.arp.dest_ip[i]   = arp->source_ip[i];
		// reply.arp.source_ip[i] = IP_address[i];
		icmp.ip.dest_ip[i]	 = ip->source_ip[i];
		icmp.ip.source_ip[i] = IP_address[i];
	}
	
	icmp.icmp.type = ECHO_REQUEST;
	icmp.icmp.code = ECHO_REPLY;
	icmp.icmp.checksum = 0x475c;

	transferDataToTxBuffer((void*)&icmp, sizeof(icmpheader_t));
	textColor(0x0D); printf("  ICMP Packet send!!! "); textColor(0x03);
	textColor(0x0D); printf("  ICMP Packet: dest_ip: %u.%u.%u.%u", icmp.ip.dest_ip[0], icmp.ip.dest_ip[1], icmp.ip.dest_ip[2], icmp.ip.dest_ip[3]); textColor(0x03);
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
