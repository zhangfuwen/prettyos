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

static void DHCP_AnalyzeOptions(uint8_t* opt);

void DHCP_AnalyzeServerMessage(dhcp_t* dhcp)
{
    printf("\nop: %u", dhcp->op);  
    printf(" htype: %u", dhcp->htype);  
    printf(" hlen: %u", dhcp->hlen);  
    printf(" hops: %u", dhcp->hops);
    printf(" xid: %X", htonl(dhcp->xid));
    printf(" secs: %u", htons(dhcp->secs));
    printf(" flags: %x", htons(dhcp->flags));
    printf("\ncIP: %u.%u.%u.%u", dhcp->ciaddr[0], dhcp->ciaddr[1], dhcp->ciaddr[2], dhcp->ciaddr[3]);
    printf(" yIP: %u.%u.%u.%u", dhcp->yiaddr[0], dhcp->yiaddr[1], dhcp->yiaddr[2], dhcp->yiaddr[3]);
    printf("\nsIP: %u.%u.%u.%u", dhcp->siaddr[0], dhcp->siaddr[1], dhcp->siaddr[2], dhcp->siaddr[3]);
    printf(" gIP: %u.%u.%u.%u", dhcp->giaddr[0], dhcp->giaddr[1], dhcp->giaddr[2], dhcp->giaddr[3]);
    printf("\nMAC: %y-%y-%y-%y-%y-%y", dhcp->chaddr[0], dhcp->chaddr[1], dhcp->chaddr[2], 
                                       dhcp->chaddr[3], dhcp->chaddr[4], dhcp->chaddr[5]);
    DHCP_AnalyzeOptions(dhcp->options);
}
static uint16_t showOptionsBytes(uint8_t* opt, uint16_t count)
{
    for (uint16_t i=0; i<opt[count+2]; i++)
    {
        if (opt[count+1]==12 || opt[count+1]==14 || 
            opt[count+1]==15 || opt[count+1]==17 || 
            opt[count+1]==18 || opt[count+1]==40 || 
            opt[count+1]==43
            )
        {
            printf("%c", opt[count+3+i]);
        }
        else
        {
            printf("%u ", opt[count+3+i]);
        }
    }
    return (count + 2 + opt[count+2]);
}

static void DHCP_AnalyzeOptions(uint8_t* opt)
{
    uint16_t count=0;
    
    // check for magic number 63h 82h 53h 63h
    if (opt[0] == 0x63 && opt[1] == 0x82 && opt[2] == 0x53 && opt[3] == 0x63)
        printf("\nMAGIC OK"); 
    else
        printf("\nMAGIC NOT OK"); 
    count=3;

    while (opt[count+1] != 0xFF) // no end token
    {
        switch (opt[count+1])
        {
        case 0:
            printf("\nPadding"); 
            count++;
            break;
        case 1:
            printf("\nSubnet Mask: "); 
            count = showOptionsBytes(opt, count);
            break;
        case 2:
            printf("Time Offset: ");
            break;
        case 3:
            printf("\nRouters: ");
            count = showOptionsBytes(opt, count);
            break;
        case 4:
            printf("\nTime Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 5:
            printf("\nName Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 6:
            printf("\nDNS IP: ");
            count = showOptionsBytes(opt, count);
            break;
        case 7:
            printf("\nLog Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 8:
            printf("\nQuote Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 9:
            printf("\nLPR Server: ");
            break;
        case 10:
            printf("\nImpress Server: ");
            break;
        case 11:
            printf("\nResource Location Server: ");
            count = showOptionsBytes(opt, count); 
            break;
        case 12:
            printf("\nHost Name: ");
            count = showOptionsBytes(opt, count); //ASCII
            break;
        case 13:
            printf("\nBoot File Size: ");
            count = showOptionsBytes(opt, count);
            break;
        case 14:
            printf("\nMerit Dump File: ");
            count = showOptionsBytes(opt, count);
            break;
        case 15:
            printf("\nDomain name: ");
            count = showOptionsBytes(opt, count); //ASCII
            break;
        case 16:
            printf("\nSwap Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 17:
            printf("\nRoot Path: ");
            count = showOptionsBytes(opt, count);
            break;
        case 18:
            printf("\nExtensions Path: ");
            count = showOptionsBytes(opt, count);
            break;
        case 19:
            printf("\nIP Forwarding enable/disable: ");
            count = showOptionsBytes(opt, count);
            break;
        case 20:
            printf("\nNon-local Source Routing enable/disable: ");
            count = showOptionsBytes(opt, count);
            break;
        case 21:
            printf("\nPolicy Filter: ");
            count = showOptionsBytes(opt, count);
            break;
        case 22:
            printf("\nMaximum Datagram Reassembly Size: ");
            count = showOptionsBytes(opt, count);
            break;
        case 23:
            printf("\nDefault IP Time-to-live: ");
            count = showOptionsBytes(opt, count);
            break;
        case 24:
            printf("\nPath MTU Aging Timeout: ");
            count = showOptionsBytes(opt, count);
            break;
        case 25:
            printf("\nPath MTU Plateau Table:");
            count = showOptionsBytes(opt, count);
            break;
        case 26:
            printf("\nInterface MTU: ");
            count = showOptionsBytes(opt, count);
            break;
        case 27:
            printf("\nAll Subnets are Local: ");
            count = showOptionsBytes(opt, count);
            break;
        case 28:
            printf("\nBroadcast Address: ");
            count = showOptionsBytes(opt, count);
            break;
        case 29:
            printf("\nPerform Mask Discovery: ");
            count = showOptionsBytes(opt, count);
            break;
        case 30:
            printf("\nMask supplier: ");
            count = showOptionsBytes(opt, count);
            break;
        case 31:
            printf("\nPerform router discovery: ");
            count = showOptionsBytes(opt, count);
            break;
        case 32:
            printf("\nRouter solicitation address: ");
            count = showOptionsBytes(opt, count);
            break;
        case 33:
            printf("\nStatic routing table: ");
            count = showOptionsBytes(opt, count);
            break;
        case 34:
            printf("\n	Trailer encapsulation: ");
            count = showOptionsBytes(opt, count);
            break;
        case 35:
            printf("\nARP cache timeout: ");
            count = showOptionsBytes(opt, count);
            break;
        case 36:
            printf("\nEthernet encapsulation: ");
            count = showOptionsBytes(opt, count);
            break;
        case 37:
            printf("\nDefault TCP TTL: ");
            count = showOptionsBytes(opt, count);
            break;
        case 38:
            printf("\nTCP keepalive interval: ");
            count = showOptionsBytes(opt, count);
            break;
        case 39:
            printf("\nTCP keepalive garbage: ");
            count = showOptionsBytes(opt, count);
            break;
        case 40:
            printf("\nNetwork Information Service Domain: ");
            count = showOptionsBytes(opt, count);
            break;
        case 41:
            printf("\nNetwork Information Servers: ");
            count = showOptionsBytes(opt, count);
            break;
        case 42:
            printf("\nNTP servers: ");
            count = showOptionsBytes(opt, count);
            break;
        case 43:
            printf("\nVendor specific info: ");
            count = showOptionsBytes(opt, count);
            break;
        case 44:
            printf("\nNetBIOS over TCP/IP Name Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 45:
            printf("\nNetBIOS over TCP/IP Datagram Distribution Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 46:
            printf("\nNetBIOS over TCP/IP Node Type: ");
            count = showOptionsBytes(opt, count);
            break;
        case 47:
            printf("\nNetBIOS over TCP/IP Scope: ");
            count = showOptionsBytes(opt, count);
            break;
        case 48:
            printf("\nX Window System Font Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 49:
            printf("\nX Window System Display Manager: ");
            count = showOptionsBytes(opt, count);
            break;
        case 50:
            printf("\nRequested IP Address: ");
            count = showOptionsBytes(opt, count);
            break;
        case 51:
            printf("\nIP address lease time: ");
            count = showOptionsBytes(opt, count);
            break;
        case 52:
            printf("\nOption overload: ");
            count = showOptionsBytes(opt, count);
            break;
        case 53:
            printf("\nMessage Type: ");
            count = showOptionsBytes(opt, count);
            break;
        case 54:
            printf("\nServer IP: ");
            count = showOptionsBytes(opt, count);
            break;
        case 55:
            printf("\nParameter request list: ");
            count = showOptionsBytes(opt, count);
            break;
        case 56:
            printf("\nMessage: ");
            count = showOptionsBytes(opt, count);
            break;
        case 57:
            printf("\nMaximum DHCP message size: ");
            count = showOptionsBytes(opt, count);
            break;
        case 58:
            printf("\nRenew time value: ");
            count = showOptionsBytes(opt, count);
            break;
        case 59:
            printf("\nRebinding time value: ");
            count = showOptionsBytes(opt, count);
            break;
        case 60:
            printf("\nClass-identifier: ");
            count = showOptionsBytes(opt, count);
            break;
        case 61:
            printf("\nClient-identifier: ");
            count = showOptionsBytes(opt, count);
            break;
        case 62:
            printf("\nNetWare/IP Domain Name: ");
            count = showOptionsBytes(opt, count);
            break;
        case 63:
            printf("\nNetWare/IP information: ");
            count = showOptionsBytes(opt, count);
            break;
        case 64:
            printf("\nNetwork Information Service Domain: ");
            count = showOptionsBytes(opt, count);
            break;
        case 65:
            printf("\nNetwork Information Service Servers:");
            count = showOptionsBytes(opt, count);
            break;
        case 66:
            printf("\nTFTP server name: ");
            count = showOptionsBytes(opt, count);
            break;
        case 67:
            printf("\nBootfile name: ");
            count = showOptionsBytes(opt, count);
            break;
        case 68:
            printf("\nMobile IP Home Agent: ");
            count = showOptionsBytes(opt, count);
            break;
        case 69:
            printf("\nSimple Mail Transport Protocol Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 70:
            printf("\nPost Office Protocol Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 71:
            printf("\nNetwork News Transport Protocol Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 72:
            printf("\nDefault World Wide Web Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 73:
            printf("\nDefault Finger Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 74:
            printf("\nDefault Internet Relay Chat Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 75:
            printf("\nStreetTalk Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 76:
            printf("\nStreetTalk Directory Assistance Server: ");
            count = showOptionsBytes(opt, count);
            break;
        case 77:
            printf("\nUser Class Information: ");
            count = showOptionsBytes(opt, count);
            break;
        case 78:
            printf("\nSLP Directory Agent: ");
            count = showOptionsBytes(opt, count);
            break;
        case 79:
            printf("\nSLP Service Scope: ");
            count = showOptionsBytes(opt, count);
            break;
        case 80:
            printf("\nRapid Commit: ");
            count = showOptionsBytes(opt, count);
            break;
        case 81:
            printf("\nFQDN, Fully Qualified Domain Name: ");
            count = showOptionsBytes(opt, count);
            break;
        case 82:
            printf("\nRelay Agent Information: ");
            count = showOptionsBytes(opt, count);
            break;
        case 83:
            printf("\nInternet Storage Name Service: ");
            count = showOptionsBytes(opt, count);
            break;
        case 84:
            printf("\n???: ");
            count = showOptionsBytes(opt, count);
            break;
        case 85:
            printf("\nNDS servers: ");
            count = showOptionsBytes(opt, count);
            break;
        case 86:
            printf("\nNDS tree name: ");
            count = showOptionsBytes(opt, count);
            break;
        case 87:
            printf("\nNDS context: ");
            count = showOptionsBytes(opt, count);
            break;
        case 88:
            printf("\nBCMCS Controller Domain Name list: ");
            count = showOptionsBytes(opt, count);
            break;
        case 89:
            printf("\nBCMCS Controller IPv4 address list: ");
            count = showOptionsBytes(opt, count);
            break;
        case 90:
            printf("\nAuthentication: ");
            count = showOptionsBytes(opt, count);
            break;
        case 91:
            printf("\nclient-last-transaction-time: ");
            count = showOptionsBytes(opt, count);
            break;
        case 92:
            printf("\nassociated-ip: ");
            count = showOptionsBytes(opt, count);
            break;
        case 93:
            printf("\nClient System Architecture Type: ");
            count = showOptionsBytes(opt, count);
            break;
        case 94:
            printf("\nClient Network Interface Identifier: ");
            count = showOptionsBytes(opt, count);
            break;
        case 95:
            printf("\nLDAP, Lightweight Directory Access Protocol: ");
            count = showOptionsBytes(opt, count);
            break;
        case 96:
            printf("\n???: ");
            count = showOptionsBytes(opt, count);
            break;
        case 97:
            printf("\nClient Machine Identifier: ");
            count = showOptionsBytes(opt, count);
            break;
        case 98:
            printf("\nOpen Group's User Authentication: ");
            count = showOptionsBytes(opt, count);
            break;
        case 99:
            printf("\nGEOCONF_CIVIC: ");
            count = showOptionsBytes(opt, count);
            break;
        case 100:
            printf("\nIEEE 1003.1 TZ String: ");
            count = showOptionsBytes(opt, count);
            break;
        default:
            printf("option number > 100");
            break;
        }//switch
    }//while
    printf("\nEND OF OPTIONS\n");    
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
