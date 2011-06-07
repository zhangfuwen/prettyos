/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "dhcp.h"
#include "udp.h"
#include "video/console.h"

uint32_t xid = 0x0000E0FF0A; // AFFE.... Transaction Code for Identification

void DHCP_Discover(network_adapter_t* adapter)
{
    xid += (1<<24);

    printf("\nDHCP Discover sent.\n");

    dhcp_t packet;
    packet.op = 1;
    packet.htype = 1; // Type: for ethernet and 802.11 wireless clients, the hardware type is always 01
    packet.hlen = 6;
    packet.hops = 0;
    packet.xid = xid; // AFFExx
    packet.secs = htons(0);
    packet.flags = 0;
    for(uint8_t i = 0; i < 4; i++)
    {
        // packet.ciaddr[i] = 0;
        packet.yiaddr[i] = 0;
        packet.siaddr[i] = 0;
        packet.giaddr[i] = 0;
    }

    // TEST
    packet.ciaddr[0] = IP_1;
    packet.ciaddr[1] = IP_2;
    packet.ciaddr[2] = IP_3;
    packet.ciaddr[3] = IP_4;

    for(uint8_t i = 0; i <   6; i++)  packet.chaddr[i] = adapter->MAC_address[i];
    for(uint8_t i = 6; i <  16; i++)  packet.chaddr[i] = 0;
    for(uint8_t i = 0; i <  64; i++)  packet.sname[i]  = 0;
    for(uint8_t i = 0; i < 128; i++)  packet.file[i]   = 0;

    // options
    packet.options[0]  =  99;  // MAGIC
    packet.options[1]  = 130;  // MAGIC
    packet.options[2]  =  83;  // MAGIC
    packet.options[3]  =  99;  // MAGIC
    for(uint16_t i = 4; i < OPTIONSIZE; i++)
        packet.options[i] = 255; // fill with end token

    packet.options[4]  =  53;  // MESSAGE TYPE
    packet.options[5]  =   1;  // Length
    packet.options[6]  =   1;  // DISCOVER

    packet.options[7]  =  57;  // MAX MESSAGE SIZE
    packet.options[8]  =   2;  // Length
    packet.options[9]  =   2;  // (data) 2*256 //
    packet.options[10] =  64;  // (data)    64 // max size: 576

    uint8_t srcIP[4] = {0,0,0,0};
    uint8_t destIP[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    UDPSend(adapter, &packet, sizeof(dhcp_t), 68, srcIP, 67, destIP);
}

void DHCP_Request()
{
}

/*
   uint8_t  op;        // DHCP_BOOTREQEUST or DHCP_BOOTREPLY
    uint8_t  htype;     // DHCP_HTYPE10MB
    uint8_t  hlen;      // DHCP_HLENETHERNET
    uint8_t  hops;      // DHCP_HOPS
    uint32_t xid;       // DHCP_XID
    uint16_t secs;      // DHCP_SECS
    uint16_t flags;     // DHCP_FLAGSBROADCAST
    uint8_t  ciaddr[4];
    uint8_t  yiaddr[4];
    uint8_t  siaddr[4];
    uint8_t  giaddr[4];
    uint8_t  chaddr[16];
    char     sname[64];
    char     file[128];
    uint8_t  options[OPTIONSIZE]; 
*/

void DHCP_AnalyzeServerMessage(dhcp_t* dhcp)
{
    printf("\nop: %u", dhcp->op);  
    printf(" htype: %u", dhcp->htype);  
    printf(" hlen: %u", dhcp->hlen);  
    printf(" hops: %u", dhcp->hops);
    printf(" xid: %X", htonl(dhcp->xid));
    printf(" secs: %u", htons(dhcp->secs));
    printf(" flags: %x", htons(dhcp->flags));
    printf("\ncIP: %u.%u.%u.%u\n", dhcp->ciaddr[0], dhcp->ciaddr[1], dhcp->ciaddr[2], dhcp->ciaddr[3]);
    printf(" yIP: %u.%u.%u.%u\n", dhcp->yiaddr[0], dhcp->yiaddr[1], dhcp->yiaddr[2], dhcp->yiaddr[3]);
    printf("\nsIP: %u.%u.%u.%u\n", dhcp->siaddr[0], dhcp->siaddr[1], dhcp->siaddr[2], dhcp->siaddr[3]);
    printf(" gIP: %u.%u.%u.%u\n", dhcp->giaddr[0], dhcp->giaddr[1], dhcp->giaddr[2], dhcp->giaddr[3]);
    printf("\nMAC: %y-%y-%y-%y-%y-%y", dhcp->chaddr[0], dhcp->chaddr[1], dhcp->chaddr[2], 
                                       dhcp->chaddr[3], dhcp->chaddr[4], dhcp->chaddr[5]);
    printf("\n");
    for (uint16_t i=0; i<60; i++)
    {
        printf("%y ",dhcp->options[i]);
    }
}

void DHCP_Inform(network_adapter_t* adapter)
{
    xid += (1<<24);

    printf("\nDHCP Discover sent.\n");

    dhcp_t packet;
    packet.op = 1;
    packet.htype = 1; // Type: for ethernet and 802.11 wireless clients, the hardware type is always 01
    packet.hlen = 6;
    packet.hops = 0;
    packet.xid = xid; // AFFExx
    packet.secs = htons(10); // TEST
    packet.flags = 0;
    for(uint8_t i = 0; i < 4; i++)
    {
        // packet.ciaddr[i] = 0;
        packet.yiaddr[i] = 0;
        // packet.siaddr[i] = 0;
        packet.giaddr[i] = 0;
    }

    // TEST
    packet.ciaddr[0] = IP_1;
    packet.ciaddr[1] = IP_2;
    packet.ciaddr[2] = IP_3;
    packet.ciaddr[3] = IP_4;

    packet.siaddr[0] = SIP_1;
    packet.siaddr[1] = SIP_2;
    packet.siaddr[2] = SIP_3;
    packet.siaddr[3] = SIP_4;

    for(uint8_t i = 0; i <   6; i++)  packet.chaddr[i] = adapter->MAC_address[i];
    for(uint8_t i = 6; i <  16; i++)  packet.chaddr[i] = 0;
    for(uint8_t i = 0; i <  64; i++)  packet.sname[i]  = 0;
    for(uint8_t i = 0; i < 128; i++)  packet.file[i]   = 0;

    // options
    packet.options[0]  =  99;  // MAGIC
    packet.options[1]  = 130;  // MAGIC
    packet.options[2]  =  83;  // MAGIC
    packet.options[3]  =  99;  // MAGIC
    for(uint16_t i = 4; i < 340; i++)
        packet.options[i] = 255; // end

    packet.options[4]  =   1;  // SUBNET
    packet.options[5]  =   4;  // Length
    packet.options[6]  = 255;  //
    packet.options[7]  = 255;  //
    packet.options[8]  = 255;  //
    packet.options[9]  =   0;  //

    packet.options[10]  = 53;  // MESSAGE TYPE
    packet.options[11]  =  1;  // Length
    packet.options[12]  =  8;  // INFORM

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

    packet.options[23] =  61;  // Client Identifier - hardware type and client hardware address
    packet.options[24] =   7;  // Length
    packet.options[25] =   1;  // Type: for ethernet and 802.11 wireless clients, the hardware type is always 01
    for(uint8_t i = 0; i < 6; i++)
        packet.options[26+i] = adapter->MAC_address[i];

    uint8_t srcIP[4] = {0,0,0,0};
    uint8_t destIP[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    UDPSend(adapter, &packet, sizeof(dhcp_t), 68, srcIP, 67, destIP);
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
