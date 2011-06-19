/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "dhcp.h"
#include "udp.h"
#include "video/console.h"
#include "network/netutils.h"


static uint32_t xid = 0x0000E0FF0A; // AFFE.... Transaction Code for Identification

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
    packet.flags = BROADCAST; // TEST: broadcast
    for(uint8_t i = 0; i < 4; i++)
    {
        packet.ciaddr[i] = 0;
        packet.yiaddr[i] = 0;
        packet.siaddr[i] = 0;
        packet.giaddr[i] = 0;
    }

    for(uint8_t i = 0; i <   6; i++)  packet.chaddr[i] = adapter->MAC[i];
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

    packet.options[11] =  50;  // Requested IP
    packet.options[12] =   4;  // Length
    packet.options[13] = RIP_1;
    packet.options[14] = RIP_2;
    packet.options[15] = RIP_3;
    packet.options[16] = RIP_4;

    packet.options[17]  = 55;  // Parameter request list
    packet.options[18]  =  4;  // Length
    packet.options[19]  =  1;  // SUBNET MASK
    packet.options[20] =   3;  // ROUTERS
    packet.options[21] =   6;  // DOMAIN NAME SERVER
    packet.options[22] =  15;  // DOMAIN NAME

    packet.options[23] =  12;  // Hostname
    packet.options[24] =   8;  // Length
    packet.options[25] =  80;  // P
    packet.options[26] = 114;  // r
    packet.options[27] = 101;  // e
    packet.options[28] = 116;  // t
    packet.options[29] = 116;  // t
    packet.options[30] = 121;  // y
    packet.options[31] =  79;  // O
    packet.options[32] =  83;  // S

    uint8_t srcIP[4] = {0,0,0,0};
    uint8_t destIP[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    UDPSend(adapter, &packet, sizeof(dhcp_t), 68, srcIP, 67, destIP);
}

void DHCP_Request(network_adapter_t* adapter, uint8_t requestedIP[4])
{
    xid += (1<<24);

    printf("\nDHCP Request sent.\n");

    dhcp_t packet;
    packet.op = 1;
    packet.htype = 1; // Type: for ethernet and 802.11 wireless clients, the hardware type is always 01
    packet.hlen = 6;
    packet.hops = 0;
    packet.xid = xid; // AFFExx
    packet.secs = htons(0);
    packet.flags = BROADCAST; // TEST: broadcast
    for(uint8_t i = 0; i < 4; i++)
    {
        packet.ciaddr[i] = 0;
        packet.yiaddr[i] = 0;
        packet.siaddr[i] = 0;
        packet.giaddr[i] = 0;
    }

    for(uint8_t i = 0; i <   6; i++)  packet.chaddr[i] = adapter->MAC[i];
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

    packet.options[4]  = 53;  // MESSAGE TYPE
    packet.options[5]  =  1;  // Length
    packet.options[6]  =  3;  // REQUEST

    packet.options[7]  = 55;  // Parameter request list
    packet.options[8]  =  4;  // Length
    packet.options[9]  =  1;  // SUBNET MASK
    packet.options[10] =  3;  // ROUTERS
    packet.options[11] =  6;  // DOMAIN NAME SERVER
    packet.options[12] = 15;  // DOMAIN NAME

    packet.options[13] = 61;  // Client Identifier - hardware type and client hardware address
    packet.options[14] =  7;  // Length
    packet.options[15] =  1;  // Type: for ethernet and 802.11 wireless clients, the hardware type is always 01
    for(uint8_t i = 0; i < 6; i++)
        packet.options[16+i] = adapter->MAC[i];

    packet.options[22] =  50;  // Requested IP
    packet.options[23] =   4;  // Length
    packet.options[24] = requestedIP[0];
    packet.options[25] = requestedIP[1];
    packet.options[26] = requestedIP[2];
    packet.options[27] = requestedIP[3];

    packet.options[28] =  12;  // Hostname
    packet.options[29] =   8;  // Length
    packet.options[30] =  80;  // P
    packet.options[31] = 114;  // r
    packet.options[32] = 101;  // e
    packet.options[33] = 116;  // t
    packet.options[34] = 116;  // t
    packet.options[35] = 121;  // y
    packet.options[36] =  79;  // O
    packet.options[37] =  83;  // S

    uint8_t srcIP[4] = {0,0,0,0};
    uint8_t destIP[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    UDPSend(adapter, &packet, sizeof(dhcp_t), 68, srcIP, 67, destIP);
}

void DHCP_Inform(network_adapter_t* adapter)
{
    xid += (1<<24);

    printf("\nDHCP Inform sent.\n");

    dhcp_t packet;
    packet.op = 1;
    packet.htype = 1; // Type: for ethernet and 802.11 wireless clients, the hardware type is always 01
    packet.hlen = 6;
    packet.hops = 0;
    packet.xid = xid; // AFFExx
    packet.secs = 0;
    packet.flags = BROADCAST; // TEST: broadcast
    for(uint8_t i = 0; i < 4; i++)
    {
        packet.ciaddr[i] = adapter->IP[i];
        packet.yiaddr[i] = 0;
        packet.siaddr[i] = 0;
        packet.giaddr[i] = 0;
    }

    for(uint8_t i = 0; i <   6; i++)  packet.chaddr[i] = adapter->MAC[i];
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
    packet.options[6]  = 255;
    packet.options[7]  = 255;
    packet.options[8]  = 255;
    packet.options[9]  =   0;

    packet.options[10]  = 53;  // MESSAGE TYPE
    packet.options[11]  =  1;  // Length
    packet.options[12]  =  8;  // INFORM

    packet.options[13]  = 55;  // Parameter Request List
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
        packet.options[26+i] = adapter->MAC[i];

    uint8_t srcIP[4];
    for(uint8_t i = 0; i < 4; i++)
    {
        srcIP[i] = adapter->IP[i];
    }

    uint8_t destIP[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    UDPSend(adapter, &packet, sizeof(dhcp_t), 68, srcIP, 67, destIP);
}

void DHCP_Release(network_adapter_t* adapter)
{
    xid += (1<<24);

    printf("\nDHCP Release sent.\n");

    dhcp_t packet;
    packet.op = 1;
    packet.htype = 1; // Type: for ethernet and 802.11 wireless clients, the hardware type is always 01
    packet.hlen = 6;
    packet.hops = 0;
    packet.xid = xid; // AFFExx
    packet.secs = htons(0);
    packet.flags = UNICAST ;
    for(uint8_t i = 0; i < 4; i++)
    {
        packet.ciaddr[i] = adapter->IP[i];
        packet.yiaddr[i] = 0;
        packet.siaddr[i] = 0;
        packet.giaddr[i] = 0;
    }

    for(uint8_t i = 0; i <   6; i++)  packet.chaddr[i] = adapter->MAC[i];
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

    packet.options[4]  = 53;  // MESSAGE TYPE
    packet.options[5]  =  1;  // Length
    packet.options[6]  =  7;  // RELEASE

    packet.options[7]  = 61;  // Client Identifier - hardware type and client hardware address
    packet.options[8]  =  7;  // Length
    packet.options[9]  =  1;  // Type: for ethernet and 802.11 wireless clients, the hardware type is always 01
    for(uint8_t i = 0; i < 6; i++)
        packet.options[10+i] = adapter->MAC[i];

    uint8_t srcIP[4];
    for(uint8_t i = 0; i < 4; i++)
    {
        srcIP[i] = adapter->IP[i];
    }

    uint8_t destIP[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    UDPSend(adapter, &packet, sizeof(dhcp_t), 68, srcIP, 67, destIP);
}

static void DHCP_AnalyzeOptions(network_adapter_t* adapter, uint8_t* opt);

static void useDHCP_IP(network_adapter_t* adapter, dhcp_t* dhcp)
{
    if (dhcp->yiaddr[0] || dhcp->yiaddr[1] || dhcp->yiaddr[2] || dhcp->yiaddr[3])
    {
        for(uint8_t i = 0; i < 4; i++)
            adapter->IP[i] = dhcp->yiaddr[i];
    }
}

void DHCP_AnalyzeServerMessage(network_adapter_t* adapter, dhcp_t* dhcp)
{
    /*
    printf("\nop: %u",    dhcp->op);
    printf(" htype: %u",  dhcp->htype);
    printf(" hlen: %u",   dhcp->hlen);
    printf(" hops: %u",   dhcp->hops);
    printf(" xid: %Xh",   ntohl(dhcp->xid));
    printf(" secs: %u",   ntohs(dhcp->secs));
    printf(" flags: %xh", ntohs(dhcp->flags));
    */
    printf("\ncIP: %I", dhcp->ciaddr);
    printf(" yIP: %I",  dhcp->yiaddr);
    printf("\nsIP: %I", dhcp->siaddr);
    printf(" gIP: %I",  dhcp->giaddr);
    printf("\nMAC: %M", dhcp->chaddr);

    DHCP_AnalyzeOptions(adapter, dhcp->options);
    switch(adapter->DHCP_State)
    {
        case OFFER:
            printf("\n >>> PrettyOS got a DHCP OFFER. <<<");
            if (dhcp->yiaddr[0] || dhcp->yiaddr[1] || dhcp->yiaddr[2] || dhcp->yiaddr[3])
            {
                DHCP_Request(adapter, dhcp->yiaddr);
            }
            else
            {
                adapter->IP[0] = IP_1;
                adapter->IP[1] = IP_2;
                adapter->IP[2] = IP_3;
                adapter->IP[3] = IP_4;
            }
            break;
        case ACK:
            printf("\n >>> PrettyOS got a DHCP ACK.   <<<");
            useDHCP_IP(adapter, dhcp);
            break;
        case NAK:
            printf("\n >>> DHCP was not successful (NAK). <<<");
            break;
        default:
            break;
    }
}

static uint16_t showOptionsBytes(network_adapter_t* adapter, uint8_t* opt, uint16_t count)
{
    uint32_t leaseTime=0;

    switch(opt[count+1])
    {
        case 12: case 14: case 15: case 17: case 18: case 40: case 43: // ASCII output
            for(uint16_t i=0; i<opt[count+2]; i++)
                printf("%c", opt[count+3+i]);
            break;
        case 51: // Lease time
            for(uint16_t i=0; i<opt[count+2]; i++)
            {
                printf("%u ", opt[count+3+i]);
                leaseTime += ((opt[count+3+i])<<(24-8*i));
            }
            printf("  %u hours", leaseTime/3600);
            break;
        case 53:
            for(uint16_t i=0; i<opt[count+2]; i++)
                printf("%u ", opt[count+3+i]);
            switch (opt[count+3])
            {
                case 1:
                    printf(" (DHCPDiscover)");
                    break;
                case 2:
                    printf(" (DHCPOffer)");
                    adapter->DHCP_State = OFFER;
                    break;
                case 3:
                    printf(" (DHCPRequest)");
                    break;
                case 4:
                    printf(" (DHCPDecline)");
                    break;
                case 5:
                    printf(" (DHCPAck)");
                    adapter->DHCP_State = ACK;
                    break;
                case 6:
                    printf(" (DHCPNak)");
                    adapter->DHCP_State = NAK;
                    break;
                case 7:
                    printf(" (DHCPRelease)");
                    break;
                case 8:
                    printf(" (DHCPInform)");
                    break;
            }
            break;
        default:
            for(uint16_t i=0; i<opt[count+2]; i++)
                printf("%u ", opt[count+3+i]);
            break;
    }
    return (count + 2 + opt[count+2]);
}

static void DHCP_AnalyzeOptions(network_adapter_t* adapter, uint8_t* opt)
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
            break;
        case 2:
            printf("Time Offset: ");
            break;
        case 3:
            printf("\nRouters: ");
            break;
        case 4:
            printf("\nTime Server: ");
            break;
        case 5:
            printf("\nName Server: ");
            break;
        case 6:
            printf("\nDNS IP: ");
            break;
        case 7:
            printf("\nLog Server: ");
            break;
        case 8:
            printf("\nQuote Server: ");
            break;
        case 9:
            printf("\nLPR Server: ");
            break;
        case 10:
            printf("\nImpress Server: ");
            break;
        case 11:
            printf("\nResource Location Server: ");
            break;
        case 12:
            printf("\nHost Name: ");
            break;
        case 13:
            printf("\nBoot File Size: ");
            break;
        case 14:
            printf("\nMerit Dump File: ");
            break;
        case 15:
            printf("\nDomain name: ");
            break;
        case 16:
            printf("\nSwap Server: ");
            break;
        case 17:
            printf("\nRoot Path: ");
            break;
        case 18:
            printf("\nExtensions Path: ");
            break;
        case 19:
            printf("\nIP Forwarding enable/disable: ");
            break;
        case 20:
            printf("\nNon-local Source Routing enable/disable: ");
            break;
        case 21:
            printf("\nPolicy Filter: ");
            break;
        case 22:
            printf("\nMaximum Datagram Reassembly Size: ");
            break;
        case 23:
            printf("\nDefault IP Time-to-live: ");
            break;
        case 24:
            printf("\nPath MTU Aging Timeout: ");
            break;
        case 25:
            printf("\nPath MTU Plateau Table:");
            break;
        case 26:
            printf("\nInterface MTU: ");
            break;
        case 27:
            printf("\nAll Subnets are Local: ");
            break;
        case 28:
            printf("\nBroadcast Address: ");
            break;
        case 29:
            printf("\nPerform Mask Discovery: ");
            break;
        case 30:
            printf("\nMask supplier: ");
            break;
        case 31:
            printf("\nPerform router discovery: ");
            break;
        case 32:
            printf("\nRouter solicitation address: ");
            break;
        case 33:
            printf("\nStatic routing table: ");
            break;
        case 34:
            printf("\nTrailer encapsulation: ");
            break;
        case 35:
            printf("\nARP cache timeout: ");
            break;
        case 36:
            printf("\nEthernet encapsulation: ");
            break;
        case 37:
            printf("\nDefault TCP TTL: ");
            break;
        case 38:
            printf("\nTCP keepalive interval: ");
            break;
        case 39:
            printf("\nTCP keepalive garbage: ");
            break;
        case 40:
            printf("\nNetwork Information Service Domain: ");
            break;
        case 41:
            printf("\nNetwork Information Servers: ");
            break;
        case 42:
            printf("\nNTP servers: ");
            break;
        case 43:
            printf("\nVendor specific info: ");
            break;
        case 44:
            printf("\nNetBIOS over TCP/IP Name Server: ");
            break;
        case 45:
            printf("\nNetBIOS over TCP/IP Datagram Distribution Server: ");
            break;
        case 46:
            printf("\nNetBIOS over TCP/IP Node Type: ");
            break;
        case 47:
            printf("\nNetBIOS over TCP/IP Scope: ");
            break;
        case 48:
            printf("\nX Window System Font Server: ");
            break;
        case 49:
            printf("\nX Window System Display Manager: ");
            break;
        case 50:
            printf("\nRequested IP Address: ");
            break;
        case 51:
            printf("\nIP address lease time: ");
            break;
        case 52:
            printf("\nOption overload: ");
            break;
        case 53:
            printf("\nMessage Type: ");
            break;
        case 54:
            printf("\nServer IP: ");
            break;
        case 55:
            printf("\nParameter request list: ");
            break;
        case 56:
            printf("\nMessage: ");
            break;
        case 57:
            printf("\nMaximum DHCP message size: ");
            break;
        case 58:
            printf("\nRenew time value: ");
            break;
        case 59:
            printf("\nRebinding time value: ");
            break;
        case 60:
            printf("\nClass-identifier: ");
            break;
        case 61:
            printf("\nClient-identifier: ");
            break;
        case 62:
            printf("\nNetWare/IP Domain Name: ");
            break;
        case 63:
            printf("\nNetWare/IP information: ");
            break;
        case 64:
            printf("\nNetwork Information Service Domain: ");
            break;
        case 65:
            printf("\nNetwork Information Service Servers:");
            break;
        case 66:
            printf("\nTFTP server name: ");
            break;
        case 67:
            printf("\nBootfile name: ");
            break;
        case 68:
            printf("\nMobile IP Home Agent: ");
            break;
        case 69:
            printf("\nSimple Mail Transport Protocol Server: ");
            break;
        case 70:
            printf("\nPost Office Protocol Server: ");
            break;
        case 71:
            printf("\nNetwork News Transport Protocol Server: ");
            break;
        case 72:
            printf("\nDefault World Wide Web Server: ");
            break;
        case 73:
            printf("\nDefault Finger Server: ");
            break;
        case 74:
            printf("\nDefault Internet Relay Chat Server: ");
            break;
        case 75:
            printf("\nStreetTalk Server: ");
            break;
        case 76:
            printf("\nStreetTalk Directory Assistance Server: ");
            break;
        case 77:
            printf("\nUser Class Information: ");
            break;
        case 78:
            printf("\nSLP Directory Agent: ");
            break;
        case 79:
            printf("\nSLP Service Scope: ");
            break;
        case 80:
            printf("\nRapid Commit: ");
            break;
        case 81:
            printf("\nFQDN, Fully Qualified Domain Name: ");
            break;
        case 82:
            printf("\nRelay Agent Information: ");
            break;
        case 83:
            printf("\nInternet Storage Name Service: ");
            break;
/*      case 84:
            printf("\n???: ");
            break;*/
        case 85:
            printf("\nNDS servers: ");
            break;
        case 86:
            printf("\nNDS tree name: ");
            break;
        case 87:
            printf("\nNDS context: ");
            break;
        case 88:
            printf("\nBCMCS Controller Domain Name list: ");
            break;
        case 89:
            printf("\nBCMCS Controller IPv4 address list: ");
            break;
        case 90:
            printf("\nAuthentication: ");
            break;
        case 91:
            printf("\nclient-last-transaction-time: ");
            break;
        case 92:
            printf("\nassociated-ip: ");
            break;
        case 93:
            printf("\nClient System Architecture Type: ");
            break;
        case 94:
            printf("\nClient Network Interface Identifier: ");
            break;
        case 95:
            printf("\nLDAP, Lightweight Directory Access Protocol: ");
            break;
/*      case 96:
            printf("\n???: ");
            break;*/
        case 97:
            printf("\nClient Machine Identifier: ");
            break;
        case 98:
            printf("\nOpen Group's User Authentication: ");
            break;
        case 99:
            printf("\nGEOCONF_CIVIC: ");
            break;
        case 100:
            printf("\nIEEE 1003.1 TZ String: ");
            break;
        default:
            printf("\nUnknown option: %u", opt[count+1]);
            break;
        } //switch
        if(opt[count+1] != 0)
            count = showOptionsBytes(adapter, opt, count);
    } //while
    printf("\nEND OF OPTIONS\n");
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
