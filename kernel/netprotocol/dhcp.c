/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "dhcp.h"
#include "udp.h"
#include "video/console.h"


void DHCP_Discover(network_adapter_t* adapter)
{
    printf("\nDHCP Discover\n");
    dhcp_t packet;
    packet.op = 1;
    packet.htype = 1;
    packet.hlen = 6;
    packet.hops = 0;
    packet.xid = 0x000AFFE000;
    packet.secs = 0;
    packet.flags = 0;
    for(uint8_t i = 0; i < 4; i++)
    {
        packet.ciaddr[i] = 0;
        packet.yiaddr[i] = 0;
        packet.siaddr[i] = 0;
        packet.giaddr[i] = 0;
    }

    for(uint8_t i = 0; i < 6; i++)
        packet.chaddr[i] = adapter->MAC_address[i];
    for(uint8_t i = 6; i < 16; i++)
        packet.chaddr[i] = 0;
    packet.sname[0] = 0;
    packet.file[0] = 0;

    // options
    for(uint16_t i = 0; i < 312; i++)
        packet.options[i] = 0;
      
    packet.options[0]  =  99;  // MAGIC
    packet.options[1]  = 130;  // MAGIC
    packet.options[2]  =  83;  // MAGIC
    packet.options[3]  =  99;  // MAGIC
    
    packet.options[4]  =  53;  // MESSAGE TYPE
    packet.options[5]  =   1;  // Length
    packet.options[6]  =   1;  // (data)

    packet.options[7]  =  55;  // 
    packet.options[8]  =   8;  // Length
    packet.options[9]  =   1;  // SUBNET MASK 
    packet.options[10] =   3;  // ROUTERS
    packet.options[11] =   6;  // DOMAIN NAME SERVER
    packet.options[12] =  15;  // DOMAIN NAME
    packet.options[13] =  28;  // BROADCAST ADDRESS
    packet.options[14] =  31;  // Perform Router Discover  
    packet.options[15] =  33;  // Static Route
    packet.options[16] =  42;  // Network Time Protocol (NTP) SERVERS
    
    packet.options[17] = 116; // DHCP Auto Configuration
    packet.options[18] =   1; // Length 
    packet.options[19] =   1; // 

    packet.options[20] = 255;  // end

    uint8_t srcIP[4] = {0,0,0,0};
    uint8_t destIP[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    UDPSend(adapter, &packet, sizeof(dhcp_t), 68, srcIP, 67, destIP);
}

void DHCP_Request()
{
}

void DHCP_AnalyzeServerMessage()
{
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
