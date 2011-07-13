/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "network/netutils.h"
#include "video/console.h"
#include "util.h"
#include "netbios.h"


void NetBIOS_Datagramm_Receive(network_adapter_t* adapter, NetBIOSDatagramHeader_t* NetBIOSdgm)
{
  #ifdef _NETBIOS_DEBUG_
    textColor(HEADLINE);
    printf("\nmessage type: ");
    textColor(TEXT);

    switch (NetBIOSdgm->messageType)
    {
        case 0x10: printf("Direct unique datagram");           break;
        case 0x11: printf("Direct group datagram");            break;
        case 0x12: printf("Broadcast datagram");               break;
        case 0x13: printf("Datagram error");                   break;
        case 0x14: printf("Datagram query request");           break;
        case 0x15: printf("Datagram positive query response"); break;
        case 0x16: printf("Datagram negative query response"); break;
    }
    textColor(GRAY);
    printf("\nflags: %yh", NetBIOSdgm->flags);
    printf(" ID: %xh",     ntohs(NetBIOSdgm->ID));
    printf(" IP: %I",      NetBIOSdgm->sourceIP);
    printf(" Port: %u",    ntohs(NetBIOSdgm->sourcePort));
    printf(" Len: %u",     ntohs(NetBIOSdgm->Length));
    printf(" Offset: %u",  ntohs(NetBIOSdgm->packetOffset));
    textColor(TEXT);
  #endif
}


/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
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
