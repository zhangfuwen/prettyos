/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "tcp.h"
#include "video/console.h"
#include "kheap.h"
#include "util.h"
#include "timer.h"
#include "network/netutils.h"
#include "ipv4.h"
#include "list.h"
#include "timer.h"


static listHead_t* tcpConnections = 0;



static uint16_t getFreeSocket();

static tcpConnection_t* findConnection(uint8_t IP[4], uint16_t port, network_adapter_t* adapter)
{
    if(tcpConnections == 0)
        return(0);

    for(element_t* e = tcpConnections->head; e != 0; e = e->next)
    {
        tcpConnection_t* connection = e->data;
        //if(connection->adapter == adapter && connection->remoteSocket.port == port && memcmp(connection->remoteSocket.IP, IP, 4) == 0) /// HACK
            return(connection);
    }

    return(0);
}

void tcp_showConnections()
{
    if(tcpConnections == 0)
        return;
    
    for(element_t* e = tcpConnections->head; e != 0; e = e->next)
    {
        tcpConnection_t* connection = e->data;
        printf("IP: %I  srcPrt: %u  destPort: %u\n", connection->adapter->IP, connection->localSocket.port, connection->remoteSocket.port);
    }
}

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

static const char* const tcpStates[] =
{
    "CLOSED", "LISTEN", "SYN_SENT", "SYN_RECEIVED", "ESTABLISHED", "FIN_WAIT_1", "FIN_WAIT_2", "CLOSING", "CLOSE_WAIT", "LAST_ACK", "TIME_WAIT"
};

static void tcpShowConnectionStatus(tcpConnection_t* connection)
{
    textColor(0x0D);
    printf("TCP curr. state: %s  connection: %X src port: %u\n", tcpStates[connection->TCP_CurrState], connection, connection->localSocket.port);
    textColor(0x0F);
}

tcpConnection_t* tcp_createConnection()
{
    if(tcpConnections == 0)
        tcpConnections = list_Create();

    tcpConnection_t* connection = malloc(sizeof(tcpConnection_t), 0, "tcp connection");
    connection->TCP_PrevState = CLOSED;
    connection->TCP_CurrState = CLOSED;
    srand(timer_getMilliseconds());
    connection->tcb.SND_ISS = rand();
    connection->tcb.SND_UNA = connection->tcb.SND_ISS;
    connection->tcb.SND_NXT = connection->tcb.SND_ISS + 1;
    list_Append(tcpConnections, connection);

    textColor(0x0A);
    printf("\nTCP connection created (CLOSED): %X\n", connection);
    textColor(0x0F);

    return(connection);
}

void tcp_deleteConnection(tcpConnection_t* connection)
{    
    connection->TCP_PrevState = connection->TCP_CurrState;
    connection->TCP_CurrState = CLOSED;
    textColor(0x0C);
    printf("\nTCP connection deleted: %X\n", connection);
    textColor(0x0F);
    list_Delete(tcpConnections, connection);
    free(connection);
    connection = 0; // TEST
}

void tcp_bind(tcpConnection_t* connection, struct network_adapter* adapter)
{
    // open TCP Server with State "LISTEN"
    memcpy(connection->localSocket.IP, adapter->IP, 4);
    connection->localSocket.port  = 0;
    connection->remoteSocket.port = 0;
    connection->TCP_PrevState = connection->TCP_CurrState;
    connection->TCP_CurrState = LISTEN;
    connection->adapter = adapter;
    
    tcpShowConnectionStatus(connection);
}

void tcp_connect(tcpConnection_t* connection) // ==> SYN-SENT
{
    connection->TCP_PrevState = connection->TCP_CurrState;
    connection->localSocket.port = getFreeSocket();

    if (connection->TCP_PrevState == CLOSED || connection->TCP_PrevState == LISTEN || connection->TCP_PrevState == TIME_WAIT)
    {
        tcp_send(connection, 0, 0, SYN_FLAG, connection->tcb.SND_ISS /*seqNumber*/ , 0 /*ackNumber*/);
        connection->TCP_CurrState = SYN_SENT;
        
        tcpShowConnectionStatus(connection); 
    }
}



void tcp_close(tcpConnection_t* connection)
{
    connection->TCP_PrevState = connection->TCP_CurrState;

    if (connection->TCP_PrevState == ESTABLISHED || connection->TCP_PrevState == SYN_RECEIVED)
    {
        tcp_send(connection, 0, 0, FIN_FLAG, 0 /*seqNumber*/ , 0 /*ackNumber*/);
        connection->TCP_CurrState = FIN_WAIT_1;
    }
    else if (connection->TCP_PrevState == CLOSE_WAIT)
    {
        tcp_send(connection, 0, 0, FIN_FLAG, 0 /*seqNumber*/ , 0 /*ackNumber*/);
        connection->TCP_CurrState = LAST_ACK;
    }
    else if (connection->TCP_PrevState == SYN_SENT || connection->TCP_PrevState == LISTEN)
    {
        // no send action
        connection->TCP_CurrState = CLOSED;
    }
}



void tcp_receive(network_adapter_t* adapter, tcpPacket_t* tcp, uint8_t transmittingIP[4], size_t length)
{
    tcpDebug(tcp);

    tcpConnection_t* connection = findConnection(transmittingIP, tcp->sourcePort, adapter);

    if(connection == 0)
    {
        textColor(0x0C);
        printf("\nTCP packet received that belongs to no TCP connection.");
        textColor(0x0F);
        return;
    }

    textColor(0x0D);
    printf("TCP prev. state: %s  connection: %X\n", tcpStates[connection->TCP_CurrState], connection); // later: prev. state
    textColor(0x0F);

    if (tcp->SYN && !tcp->ACK) // SYN
    {
        connection->TCP_PrevState = connection->TCP_CurrState;
        connection->remoteSocket.port = ntohs(tcp->sourcePort);
        connection->localSocket.port = ntohs(tcp->destPort);
        memcpy(connection->remoteSocket.IP, transmittingIP, 4);
        switch(connection->TCP_CurrState)
        {
            case CLOSED: 
            case TIME_WAIT: // HACK, TODO: use timeout (TIME_WAIT --> CLOSED)
                printf("TCP connection %X set from CLOSED to LISTEN.\n", connection);
                connection->TCP_CurrState = LISTEN;
                break;
            case LISTEN:
                tcp_send(connection, 0, 0, SYN_ACK_FLAG, 0 /*seqNumber*/ , htonl(ntohl(tcp->sequenceNumber)+1) /*ackNumber*/);
                connection->TCP_CurrState = SYN_RECEIVED;
                break;
            case SYN_SENT:
                tcp_send(connection, 0, 0, SYN_ACK_FLAG, 0 /*seqNumber*/ , htonl(ntohl(tcp->sequenceNumber)+1) /*ackNumber*/);
                connection->TCP_CurrState = SYN_RECEIVED;
                break;
            default:
                break;
        }
    }
    else if (tcp->SYN && tcp->ACK)  // SYN ACK
    {
        connection->TCP_PrevState = connection->TCP_CurrState;

        if (connection->TCP_CurrState == SYN_SENT)
        {
            connection->tcb.SND_NXT = tcp->acknowledgmentNumber;            // HACK for keyboard.c       
            connection->tcb.SND_UNA = htonl(ntohl(tcp->sequenceNumber)+1);  // HACK for keyboard.c      
        }
        
        tcp_send(connection, 0, 0, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, htonl(ntohl(tcp->sequenceNumber)+1) /*ackNumber*/);
        connection->TCP_CurrState = ESTABLISHED;
    }
    else if (!tcp->SYN && !tcp->FIN && tcp->ACK) // ACK
    {
        connection->TCP_PrevState = connection->TCP_CurrState;

        switch(connection->TCP_CurrState)
        {
            case ESTABLISHED: // ESTABLISHED --> DATA TRANSFER
            {
                uint32_t tcpDataLength = -4 /* frame ? */ + length - (tcp->dataOffset << 2);
                printf("data:");
                textColor(0x0A);
                for (uint16_t i=0; i<tcpDataLength; i++)
                {
                    printf("%c", ((uint8_t*)(tcp+1))[i]);
                }
                putch('\n');
                textColor(0x0F);
                tcp_send(connection, 0, 0, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, htonl(ntohl(tcp->sequenceNumber)+tcpDataLength) /*ackNumber*/);
                break;
            }
            // no send action
            case SYN_RECEIVED:
                connection->TCP_CurrState = ESTABLISHED;
                break;
            case LAST_ACK:
                connection->TCP_CurrState = CLOSED;
                break;
            case FIN_WAIT_1:
                connection->TCP_CurrState = FIN_WAIT_2;
                break;
            case CLOSING:
                connection->TCP_CurrState = TIME_WAIT;
                /// TEST
                delay(100000);
                tcp_deleteConnection(connection);
                /// TEST
                break;
            default:
                break;
        }
    }
    else if (tcp->FIN && !tcp->ACK) // FIN
    {
        connection->TCP_PrevState = connection->TCP_CurrState;

        switch(connection->TCP_CurrState)
        {
            case ESTABLISHED:
                tcp_send(connection, 0, 0, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, htonl(ntohl(tcp->sequenceNumber)+1) /*ackNumber*/);
                connection->TCP_CurrState = CLOSE_WAIT;
                break;
            case FIN_WAIT_2:
                tcp_send(connection, 0, 0, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, htonl(ntohl(tcp->sequenceNumber)+1) /*ackNumber*/);
                connection->TCP_CurrState = TIME_WAIT;                
                /// TEST
                delay(100000);
                tcp_deleteConnection(connection);
                /// TEST
                break;
            case FIN_WAIT_1:
                tcp_send(connection, 0, 0, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, htonl(ntohl(tcp->sequenceNumber)+1) /*ackNumber*/);
                connection->TCP_CurrState = CLOSING;
                break;
            default:
                break;
        }
    }
    else if (tcp->FIN && tcp->ACK) // FIN ACK
    {
        connection->TCP_PrevState = connection->TCP_CurrState;

        if (connection->TCP_CurrState == FIN_WAIT_1)
        {
            tcp_send(connection, 0, 0, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, htonl(ntohl(tcp->sequenceNumber)+1) /*ackNumber*/);
            connection->TCP_CurrState = TIME_WAIT;
            /// TEST
            delay(100000);
            tcp_deleteConnection(connection);
            /// TEST
        }

        // HACK due to observations in wireshark with telnet:
        // w/o conditions
        {
            tcp_send(connection, 0, 0, ACK_FLAG, tcp->acknowledgmentNumber /*seqNumber*/, htonl(ntohl(tcp->sequenceNumber)+1) /*ackNumber*/);
            connection->TCP_CurrState = TIME_WAIT;
            /// TEST
            delay(100000);
            tcp_deleteConnection(connection);
            /// TEST
        }
    }
    if (tcp->RST) // RST
    {
        connection->TCP_PrevState = connection->TCP_CurrState;

        if (connection->TCP_CurrState == SYN_RECEIVED)
        {
            // no send action
            connection->TCP_CurrState = LISTEN;
        }
    }

    tcpShowConnectionStatus(connection);
}



void tcp_send(tcpConnection_t* connection, void* data, uint32_t length, tcpFlags flags, uint32_t seqNumber, uint32_t ackNumber)
{
    textColor(0x0B);
    printf("TCP sends at connection: %X src port: %u.\n", connection, connection->localSocket.port);
    textColor(0x0F);

    tcpPacket_t* packet = malloc(sizeof(tcpPacket_t)+length, 0, "TCP packet");
    memcpy(packet+1, data, length);

    packet->sourcePort = htons(connection->localSocket.port);
    packet->destPort   = htons(connection->remoteSocket.port);
    packet->sequenceNumber = seqNumber;
    packet->acknowledgmentNumber = ackNumber;
    packet->dataOffset = sizeof(tcpPacket_t)>>2; // header length as number of DWORDS
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
    packet->urgentPointer = 0; // TODO: Clarify

    packet->checksum = 0; // for checksum calculation
    
    packet->checksum = htons(udptcpCalculateChecksum((void*)packet, length + sizeof(tcpPacket_t), connection->localSocket.IP, connection->remoteSocket.IP, 6));

    ipv4_send(connection->adapter, packet, length + sizeof(tcpPacket_t), connection->remoteSocket.IP, 6);
    free(packet);
}

static uint16_t getFreeSocket()
{
    static uint16_t srcPort = 49152;
    return srcPort++;
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
