/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "icmp.h"
#include "network/rtl8139.h"
#include "video/console.h"
#include "types.h"
#include "util.h"

extern uint8_t MAC_address[6];
extern uint8_t IP_address[4];

void ICMPAnswerPing(void* data, uint32_t length)
{
    icmppacket_t*  rec = data;
    size_t icmp_data_length = ntohs(rec->ip.length) - (sizeof(rec->ip) + sizeof(rec->icmp));
    uint8_t pkt[sizeof(*rec) + icmp_data_length];
    icmppacket_t* icmp = (icmppacket_t*)pkt;

    for (uint32_t i = 0; i < 6; i++)
    {
        icmp->eth.recv_mac[i]   = rec->eth.send_mac[i]; 
        icmp->eth.send_mac[i]   = MAC_address[i];
    }

    icmp->eth.type_len[0] = 0x08;
    icmp->eth.type_len[1] = 0x00;

    for (uint32_t i = 0; i < 4; i++)
    {
        icmp->ip.destIP[i]   = rec->ip.sourceIP[i];
        icmp->ip.sourceIP[i] = IP_address[i];
    }

    icmp->ip.version        = 4;
    icmp->ip.ipHeaderLength = sizeof(icmp->ip) / 4;
    icmp->ip.typeOfService  = 0;
    icmp->ip.length         = htons(sizeof(icmp->ip) + sizeof(icmp->icmp) + icmp_data_length);
    icmp->ip.identification = 0;
    icmp->ip.fragmentation  = htons(0x4000);
    icmp->ip.ttl            = 128;
    icmp->ip.protocol       = 1;
    icmp->ip.checksum       = 0;
    icmp->ip.checksum       = htons(internetChecksum(&icmp->ip, sizeof(icmp->ip)));

    icmp->icmp.type         = ICMP_ECHO_REPLY;
    icmp->icmp.code         = 0;
    icmp->icmp.id           = rec->icmp.id;
    icmp->icmp.seqnumber    = rec->icmp.seqnumber;
    icmp->icmp.checksum     = 0;

    memcpy(&pkt[sizeof(*icmp)], (uint8_t*)data + sizeof(*rec), icmp_data_length);

    icmp->icmp.checksum = htons(internetChecksum(&icmp->icmp, sizeof(icmp->icmp) + icmp_data_length));

    transferDataToTxBuffer(icmp, sizeof(*icmp) + icmp_data_length);
}

// compute internet checksum for "count" bytes beginning at location "addr"
int internetChecksum(void *addr, size_t count)
{
    uint32_t sum  = 0;
    uint8_t* data = addr;

    while (count > 1) // inner loop
    {        
        sum   += (data[0] << 8) | data[1]; // Big Endian
        data  += 2;
        count -= 2;
    }
        
    if (count > 0) // add left-over byte, if any
    {
        sum += data[0] << 8;
    }
        
    while (sum >> 16) // fold 32-bit sum to 16 bits
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum & 0xFFFF;
}

void icmpDebug(void* data, uint32_t length)
{
	icmppacket_t*  rec = data;
    size_t icmp_data_length = ntohs(rec->ip.length) - (sizeof(rec->ip) + sizeof(rec->icmp));
    uint8_t pkt[sizeof(*rec) + icmp_data_length];
    icmppacket_t* icmp = (icmppacket_t*)pkt;
	
    icmp->icmp.type         = ICMP_ECHO_REPLY;
    icmp->icmp.code         = 0;
	icmp->icmp.checksum = htons(internetChecksum(&icmp->icmp, sizeof(icmp->icmp) + icmp_data_length));
	
	icmp->icmp.id           = rec->icmp.id;
    icmp->icmp.seqnumber    = rec->icmp.seqnumber;
    icmp->icmp.checksum     = 0;

    memcpy(&pkt[sizeof(*icmp)], (uint8_t*)data + sizeof(*rec), icmp_data_length);

    icmp->icmp.checksum = htons(internetChecksum(&icmp->icmp, sizeof(icmp->icmp) + icmp_data_length));
	
	printf("\n\n");
	printf("ICMP Header information:\n");
	textColor(0x0E);
	printf("+--------+--------+-------------+\n");
	printf("|   %u    |   %u    |     %u      | (type, code, checksum)\n", icmp->icmp.type, icmp->icmp.code, icmp->icmp.checksum);
	printf("+-------------------------------+\n");
	printf("|              %u                | (data)\n", icmp->icmp.checksum);
	printf("+-------------------------------+\n");
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
