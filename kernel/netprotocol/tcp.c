/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "video/console.h"
#include "kheap.h"
#include "util.h"
#include "ipv4.h"
#include "tcp.h"

static void tcpDebug(tcpPacket_t* tcp)
{
  textColor(0x0E);
  printf("src port: %u  dest port: %u ", ntohs(tcp->sourcePort), ntohs(tcp->destPort));
  // printf("seq: %X  ack: %X\n", ntohl(tcp->sequenceNumber), ntohl(tcp->acknowledgmentNumber));
  textColor(0x0A);
  printf("URG: %u ACK: %u PSH: %u RST: %u SYN: %u FIN: %u\n", tcp->URG, tcp->ACK, tcp->PSH, tcp->RST, tcp->SYN, tcp->FIN);
  /*
  printf("window: %u  ", ntohs(tcp->window));
  printf("checksum: %x  urgent ptr: %X\n", ntohs(tcp->checksum), ntohs(tcp->urgentPointer));
  */
  textColor(0x0F);
}

// Binds the connection to a local portnumber and IP address.
void tcpBind(network_adapter_t* adapter, uint16_t srcPort, uint16_t destPort, uint8_t destIP[4])
{
    // TODO
}

void tcpConnect(network_adapter_t* adapter, uint16_t srcPort, uint16_t destPort, uint8_t destIP[4])
{
    adapter->TCP_PrevState = adapter->TCP_CurrState;

    if (adapter->TCP_PrevState == CLOSED || adapter->TCP_PrevState == LISTEN)
    {
        tcpSend(adapter, 0, 0, htons(srcPort), adapter->IP_address, htons(destPort), destIP, SYN_FLAG, 0 /*seqNumber*/ , 0 /*ackNumber*/);
        adapter->TCP_CurrState = SYN_SENT;
    }
}

void tcpClose(network_adapter_t* adapter, uint16_t srcPort, uint16_t destPort, uint8_t destIP[4])
{
    adapter->TCP_PrevState = adapter->TCP_CurrState;

    if (adapter->TCP_PrevState == ESTABLISHED || adapter->TCP_PrevState == SYN_RECEIVED)
    {
        tcpSend(adapter, 0, 0, htons(srcPort), adapter->IP_address, htons(destPort), destIP, FIN_FLAG, 0 /*seqNumber*/ , 0 /*ackNumber*/);
        adapter->TCP_CurrState = FIN_WAIT_1;
    }
    else if (adapter->TCP_PrevState == CLOSE_WAIT)
    {
        tcpSend(adapter, 0, 0, htons(srcPort), adapter->IP_address, htons(destPort), destIP, FIN_FLAG, 0 /*seqNumber*/ , 0 /*ackNumber*/);
        adapter->TCP_CurrState = LAST_ACK;
    }

    else if (adapter->TCP_PrevState == SYN_SENT || adapter->TCP_PrevState == LISTEN)
    {
        // no send action
        adapter->TCP_CurrState = CLOSED;
    }
}

void tcpListen(network_adapter_t* adapter)
{
    adapter->TCP_PrevState = adapter->TCP_CurrState;
    adapter->TCP_CurrState = LISTEN;

    // TODO: more action needed?
}

void tcpReceive(network_adapter_t* adapter, tcpPacket_t* tcp, uint8_t transmittingIP[4])
{   
    tcpDebug(tcp);
    textColor(0x0D);
    switch (adapter->TCP_CurrState) // later: prev. state
    {
        char tcpStateString[20];

    case 0: 
        strcpy(tcpStateString, "CLOSED");
        printf("TCP prev. state: %s\n", tcpStateString);
        break;
    case 1: 
        strcpy(tcpStateString, "LISTEN");
        printf("TCP prev. state: %s\n", tcpStateString);
        break;
    case 2: 
        strcpy(tcpStateString, "SYN_SENT");
        printf("TCP prev. state: %s\n", tcpStateString);
        break;
    case 3: 
        strcpy(tcpStateString, "SYN_RECEIVED");
        printf("TCP prev. state: %s\n", tcpStateString);
        break;
    case 4: 
        strcpy(tcpStateString, "ESTABLISHED");
        printf("TCP prev. state: %s\n", tcpStateString);
        break;            
    case 5: 
        strcpy(tcpStateString, "FIN_WAIT_1");
        printf("TCP prev. state: %s\n", tcpStateString);
        break;
    case 6: 
        strcpy(tcpStateString, "FIN_WAIT_2");
        printf("TCP prev. state: %s\n", tcpStateString);
        break;
    case 7: 
        strcpy(tcpStateString, "CLOSING");
        printf("TCP prev. state: %s\n", tcpStateString);
        break;
    case 8: 
        strcpy(tcpStateString, "CLOSE_WAIT");
        printf("TCP prev. state: %s\n", tcpStateString);
        break;
    case 9: 
        strcpy(tcpStateString, "LAST_ACK");
        printf("TCP prev. state: %s\n", tcpStateString);
        break;
    case 10: 
        strcpy(tcpStateString, "TIME_WAIT");
        printf("TCP prev. state: %s\n", tcpStateString);
        break;
        textColor(0x0F);
    }// switch

    // handshake: http://upload.wikimedia.org/wikipedia/commons/9/98/Tcp-handshake.svg

    if (tcp->SYN && !tcp->ACK) // SYN
    {
        adapter->TCP_PrevState = adapter->TCP_CurrState;
        if (adapter->TCP_CurrState == CLOSED)
        {   
            printf("TCP set from CLOSED to LISTEN.\n");
            tcpListen(adapter);
        }
        else if (adapter->TCP_CurrState == LISTEN)
        {
            tcpSend(adapter, 0, 0, htons(tcp->destPort), adapter->IP_address, htons(tcp->sourcePort), transmittingIP, SYN_ACK_FLAG, 0 /*seqNumber*/ , tcp->sequenceNumber+htonl(1) /*ackNumber*/);
            adapter->TCP_CurrState = SYN_RECEIVED;
        }
        else if (adapter->TCP_CurrState == SYN_SENT)
        {
            tcpSend(adapter, 0, 0, htons(tcp->destPort), adapter->IP_address, htons(tcp->sourcePort), transmittingIP, SYN_ACK_FLAG, 0 /*seqNumber*/ , tcp->sequenceNumber+htonl(1) /*ackNumber*/);
            adapter->TCP_CurrState = SYN_RECEIVED;
        }
    }
    if (tcp->SYN && tcp->ACK)  // SYN ACK
    {
        adapter->TCP_PrevState = adapter->TCP_CurrState;

        if (adapter->TCP_CurrState == SYN_SENT)
        tcpSend(adapter, 0, 0, tcp->destPort, adapter->IP_address, tcp->sourcePort, transmittingIP, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, tcp->sequenceNumber+htonl(1) /*ackNumber*/);
        adapter->TCP_CurrState = ESTABLISHED;
    }
    if (!tcp->SYN && !tcp->FIN && tcp->ACK) // ACK
    {
        // no send action
        adapter->TCP_PrevState = adapter->TCP_CurrState;

        if (adapter->TCP_CurrState == SYN_RECEIVED)
        {
            adapter->TCP_CurrState = ESTABLISHED;
        }
        else if (adapter->TCP_CurrState == LAST_ACK)
        {
            adapter->TCP_CurrState = CLOSED;
        }
        else if (adapter->TCP_CurrState == FIN_WAIT_1)
        {
            adapter->TCP_CurrState = FIN_WAIT_2;
        }
        else if (adapter->TCP_CurrState == CLOSING)
        {
            adapter->TCP_CurrState = TIME_WAIT;
        }
    }
    if (tcp->FIN && !tcp->ACK) // FIN
    {
        adapter->TCP_PrevState = adapter->TCP_CurrState;

        if (adapter->TCP_CurrState == ESTABLISHED)
        {
            tcpSend(adapter, 0, 0, tcp->destPort, adapter->IP_address, tcp->sourcePort, transmittingIP, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, tcp->sequenceNumber+htonl(1) /*ackNumber*/);
            adapter->TCP_CurrState = CLOSE_WAIT;
        }
        else if (adapter->TCP_CurrState == FIN_WAIT_2)
        {
            tcpSend(adapter, 0, 0, tcp->destPort, adapter->IP_address, tcp->sourcePort, transmittingIP, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, tcp->sequenceNumber+htonl(1) /*ackNumber*/);
            adapter->TCP_CurrState = TIME_WAIT;
        }
        else if (adapter->TCP_CurrState == FIN_WAIT_1)
        {
            tcpSend(adapter, 0, 0, tcp->destPort, adapter->IP_address, tcp->sourcePort, transmittingIP, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, tcp->sequenceNumber+htonl(1) /*ackNumber*/);
            adapter->TCP_CurrState = CLOSING;
        }
    }
    if (tcp->FIN && tcp->ACK) // FIN ACK
    {
        adapter->TCP_PrevState = adapter->TCP_CurrState;

        if (adapter->TCP_CurrState == FIN_WAIT_1)
        {
            tcpSend(adapter, 0, 0, tcp->destPort, adapter->IP_address, tcp->sourcePort, transmittingIP, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, tcp->sequenceNumber+htonl(1) /*ackNumber*/);
            adapter->TCP_CurrState = TIME_WAIT;
        }
    }
    if (tcp->RST) // RST
    {
        adapter->TCP_PrevState = adapter->TCP_CurrState;

        if (adapter->TCP_CurrState == SYN_RECEIVED)
        {
            // no send action
            adapter->TCP_CurrState = LISTEN;
        }
    }

    textColor(0x0D);
    switch (adapter->TCP_CurrState)
    {
        char tcpStateString[20];

    case 0: 
        strcpy(tcpStateString, "CLOSED");
        printf("TCP curr. state: %s\n", tcpStateString);
        break;
    case 1: 
        strcpy(tcpStateString, "LISTEN");
        printf("TCP curr. state: %s\n", tcpStateString);
        break;
    case 2: 
        strcpy(tcpStateString, "SYN_SENT");
        printf("TCP curr. state: %s\n", tcpStateString);
        break;
    case 3: 
        strcpy(tcpStateString, "SYN_RECEIVED");
        printf("TCP curr. state: %s\n", tcpStateString);
        break;
    case 4: 
        strcpy(tcpStateString, "ESTABLISHED");
        printf("TCP curr. state: %s\n", tcpStateString);
        break;            
    case 5: 
        strcpy(tcpStateString, "FIN_WAIT_1");
        printf("TCP curr. state: %s\n", tcpStateString);
        break;
    case 6: 
        strcpy(tcpStateString, "FIN_WAIT_2");
        printf("TCP curr. state: %s\n", tcpStateString);
        break;
    case 7: 
        strcpy(tcpStateString, "CLOSING");
        printf("TCP curr. state: %s\n", tcpStateString);
        break;
    case 8: 
        strcpy(tcpStateString, "CLOSE_WAIT");
        printf("TCP curr. state: %s\n", tcpStateString);
        break;
    case 9: 
        strcpy(tcpStateString, "LAST_ACK");
        printf("TCP curr. state: %s\n", tcpStateString);
        break;
    case 10: 
        strcpy(tcpStateString, "TIME_WAIT");
        printf("TCP curr. state: %s\n", tcpStateString);
        break;
        textColor(0x0F);
    }// switch
}

void tcpSend(network_adapter_t* adapter, void* data, uint32_t length, uint16_t srcPort, uint8_t srcIP[4], uint16_t destPort, uint8_t destIP[4], tcpFlags flags, uint32_t seqNumber, uint32_t ackNumber)
{
    tcpPacket_t* packet = malloc(sizeof(tcpPacket_t)+length, 0, "TCP packet");
    memcpy(packet+1, data, length);

    packet->sourcePort  = htons(srcPort);
    packet->destPort    = htons(destPort);
    packet->sequenceNumber = seqNumber;
    packet->acknowledgmentNumber = ackNumber;
    packet->dataOffset = sizeof(tcpPacket_t)>>2 ; // header length as number of DWORDS
    packet->reserved = 0;
    switch (flags)
    {
    case SYN_FLAG:
        packet->CWR = 0;
        packet->ECN = 0;
        packet->URG = 0;
        packet->ACK = 0;
        packet->PSH = 0;
        packet->RST = 0;
        packet->SYN = 1; // SYN
        packet->FIN = 0;
        break;
    case SYN_ACK_FLAG:
        packet->CWR = 0;
        packet->ECN = 0;
        packet->URG = 0;
        packet->ACK = 1; // ACK
        packet->PSH = 0;
        packet->RST = 0;
        packet->SYN = 1; // SYN
        packet->FIN = 0;
        break;
    case ACK_FLAG:
        packet->CWR = 0;
        packet->ECN = 0;
        packet->URG = 0;
        packet->ACK = 1; // ACK
        packet->PSH = 0;
        packet->RST = 0;
        packet->SYN = 0;
        packet->FIN = 0;
        break;
    case FIN_FLAG:
        packet->CWR = 0;
        packet->ECN = 0;
        packet->URG = 0;
        packet->ACK = 0;
        packet->PSH = 0;
        packet->RST = 0;
        packet->SYN = 0;
        packet->FIN = 1; // FIN
        break;
    case FIN_ACK_FLAG:
        packet->CWR = 0;
        packet->ECN = 0;
        packet->URG = 0;
        packet->ACK = 1; // ACK
        packet->PSH = 0;
        packet->RST = 0;
        packet->SYN = 0;
        packet->FIN = 1; // FIN
        break;
    case RST_FLAG:
        packet->CWR = 0;
        packet->ECN = 0;
        packet->URG = 0;
        packet->ACK = 0;
        packet->PSH = 0;
        packet->RST = 1; // RST
        packet->SYN = 0;
        packet->FIN = 0;
        break;
    }

    packet->window = 65535; // TODO: Clarify
    packet->checksum = 0; // for checksum calculation
    packet->checksum = htons(udptcpCalculateChecksum((void*)packet, length + sizeof(tcpPacket_t), srcIP, destIP, 6));

    ipv4_send(adapter, packet, length + sizeof(tcpPacket_t), destIP, 6);
    free(packet);
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
