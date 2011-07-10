/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "icmp.h"
#include "video/console.h"
#include "ipv4.h"
#include "util.h"
#include "kheap.h"
#include "network/netutils.h"


void icmp_Send_echoRequest (network_adapter_t* adapter, IP_t destIP)
{
    static uint16_t count = 0;
    count++;

    char* data = "PrettyOS ist das Betriebssystem der Projektgruppe \"OS-Development\" im deutschsprachigen C++-Forum";
    icmpheader_t* icmp = malloc(sizeof(icmpheader_t) + strlen(data), 0, "ICMP packet");
    memcpy(icmp+1, (void*)data, strlen(data));
    icmp->type         = 8; // echo request
    icmp->code         = 0;
    icmp->id           = htons(0xAFFE);
    icmp->seqnumber    = htons(count);
    icmp->checksum     = 0;
    icmp->checksum     = htons( internetChecksum(icmp, sizeof(icmpheader_t) + strlen(data), 0) );

    ipv4_send(adapter, icmp, sizeof(icmpheader_t) + strlen(data), destIP, 1);
    free(icmp);
	textColor(HEADLINE);  printf("\nICMP: ");
	textColor(TEXT);      printf("echo request (PING) send to ");
	textColor(IMPORTANT); printf("%I", destIP);
}

void icmp_Receive(network_adapter_t* adapter, icmpheader_t* rec, uint32_t length, IP_t sourceIP)
{
    if (rec->type == 0) // Echo Reply
    {
        textColor(HEADLINE);
        printf("\nICMP_echoReply:");

        size_t icmp_data_length = length - sizeof(icmpheader_t);
        if (rec->code == 0)
        {
            textColor(TEXT);
            printf("  ID: %x seq: %u\n", ntohs(rec->id), ntohs(rec->seqnumber));
            char str[icmp_data_length+2];
            strncpy(str, (char*)(rec+1), icmp_data_length);
            str[icmp_data_length+1] = 0;
            textColor(DATA);
            printf("%s", str);
        }
        textColor(TEXT);
    }
	else if(rec->type == 3) // Destination Unreachable
	{
		switch (rec->code)
		{
			case 0:
				printf("\ncode 0: Destination network unreachable");
				break;
			case 1:
				printf("\ncode 1: Destination host unreachable");
				break;
			case 2:
				printf("\ncode 2: Destination protocol unreachable");
				break;
			case 3:
				printf("\ncode 3: Destination port unreachable");
				break;
			case 4:
				printf("\ncode 4: Fragmentation required, and DF flag set");
				break;
			case 5:
				printf("\ncode 5: Source route failed");
				break;
			case 6:
				printf("\ncode 6: Destination network unknown");
				break;
			case 7:
				printf("\ncode 7: Destination host unknown");
				break;
			case 8:
				printf("\ncode 8: Source host isolated");
				break;
			case 9:
				printf("\ncode 9: Network administratively prohibited");
				break;
			case 10:
				printf("\ncode 10: Host administratively prohibited");
				break;
			case 11:
				printf("\ncode 11: Network unreachable for TOS");
				break;
			case 12:
				printf("\ncode 12: Host unreachable for TOS");
				break;
			case 13:
				printf("\ncode 13: Communication administratively prohibited");
				break;
		}
	}
	else if(rec->type == 4) // Source Quench  
	{
		printf("\nSource quench (congestion control)");		
	}
    else if(rec->type == 5) // Redirect Message
	{
		switch (rec->code)
		{
			case 0:
				printf("\ncode 0: Redirect Datagram for the Network");
				break;
			case 1:
				printf("\ncode 1: Redirect Datagram for the Host");
				break;
		    case 2:
				printf("\ncode 2: Redirect Datagram for the TOS & network");
				break;
			case 3:
				printf("\ncode 3: Redirect Datagram for the TOS & host");
				break;
		}		
	}
	else if(rec->type == 6) 
	{
		printf("\nAlternate Host Address");		
	}
	// 7 Reserved
	else if (rec->type == 8) // Echo Request
    {
        textColor(HEADLINE);
        printf("\nICMP_echoRequest:");

        size_t icmp_data_length = length - sizeof(icmpheader_t);
        uint8_t pkt[sizeof(icmpheader_t) + icmp_data_length];

        icmpheader_t* icmp = (icmpheader_t*)pkt;

        icmp->type         = ICMP_ECHO_REPLY;
        icmp->code         = 0;
        icmp->id           = rec->id;
        icmp->seqnumber    = rec->seqnumber;
        icmp->checksum     = 0;

        memcpy(&pkt[sizeof(*icmp)], (void*)(rec+1), icmp_data_length);

        icmp->checksum = htons( internetChecksum(icmp, sizeof(icmpheader_t) + icmp_data_length, 0) );

        textColor(TEXT);
        printf(" type: %u  code: %u  checksum %u\n", icmp->type, icmp->code, icmp->checksum);

        ipv4_send(adapter, (void*)icmp, sizeof(icmpheader_t) + icmp_data_length, sourceIP,1);
    }
	else if(rec->type == 9) 
	{
		printf("\n	Router Advertisement");		
	}
	else if(rec->type == 10) 
	{
		printf("\nRouter discovery/selection/solicitation");		
	}
	else if(rec->type == 11) // Time Exceeded 
	{
		printf("\nTime Exceeded ");
		switch (rec->code)
		{
			case 0:
				printf(" code 0: TTL expired in transit");
				break;
			case 1:
				printf(" code 1: Fragment reassembly time exceeded");
				break;            
		}
	}
    else if(rec->type == 12) 
	{
		printf("\nParameter Problem: Bad IP header");
		switch (rec->code)
		{
			case 0:
				printf(" code 0: Pointer indicates the error");
				break;
			case 1:
				printf(" code 1: Missing a required option");
				break;            
		}
	}
    else if(rec->type == 13) 
	{
		printf("Timestamp");
	}
    else if(rec->type == 14) 
	{
		printf("Timestamp reply");
	}
    else if(rec->type == 15) 
	{
		printf("Information Request");
	}
    else if(rec->type == 16) 
	{
		printf("Information Reply");
	}
    else if(rec->type == 17) 
	{
		printf("Address Mask Request");
	}
    else if(rec->type == 18) 
	{
		printf("Address Mask Reply");
	}  
	// 19 Reserved for security
    // 20 through 29 Reserved for robustness experiment
    else if(rec->type == 30) 
	{
		printf("Information Request");
	}
    else if(rec->type == 31) 
	{
		printf("Datagram Conversion Error");
	}
    else if(rec->type == 32) 
	{
		printf("Mobile Host Redirect");
	}
    else if(rec->type == 33) 
	{
		printf("Where-Are-You (originally meant for IPv6)");
	}
    else if(rec->type == 34) 
	{
		printf("Here-I-Am (originally meant for IPv6)");
	}
    else if(rec->type == 35) 
	{
		printf("Mobile Registration Request");
	}
    else if(rec->type == 36) 
	{
		printf("Mobile Registration Reply");
	}
    else if(rec->type == 37) 
	{
		printf("Domain Name Request");
	}
    else if(rec->type == 38) 
	{
		printf("Domain Name Reply");
	}
    else if(rec->type == 39) 
	{
		printf("SKIP Algorithm Discovery Protocol, Simple Key-Management for Internet Protocol");
	}
    else if(rec->type == 40) 
	{
		printf("Photuris, Security failures");
	}
    else if(rec->type == 41) 
	{
		printf("ICMP for experimental mobility protocols such as Seamoby [RFC4065]");
	}
	// 42 through 255 Reserved
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
