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
  printf("\n");
  printf("TCP Header information:\n");
  textColor(0x0E);
  printf("+--------------+----------------+\n");
  printf("|      %u    |      %u          (source port, destination port)\n", ntohs(tcp->sourcePort), ntohs(tcp->destPort));
  printf("+-------------------------------+\n");
  printf("|      %Xh                  (sequence number)\n", ntohl(tcp->sequenceNumber));
  printf("+-------------------------------+\n");
  printf("|              %u                  (acknowledgmentnumber)\n", ntohl(tcp->acknowledgmentNumber));
  printf("+-------------------------------+\n");
  printf("| |%u%u%u%u%u%u|        %u           (flags: URG ACK PUSH RESET SYN FIN", /*tcp->CWR, tcp->ECN,*/ tcp->URG, tcp->ACK, tcp->PSH, tcp->RST, tcp->SYN, tcp->FIN, ntohs(tcp->window)); printf(", window)\n");
  printf("+--------------+----------------+\n");
  printf("|    %xh     |      %u           (checksum, urgent pointer)\n", ntohs(tcp->checksum), ntohs(tcp->urgentPointer));
  printf("+-------------------------------+\n");
}

void tcpReceive(network_adapter_t* adapter, tcpPacket_t* tcp, uint8_t transmittingIP[4])
{
    tcpDebug(tcp);

    // handshake: http://upload.wikimedia.org/wikipedia/commons/9/98/Tcp-handshake.svg

    if (tcp->SYN && !tcp->ACK) // SYN
    {
        tcpSend(adapter, 0, 0, htons(tcp->destPort), adapter->IP_address, htons(tcp->sourcePort), transmittingIP, SYN_ACK_FLAG, 0 /*seqNumber*/ , tcp->sequenceNumber+htonl(1) /*ackNumber*/);
        adapter->TCP_PrevState = adapter->TCP_CurrState;
        adapter->TCP_CurrState = SYN_RECEIVED;        
    }
    if (tcp->SYN && tcp->ACK)  // SYN ACK
    {
        tcpSend(adapter, 0, 0, tcp->destPort, adapter->IP_address, tcp->sourcePort, transmittingIP, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, tcp->sequenceNumber+htonl(1) /*ackNumber*/);
        adapter->TCP_PrevState = adapter->TCP_CurrState;
        adapter->TCP_CurrState = ESTABLISHED;
    }
    if (!tcp->SYN && tcp->ACK) // ACK
    {
        adapter->TCP_PrevState = adapter->TCP_CurrState;
        adapter->TCP_CurrState = ESTABLISHED;
    }
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
    packet->checksum = udptcpCalculateChecksum((void*)packet, length + sizeof(tcpPacket_t), srcIP, destIP);

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
