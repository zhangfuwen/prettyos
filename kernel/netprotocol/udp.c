/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// http://www.rfc-editor.org/rfc/rfc768.txt <---  User Datagram Protocol

#include "video/console.h"
#include "udp.h"
#include "ipTcpStack.h"

void UDPRecv(void* data, uint32_t length)
{
	
    udppacket_t* rec = data;
    size_t udp_data_length = ntohs(rec->ip.length) - (sizeof(rec->eth) + sizeof(rec->ip) + sizeof(rec->udp));
    uint8_t pkt[sizeof(*rec) + udp_data_length];
    udppacket_t* udp = (udppacket_t*)pkt;
	
	printf("\n");
	printf("source port: %u\n", udp->udp.source_port);
	printf("destination port: %u\n", udp->udp.destination_port);
}

void UDPSend(void* data, uint32_t length)
{
    // transferDataToTxBuffer(void* data, uint32_t length);
}

void UDPDebug(void* data, uint32_t length)
{
    udppacket_t* rec = data;
    size_t udp_data_length = ntohs(rec->ip.length) - (sizeof(rec->eth) + sizeof(rec->ip) + sizeof(rec->udp));
    uint8_t pkt[sizeof(*rec) + udp_data_length];
    udppacket_t* udp = (udppacket_t*)pkt;
	
	printf("\n");
	printf("UDP Header information:\n");
	textColor(0x0E);
	printf("+--------------+----------------+\n");
	printf("|      %u       |      %u        | (source port, destination port)\n", udp->udp.source_port, udp->udp.destination_port);
	printf("+-------------------------------+\n");
	printf("|        %u     |      %u        | (length, checksum)\n", udp->udp.length, udp->udp.checksum);
	printf("+-------------------------------+\n");
	printf("|              data              | (data)\n", udp->udp.checksum);
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
