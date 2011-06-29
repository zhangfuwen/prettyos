/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "tcp.h"
#include "video/console.h"
#include "kheap.h"
#include "util.h"
#include "timer.h"
#include "ipv4.h"
#include "list.h"
#include "task.h"


static const char* const tcpStates[] =
{
    "CLOSED", "LISTEN", "SYN_SENT", "SYN_RECEIVED", "ESTABLISHED", "FIN_WAIT_1", "FIN_WAIT_2", "CLOSING", "CLOSE_WAIT", "LAST_ACK", "TIME_WAIT"
};

static listHead_t* tcpConnections = 0;

static bool IsSegmentAcceptable (tcpPacket_t* tcp, tcpConnection_t* connection, uint16_t tcpDatalength);
static uint16_t getFreeSocket();
static uint32_t getConnectionID();

tcpConnection_t* findConnectionID(uint32_t ID)
{
    if(tcpConnections == 0)
        return(0);

    for(element_t* e = tcpConnections->head; e != 0; e = e->next)
    {
        tcpConnection_t* connection = e->data;
        if(connection->ID == ID)
        {
            return(connection);
        }
    }

    return(0);
}

static tcpConnection_t* findConnectionListen(network_adapter_t* adapter)
{
    if(tcpConnections == 0)
        return(0);

    for(element_t* e = tcpConnections->head; e != 0; e = e->next)
    {
        tcpConnection_t* connection = e->data;
        if (connection->adapter == adapter && connection->TCP_CurrState == LISTEN)
        {
            return(connection);
        }
    }

    return(0);
}

tcpConnection_t* findConnection(IP_t IP, uint16_t port, network_adapter_t* adapter, bool established)
{
    if(tcpConnections == 0)
        return(0);

    for(element_t* e = tcpConnections->head; e != 0; e = e->next)
    {
        tcpConnection_t* connection = e->data;

        if (connection->adapter == adapter && connection->remoteSocket.port == port && connection->remoteSocket.IP.iIP == IP.iIP && (!established || connection->TCP_CurrState==ESTABLISHED))
            return(connection);
    }

    return(0);
}

void tcp_showConnections()
{
    if(tcpConnections == 0)
        return;

    textColor(TABLE_HEADING);
    printf("\nID\tIP\t\tSrc\tDest\tAddr\t\tState");
    printf("\n--------------------------------------------------------------------------------");
    textColor(TEXT);
    for(element_t* e = tcpConnections->head; e != 0; e = e->next)
    {
        tcpConnection_t* connection = e->data;
        printf("%u\t%I\t%u\t%u\t%X\t%s\n", connection->ID, connection->adapter->IP, connection->localSocket.port, connection->remoteSocket.port, connection, tcpStates[connection->TCP_CurrState]);
    }
    textColor(TABLE_HEADING);
    printf("--------------------------------------------------------------------------------\n");
    textColor(TEXT);
}

static void printFlag(uint8_t b, const char* s)
{
    textColor(b ? LIGHT_GREEN : GRAY);
    printf("%s ", s);
}

static void tcpDebug(tcpPacket_t* tcp)
{
    textColor(LIGHT_GRAY); printf(" src port: ");   textColor(IMPORTANT); printf("%u", ntohs(tcp->sourcePort));
    textColor(LIGHT_GRAY); printf("  dest port: "); textColor(IMPORTANT); printf("%u   ", ntohs(tcp->destPort));
    // printf("seq: %X  ack: %X\n", ntohl(tcp->sequenceNumber), ntohl(tcp->acknowledgmentNumber));
    printFlag(tcp->URG, "URG"); printFlag(tcp->ACK, "ACK"); printFlag(tcp->PSH, "PSH");
    printFlag(tcp->RST, "RST"); printFlag(tcp->SYN, "SYN"); printFlag(tcp->FIN, "FIN");
    textColor(TEXT);
    /*
    printf("window: %u  ", ntohs(tcp->window));
    printf("checksum: %x  urgent ptr: %X\n", ntohs(tcp->checksum), ntohs(tcp->urgentPointer));
    */
}

static void tcpShowConnectionStatus(tcpConnection_t* connection)
{
    textColor(TEXT);
    printf("\nTCP curr. state: ", tcpStates[connection->TCP_CurrState]);
    textColor(IMPORTANT);
    puts(tcpStates[connection->TCP_CurrState]);
    textColor(TEXT);
    printf("   conn. ID: %u   src port: %u\n", connection->ID, connection->localSocket.port);
}

tcpConnection_t* tcp_createConnection()
{
    if(tcpConnections == 0)
    {
        tcpConnections = list_Create();
    }

    tcpConnection_t* connection = malloc(sizeof(tcpConnection_t), 0, "tcp connection");
    connection->owner = (void*)currentTask;
    connection->ID = getConnectionID();
    connection->TCP_PrevState = CLOSED;
    connection->TCP_CurrState = CLOSED;

    list_Append(tcpConnections, connection);
    textColor(TEXT);
    printf("\nTCP conn. created, ID: %u\n", connection->ID);
    return(connection);
}

void tcp_deleteConnection(tcpConnection_t* connection)
{
    connection->TCP_PrevState = connection->TCP_CurrState;
    connection->TCP_CurrState = CLOSED;
    list_Delete(tcpConnections, connection);
    free(connection);

    textColor(TEXT);
    printf("\nTCP conn. deleted, ID: %u\n", connection->ID);
}

void tcp_bind(tcpConnection_t* connection, struct network_adapter* adapter) // passive open  ==> LISTEN
{
    connection->localSocket.IP.iIP = adapter->IP.iIP;
    connection->localSocket.port  = 0;
    connection->remoteSocket.port = 0;
    connection->TCP_PrevState = connection->TCP_CurrState;
    connection->TCP_CurrState = LISTEN;
    connection->adapter = adapter;

    tcpShowConnectionStatus(connection);
}

void tcp_connect(tcpConnection_t* connection) // active open  ==> SYN-SENT
{
    connection->TCP_PrevState = connection->TCP_CurrState;
    connection->localSocket.port = getFreeSocket();

    if (connection->TCP_PrevState == CLOSED || connection->TCP_PrevState == LISTEN || connection->TCP_PrevState == TIME_WAIT)
    {
        srand(timer_getMilliseconds());
        connection->tcb.SND.ISS = rand();
        connection->tcb.SEG.SEQ = connection->tcb.SND.ISS;
        connection->tcb.SEG.CTL = SYN_FLAG;
        connection->tcb.SEG.ACK = 0;

        connection->tcb.SND.UNA = connection->tcb.SND.ISS;
        connection->tcb.SND.NXT = connection->tcb.SND.ISS + 1; // CHECK!!!

        tcp_send(connection, 0, 0);
        connection->TCP_CurrState = SYN_SENT;

        tcpShowConnectionStatus(connection);
    }
}

void tcp_close(tcpConnection_t* connection)
{
    connection->TCP_PrevState = connection->TCP_CurrState;

    if (connection->TCP_PrevState == ESTABLISHED || connection->TCP_PrevState == SYN_RECEIVED)
    {
        connection->tcb.SEG.CTL = FIN_FLAG;
        connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
        connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
        connection->tcb.SND.NXT = connection->tcb.SEG.SEQ + 1; // CHECK!!!
        tcp_send(connection, 0, 0);
        connection->TCP_CurrState = FIN_WAIT_1;
    }
    else if (connection->TCP_PrevState == CLOSE_WAIT) // CHECK
    {
        connection->tcb.SEG.CTL = FIN_FLAG;
        connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
        connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
        connection->tcb.SND.NXT = connection->tcb.SEG.SEQ + 1; // CHECK!!!
        tcp_send(connection, 0, 0);

        connection->TCP_CurrState = LAST_ACK;
    }
    else if (connection->TCP_PrevState == SYN_SENT || connection->TCP_PrevState == LISTEN)
    {
        // no send action
        connection->TCP_CurrState = CLOSED;
        tcp_deleteConnection(connection);
    }
}

// This function has to be checked intensively!!!
// cf. http://www.systems.ethz.ch/education/past-courses/fs10/operating-systems-and-networks/material/TCP-Spec.pdf

void tcp_receive(network_adapter_t* adapter, tcpPacket_t* tcp, IP_t transmittingIP, size_t length)
{
    textColor(HEADLINE);
    printf("\nTCP:");
    tcpDebug(tcp);

    tcpConnection_t* connection;
    if (tcp->SYN && !tcp->ACK) // SYN
    {
        connection = findConnectionListen(adapter);
    }
    else
    {
        connection = findConnection(transmittingIP, ntohs(tcp->sourcePort), adapter, false);
    }

    if(connection == 0)
    {
        textColor(RED);
        printf("\nTCP packet received that does not belong to a TCP connection.");
        textColor(TEXT);
        return;
    }

    textColor(TEXT);
    printf("\nTCP prev. state: %s  conn. ID: %u\n", tcpStates[connection->TCP_CurrState], connection->ID); // later: prev. state

    if (tcp->SYN && !tcp->ACK) // SYN
    {
        connection->TCP_PrevState = connection->TCP_CurrState;
        connection->remoteSocket.port = ntohs(tcp->sourcePort);
        connection->localSocket.port = ntohs(tcp->destPort);
        connection->remoteSocket.IP.iIP = transmittingIP.iIP;

        switch(connection->TCP_CurrState)
        {
            case CLOSED:
            case TIME_WAIT: // HACK, TODO: use timeout (TIME_WAIT --> CLOSED)
                printf("TCP conn. ID %u set from CLOSED to LISTEN.\n", connection->ID);
                connection->TCP_CurrState = LISTEN;
                break;
            case LISTEN:
                srand(timer_getMilliseconds());
                connection->tcb.SND.ISS  = rand();
                connection->tcb.SEG.SEQ  = connection->tcb.SND.ISS;
                connection->tcb.SND.NXT  = connection->tcb.SEG.SEQ;
                connection->tcb.SND.UNA  = connection->tcb.SEG.SEQ;
                connection->tcb.RCV.NXT  = ntohl(tcp->sequenceNumber) + 1;
                connection->tcb.SEG.ACK  = connection->tcb.RCV.NXT;
                connection->tcb.SEG.CTL  = SYN_ACK_FLAG;
                tcp_send(connection, 0, 0);

                connection->TCP_CurrState = SYN_RECEIVED;
                break;
            case SYN_SENT:
                connection->tcb.SEG.SEQ = connection->tcb.SND.NXT; // CHECK
                connection->tcb.RCV.NXT = ntohl(tcp->sequenceNumber)+1;
                connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
                connection->tcb.SEG.CTL = SYN_ACK_FLAG;
                tcp_send(connection, 0, 0);

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
            connection->tcb.SEG.CTL = ACK_FLAG;
            connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
            connection->tcb.RCV.NXT = ntohl(tcp->sequenceNumber)+1;
            connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
            tcp_send(connection, 0, 0);

            connection->TCP_CurrState = ESTABLISHED;
            event_issue(connection->owner->eventQueue, EVENT_TCP_CONNECTED, &connection->ID, sizeof(connection->ID));
        }
        else
        {
            textColor(ERROR);
            printf("SYN ACK received, but not at SYN-SENT but at %s.", tcpStates[connection->TCP_CurrState]);
        }
    }
    else if (!tcp->SYN && !tcp->FIN && tcp->ACK) // ACK
    {
        connection->TCP_PrevState = connection->TCP_CurrState;

        switch(connection->TCP_CurrState)
        {
            case ESTABLISHED: // ESTABLISHED --> DATA TRANSFER
            {
                uint32_t tcpDataLength = -4 /* frame ? */ + length - (tcp->dataOffset << 2);
              
				textColor(ERROR);
				if (!IsSegmentAcceptable(tcp, connection, tcpDataLength))
				{
					printf("tcp packet is not acceptable!");
				}
				textColor(TEXT);

			  #ifdef _NETWORK_DATA_
                textColor(LIGHT_GRAY);
                printf("data:");
                textColor(DATA);
                for (uint16_t i=0; i<tcpDataLength; i++)
                {
                    printf("%c", ((uint8_t*)(tcp+1))[i]);
                }
                putch('\n');
                textColor(TEXT);
              #endif

                connection->tcb.SND.UNA = ntohl(tcp->acknowledgmentNumber);
                connection->tcb.SEG.LEN = tcpDataLength;
                connection->tcb.RCV.NXT = ntohl(tcp->sequenceNumber) + connection->tcb.SEG.LEN;
                connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
                connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
                connection->tcb.SEG.CTL = ACK_FLAG;
                tcp_send(connection, 0, 0);

                // Issue event
                struct
                {
                    tcpReceivedEventHeader_t header;
                    char buffer[tcpDataLength];
                } __attribute__((packed)) event;

                event.header.connection = connection->ID;
                event.header.length = tcpDataLength;
                memcpy(event.buffer, (void*)(tcp+1), tcpDataLength);
                event_issue(connection->owner->eventQueue, EVENT_TCP_RECEIVED, &event, sizeof(tcpReceivedEventHeader_t)+tcpDataLength);
                break;
            }
            // no send action
            case SYN_RECEIVED:
                connection->TCP_CurrState = ESTABLISHED;
                event_issue(connection->owner->eventQueue, EVENT_TCP_CONNECTED, &connection->ID, sizeof(connection->ID));
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
    else if (tcp->FIN && !tcp->ACK) // FIN  // CODE TO BE CHANGED TO RFC 793, cf. rev. 1001
    {
        connection->TCP_PrevState = connection->TCP_CurrState;

        switch(connection->TCP_CurrState)
        {
            case ESTABLISHED:
                connection->tcb.SEG.CTL = ACK_FLAG;
				connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
				connection->tcb.RCV.NXT = ntohl(tcp->sequenceNumber)+1;
				connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
				tcp_send(connection, 0, 0);
								
                connection->TCP_CurrState = CLOSE_WAIT;
                break;
            case FIN_WAIT_2:
                connection->tcb.SEG.CTL = ACK_FLAG;
				connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
				connection->tcb.RCV.NXT = ntohl(tcp->sequenceNumber)+1;
				connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
				tcp_send(connection, 0, 0);
								
                connection->TCP_CurrState = TIME_WAIT;
                /// TEST
                delay(100000);
                tcp_deleteConnection(connection);
                /// TEST
                break;
            case FIN_WAIT_1:
                connection->tcb.SEG.CTL = ACK_FLAG;
				connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
				connection->tcb.RCV.NXT = ntohl(tcp->sequenceNumber)+1;
				connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
				tcp_send(connection, 0, 0);
				
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
            connection->tcb.SND.UNA = ntohl(tcp->acknowledgmentNumber);
            connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
            connection->tcb.RCV.NXT = ntohl(tcp->sequenceNumber) + 1;
            connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
            connection->tcb.SEG.CTL = ACK_FLAG;
            tcp_send(connection, 0, 0);

            connection->TCP_CurrState = TIME_WAIT;
            /// TEST
            delay(100000);
            tcp_deleteConnection(connection);
            /// TEST
        }

        // HACK due to observations in wireshark:
        if (connection->TCP_CurrState == ESTABLISHED)
        {
            connection->tcb.SND.UNA = ntohl(tcp->acknowledgmentNumber);
            connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
            connection->tcb.RCV.NXT = ntohl(tcp->sequenceNumber) + 1;
            connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
            connection->tcb.SEG.CTL = ACK_FLAG;
            tcp_send(connection, 0, 0);

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

void tcp_send(tcpConnection_t* connection, void* data, uint32_t length)	
{
	tcpFlags flags     = connection->tcb.SEG.CTL;
	uint32_t seqNumber = connection->tcb.SEG.SEQ;
	uint32_t ackNumber = connection->tcb.SEG.ACK;

    textColor(HEADLINE);
    printf("\n\nTCP send: ");
    textColor(TEXT);
    printf("conn. ID: %u  ", connection->ID);
    textColor(IMPORTANT);
    printf("%u ==> %u  ", connection->localSocket.port, connection->remoteSocket.port);
    textColor(TEXT);

    tcpPacket_t* tcp = malloc(sizeof(tcpPacket_t)+length, 0, "TCP packet");
    memcpy(tcp+1, data, length);

    tcp->sourcePort           = htons(connection->localSocket.port);
    tcp->destPort             = htons(connection->remoteSocket.port);
    tcp->sequenceNumber       = htonl(seqNumber);
    tcp->acknowledgmentNumber = htonl(ackNumber);
    tcp->dataOffset           = sizeof(tcpPacket_t)>>2; // header length has to be provided as number of DWORDS
    tcp->reserved             = 0;

    tcp->CWR = tcp->ECN = tcp->URG = tcp->ACK = tcp->PSH = tcp->RST = tcp->SYN = tcp->FIN = 0;
    switch (flags)
    {
        case SYN_FLAG:
            tcp->SYN = 1; // SYN
            break;
        case SYN_ACK_FLAG:
            tcp->ACK = 1; // ACK
            tcp->SYN = 1; // SYN
            break;
        case ACK_FLAG:
            tcp->ACK = 1; // ACK
            break;
        case FIN_FLAG:
            tcp->FIN = 1; // FIN
            break;
        case FIN_ACK_FLAG:
            tcp->ACK = 1; // ACK
            tcp->FIN = 1; // FIN
            break;
        case RST_FLAG:
            tcp->RST = 1; // RST
            break;
    }
    printFlag(tcp->URG, "URG"); printFlag(tcp->ACK, "ACK"); printFlag(tcp->PSH, "PSH");
    printFlag(tcp->RST, "RST"); printFlag(tcp->SYN, "SYN"); printFlag(tcp->FIN, "FIN");

    tcp->window = htons(8192); // TODO: Clarify
    tcp->urgentPointer = 0;    // TODO: Clarify

    tcp->checksum = 0; // for checksum calculation

    tcp->checksum = htons(udptcpCalculateChecksum((void*)tcp, length + sizeof(tcpPacket_t), connection->localSocket.IP, connection->remoteSocket.IP, 6));

    ipv4_send(connection->adapter, tcp, length + sizeof(tcpPacket_t), connection->remoteSocket.IP, 6);
    free(tcp);

	// increase SND.NXT
    if (connection->TCP_CurrState == ESTABLISHED)
    {
        connection->tcb.SND.NXT += length;
    }
}

static bool IsSegmentAcceptable (tcpPacket_t* tcp, tcpConnection_t* connection, uint16_t tcpDatalength)
{
    if (tcp->window == 0)
    {
        if (tcpDatalength)
        {
            return false;
        }
        else
        {
            return (ntohl(tcp->sequenceNumber) == connection->tcb.RCV.NXT);            
        }
    }
    else // Receive Window > 0
    {
        if (tcpDatalength)
        {
            bool cond1 = ( ntohl(tcp->sequenceNumber) >= connection->tcb.RCV.NXT  &&  ntohl(tcp->sequenceNumber) < connection->tcb.RCV.NXT + ntohs(tcp->window) );
			bool cond2 = ( ntohl(tcp->sequenceNumber) + tcpDatalength >= connection->tcb.RCV.NXT ) && ( ( ntohl(tcp->sequenceNumber) + tcpDatalength ) < ( connection->tcb.RCV.NXT + ntohs(tcp->window) ) ); 	
			return ( cond1 || cond2 );
        }
        else // LEN = 0
        {
            return ( ntohl(tcp->sequenceNumber) >= connection->tcb.RCV.NXT && ntohl(tcp->sequenceNumber) < connection->tcb.RCV.NXT + ntohs(tcp->window) );            
        }
    }
}

static uint16_t getFreeSocket()
{
    static uint16_t srcPort = 49152;
    return srcPort++;
}

static uint32_t getConnectionID()
{
    static uint16_t ID = 1;
    return ID++;
}



// User functions
uint32_t tcp_uconnect(IP_t IP, uint16_t port)
{
    tcpConnection_t* connection = tcp_createConnection();
    connection->remoteSocket.IP.iIP = IP.iIP;
    connection->remoteSocket.port = port;
    connection->adapter = network_getFirstAdapter(); // Hack

    if(connection->adapter)
    {
        connection->localSocket.IP.iIP = connection->adapter->IP.iIP;
    }
    else
    {
        return 0;
    }

    if(IP.iIP == 0)
    {
        tcp_bind(connection, connection->adapter); // passive open
    }
    else
    {
        tcp_connect(connection); // active open
    }

    return(connection->ID);
}

void tcp_usend(uint32_t ID, void* data, size_t length) // data exchange in state ESTABLISHED
{
    tcpConnection_t* connection = findConnectionID(ID);
    if (connection && connection->TCP_CurrState == ESTABLISHED)
    {
		connection->tcb.SEG.CTL = ACK_FLAG;
        tcp_send(connection, data, length);
    }
}

void tcp_uclose(uint32_t ID)
{
    tcpConnection_t* connection = findConnectionID(ID);
    if(connection)
    {
        tcp_deleteConnection(connection);
    }
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
