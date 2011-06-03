/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "dhcp.h"
#include "udp.h"
#include "video/console.h"


void DHCP_Discover(network_adapter_t* adapter)
{
    static uint32_t xid = 0x0000E0FF0A; // AFFE....
    xid += (1<<24);
    printf("\nDHCP Discover\n");
    dhcp_t packet;
    packet.op = 1;
    packet.htype = 1;
    packet.hlen = 6;
    packet.hops = 0;
    packet.xid = xid; // AFFExx
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
    
    packet.options[4]  =   1;  // SUBNET
    packet.options[5]  =   4;  // Length
    packet.options[6]  = 255;  // 
    packet.options[7]  = 255;  // 
    packet.options[8]  = 255;  // 
    packet.options[9]  =   0;  // 

    packet.options[10]  = 53;  // MESSAGE TYPE
    packet.options[11]  =  1;  // Length
    packet.options[12]  =  1;  // (data)

    packet.options[13]  = 55;  // 
    packet.options[14]  =  8;  // Length
    packet.options[15]  =  1;  // SUBNET MASK 
    packet.options[16] =   3;  // ROUTERS
    packet.options[17] =   6;  // DOMAIN NAME SERVER
    packet.options[18] =  15;  // DOMAIN NAME
    packet.options[19] =  28;  // BROADCAST ADDRESS
    packet.options[20] =  31;  // Perform Router Discover  
    packet.options[21] =  33;  // Static Route
    packet.options[22] =  42;  // Network Time Protocol (NTP) SERVERS
    
    packet.options[23] =  61;  // Client Identifier
    packet.options[24] =   7;  // Length
    packet.options[25] =   1;  // Type
    for(uint8_t i = 0; i < 6; i++)
        packet.options[26+i] = adapter->MAC_address[i];

    packet.options[32] = 116; // DHCP Auto Configuration
    packet.options[33] =   1; // Length 
    packet.options[34] =   0; // 

    packet.options[35]  =  50; // Requested IP
    packet.options[36]  =   4; // Length
    packet.options[37]  = 192; // 
    packet.options[38]  = 168; // 
    packet.options[39]  =  10; // 
    packet.options[40]  =  98; // 

    packet.options[41] = 255;  // end

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
