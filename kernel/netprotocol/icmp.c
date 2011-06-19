/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "icmp.h"
#include "video/console.h"
#include "ipv4.h"
#include "util.h"


void ICMPAnswerPing(network_adapter_t* adapter, icmpheader_t* rec, uint32_t length, uint8_t sourceIP[4])
{
    size_t icmp_data_length = length-sizeof(icmpheader_t);
    uint8_t pkt[sizeof(icmpheader_t) + icmp_data_length];

    icmpheader_t* icmp = (icmpheader_t*)pkt;

    icmp->type         = ICMP_ECHO_REPLY;
    icmp->code         = 0;
    icmp->id           = rec->id;
    icmp->seqnumber    = rec->seqnumber;
    icmp->checksum     = 0;

    memcpy(&pkt[sizeof(*icmp)], (void*)(rec+1), icmp_data_length);

    icmp->checksum = htons( internetChecksum(icmp, sizeof(icmpheader_t) + icmp_data_length, 0) );

    printf("\n\n");
    printf("ICMP Header information:\n");
    textColor(0x0E);
    printf("type: %u  code: %u  checksum %u\n", icmp->type, icmp->code, icmp->checksum);
    
    ipv4_send(adapter, (void*)icmp, sizeof(icmpheader_t) + icmp_data_length, sourceIP,1);
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
