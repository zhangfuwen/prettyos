/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// TODO-LIST:
// implement dup-ack (as receiver: receive 1,2,3,n. If n!=4 then send dup-ack for 3. as sender: if we get dup-ack, send segment behind that)
// implement waiting 2 * connection->tcb.msl, delete connection (TIME_WAIT ==> CLOSED)

#include "tcp.h"
#include "video/console.h"
#include "kheap.h"
#include "util.h"
#include "timer.h"
#include "ipv4.h"
#include "list.h"
#include "task.h"
#include "serial.h"
#include "todo_list.h"


static const uint16_t STARTWINDOW =  6000;
static const uint16_t INCWINDOW   =    50;
static const uint16_t DECWINDOW   =   100;
static const uint16_t MAXWINDOW   = 10000;


// RTO constants
static const uint16_t RTO_STARTVALUE = 3000; // 3 sec // rfc 2988

// RTO calculation (RFC 2988)
// Variables: RTO  = retransmission timeout,   RTT    = round-trip time,
//            SRTT = smoothed round-trip time, RTTVAR = round-trip time variation
//
static void calculateRTO(tcpConnection_t* connection, uint32_t rtt)
{
    if (connection->tcb.rto == RTO_STARTVALUE)
    {
        // first RTT measurement R (in msec): SRTT <- R    and RTTVAR <- R/2
        connection->tcb.srtt    = rtt;
        connection->tcb.rttvar  = rtt/2;
    }
    else
    {
        const uint8_t  ALPHA = 8; // ALPHA = 1/alpha
        const uint8_t  BETA  = 4; // BETA  = 1/beta

        // subsequent RTT measurement R': RTTVAR <- (1 - beta) * RTTVAR + beta * |SRTT - R'|
        connection->tcb.rttvar = connection->tcb.rttvar + abs(connection->tcb.srtt - rtt) / BETA -
                                (connection->tcb.rttvar + abs(connection->tcb.srtt - rtt) / BETA) / BETA;

        // SRTT <- (1 - alpha) * SRTT + alpha * R'
        connection->tcb.srtt = connection->tcb.srtt + rtt / ALPHA -
                              (connection->tcb.srtt + rtt / ALPHA) / ALPHA;
    }

    //     RTO <- SRTT + max (G, K*RTTVAR) where K = 4.
    //     If it is less than 1 second then the RTO SHOULD be rounded up to 1 second.
    connection->tcb.rto = max( connection->tcb.srtt + /* max( 1000/timer_getFrequency(), */ 4 * connection->tcb.rttvar /* ) */, 1000);

    // A maximum value MAY be placed on RTO provided it is at least 60 seconds.
    if (connection->tcb.rto > 60000)
        connection->tcb.rto = 60000;
}

static const char* const tcpStates[] =
{
    "CLOSED", "LISTEN", "SYN_SENT", "SYN_RECEIVED", "ESTABLISHED", "FIN_WAIT_1", "FIN_WAIT_2", "CLOSING", "CLOSE_WAIT", "LAST_ACK", "TIME_WAIT"
};

static list_t* tcpConnections = 0;

static bool IsPacketAcceptable(tcpPacket_t* tcp, tcpConnection_t* connection, uint16_t tcpDatalength);
static uint16_t getFreeSocket();
static uint32_t getConnectionID();
static uint32_t tcp_deleteInBuffers(tcpConnection_t* connection);
static uint32_t tcp_deleteOutBuffers(tcpConnection_t* connection);

static void scheduledDeleteConnection(void* data, size_t length)
{
    tcp_deleteConnection(*(tcpConnection_t**)data);    
}

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

tcpConnection_t* findConnection(IP_t IP, uint16_t port, network_adapter_t* adapter, TCP_state state)
{
    if(tcpConnections == 0)
        return(0);

    for(element_t* e = tcpConnections->head; e != 0; e = e->next)
    {
        tcpConnection_t* connection = e->data;

        switch(state)
        {
            case TCP_ANY:
                if (connection->adapter == adapter &&
                    connection->remoteSocket.port == port &&
                    connection->remoteSocket.IP.iIP == IP.iIP)
                    return(connection);
                break;
            case LISTEN:
                if (connection->TCP_CurrState == state &&
                    connection->adapter == adapter &&
                  ((connection->remoteSocket.port == port &&
                    connection->remoteSocket.IP.iIP == IP.iIP) ||
                   (connection->remoteSocket.port == 0 &&
                    connection->remoteSocket.IP.iIP == 0)))
                    return(connection);
                break;
            default:
                if (connection->adapter == adapter &&
                    connection->remoteSocket.port == port &&
                    connection->remoteSocket.IP.iIP == IP.iIP &&
                    connection->TCP_CurrState == state)
                    return(connection);
                break;
        }
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

static void tcp_debug(tcpPacket_t* tcp, bool showWnd)
{
    textColor(IMPORTANT);
    printf("%u ==> %u   ", ntohs(tcp->sourcePort), ntohs(tcp->destPort));
    textColor(TEXT);
    printFlag(tcp->URG, "URG"); printFlag(tcp->ACK, "ACK"); printFlag(tcp->PSH, "PSH");
    printFlag(tcp->RST, "RST"); printFlag(tcp->SYN, "SYN"); printFlag(tcp->FIN, "FIN");
    textColor(LIGHT_GRAY);
    if (showWnd)
    {
        printf("  WND = %u  ", ntohs(tcp->window));
    }
    textColor(TEXT);
}

static void tcpShowConnectionStatus(tcpConnection_t* connection)
{
    textColor(IMPORTANT);
    putch(' ');
    puts(tcpStates[connection->TCP_CurrState]);
    textColor(TEXT);
  #ifdef _NETWORK_DATA_
    printf("   conn. ID: %u   src port: %u\n", connection->ID, connection->localSocket.port);
    printf("SND.UNA = %u, SND.NXT = %u, SND.WND = %u", connection->tcb.SND.UNA, connection->tcb.SND.NXT, connection->tcb.SND.WND);
  #endif
}

tcpConnection_t* tcp_createConnection()
{
    if(tcpConnections == 0)
    {
        tcpConnections = list_create();
    }
    tcpConnection_t* connection = malloc(sizeof(tcpConnection_t), 0, "tcp connection");
    connection->inBuffer        = list_create();
    connection->outBuffer       = list_create();
    connection->owner           = (void*)currentTask;
    connection->ID              = getConnectionID();
    connection->TCP_PrevState   = CLOSED;
    connection->TCP_CurrState   = CLOSED;
    connection->tcb.rto         = RTO_STARTVALUE; // for first calculation
    connection->tcb.retrans     = false;
    connection->tcb.msl         = 10000; // 10 sec max. segment lifetime  // CHECK
    connection->tcb.RCV.dACK    = 0; // duplicate ACKs received

    list_append(tcpConnections, connection);
    textColor(TEXT);
    printf("\nTCP conn. created, ID: %u\n", connection->ID);
    return(connection);
}

void tcp_deleteConnection(tcpConnection_t* connection)
{
    serial_log(1,"\r\ntcp_deleteConnection");

    connection->TCP_PrevState = connection->TCP_CurrState;
    connection->TCP_CurrState = CLOSED;

    uint32_t countOUT = tcp_deleteOutBuffers(connection); // free
    uint32_t countIN  = tcp_deleteInBuffers (connection); // free

    list_delete(tcpConnections, connection);
    free(connection);

    serial_log(1,"\r\nTCP conn.ID: %u <--- deleted, del countIN: %u del countOUT (not acked): %u \n", connection->ID, countIN, countOUT);
}

void tcp_bind(tcpConnection_t* connection, struct network_adapter* adapter) // passive open  ==> LISTEN
{
    connection->localSocket.IP.iIP = adapter->IP.iIP;
    connection->localSocket.port  = 0;
    connection->remoteSocket.port = 0;
    connection->TCP_PrevState = connection->TCP_CurrState;
    connection->TCP_CurrState = LISTEN;
    connection->adapter = adapter;
    connection->passive = true;

    tcpShowConnectionStatus(connection);
}

void tcp_connect(tcpConnection_t* connection) // active open  ==> SYN-SENT
{
    connection->TCP_PrevState = connection->TCP_CurrState;
    connection->localSocket.port = getFreeSocket(); // with srand(...)
    connection->passive = false;

    if (connection->TCP_PrevState == CLOSED || connection->TCP_PrevState == LISTEN || connection->TCP_PrevState == TIME_WAIT)
    {
        connection->tcb.SND.WND = STARTWINDOW;
        connection->tcb.SND.ISS = rand();
        connection->tcb.SND.UNA = connection->tcb.SND.ISS;
        connection->tcb.SND.NXT = connection->tcb.SND.ISS + 1;

        connection->tcb.SEG.WND = connection->tcb.SND.WND;
        connection->tcb.SEG.SEQ = connection->tcb.SND.ISS;
        connection->tcb.SEG.CTL = SYN_FLAG;
        connection->tcb.SEG.ACK = 0;

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
        connection->tcb.SND.NXT = connection->tcb.SEG.SEQ + 1;
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

        //connection->TCP_CurrState = LAST_ACK;
        connection->TCP_CurrState = CLOSED;
        tcp_deleteConnection(connection);
    }
    else if (connection->TCP_PrevState == SYN_SENT || connection->TCP_PrevState == LISTEN)
    {
        connection->TCP_CurrState = CLOSED;
        tcp_deleteConnection(connection);
    }
}

static bool tcp_prepare_send_ACK(tcpConnection_t* connection, tcpPacket_t* tcp, bool set_SND_UNA)
{
    if (set_SND_UNA)
    {
        connection->tcb.SND.UNA = max(connection->tcb.SND.UNA, ntohl(tcp->acknowledgmentNumber));
    }

    connection->tcb.RCV.WND = ntohs(tcp->window);
    connection->tcb.RCV.NXT = ntohl(tcp->sequenceNumber)+1;
    connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
    connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
    connection->tcb.SEG.CTL = ACK_FLAG;
    return true;
}

static void tcp_send_DupAck(tcpConnection_t* connection)
{    
    connection->tcb.SEG.ACK = connection->tcb.SND.UNA;
    connection->tcb.SEG.CTL = ACK_FLAG;
    serial_log(1,"We send now Dup-Ack:\r\n");
    serial_log(1,"\tseq:\t%u", connection->tcb.SEG.SEQ - connection->tcb.SND.ISS);
    serial_log(1,"\tack:\t%u\r\n", connection->tcb.SEG.ACK - connection->tcb.RCV.IRS);
    tcp_send(connection, 0, 0);
}

/*
// TEST DUP_ACK
if ( tcpDataLength && (tcp->sequenceNumber > connection->tcb.RCV.NXT) )
{
    serial_log(1,"rcvd:\tseq:\t%u", ntohl(tcp->sequenceNumber) - connection->tcb.RCV.IRS);
    tcp_send_DupAck(connection);
    return;
}
*/

static bool tcp_prepare_send_FIN(tcpConnection_t* connection, tcpPacket_t* tcp, bool set_SND_UNA)
{
    if (set_SND_UNA)
    {
        connection->tcb.SND.UNA = max(connection->tcb.SND.UNA, ntohl(tcp->acknowledgmentNumber));
    }

    connection->tcb.RCV.WND = ntohs(tcp->window);
    connection->tcb.RCV.NXT = ntohl(tcp->sequenceNumber) + 1;
    connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
    connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
    connection->tcb.SEG.CTL = FIN_FLAG;
    return true;
}

// This function has to be checked intensively!!!
// cf. http://www.systems.ethz.ch/education/past-courses/fs10/operating-systems-and-networks/material/TCP-Spec.pdf
void tcp_receive(network_adapter_t* adapter, tcpPacket_t* tcp, IP_t transmittingIP, size_t length)
{
    bool tcp_sendFlag   = false;
    bool tcp_deleteFlag = false;

    textColor(HEADLINE);
    printf("\n\nTCP rcvd: ");
    tcp_debug(tcp, false);

    // search connection
    tcpConnection_t* connection = 0;
    if (tcp->SYN && !tcp->ACK) // SYN
    {
        connection = findConnection(transmittingIP, ntohs(tcp->destPort), adapter, LISTEN);
        if (connection)
        {
            connection->TCP_PrevState       = connection->TCP_CurrState;
            connection->remoteSocket.port   = ntohs(tcp->sourcePort);
            connection->localSocket.port    = ntohs(tcp->destPort);
            connection->remoteSocket.IP.iIP = transmittingIP.iIP;
        }
    }
    else
    {
        connection = findConnection(transmittingIP, ntohs(tcp->sourcePort), adapter, TCP_ANY);
    }

    if(connection == 0)
    {
        textColor(RED);
        printf("\nTCP packet received that does not belong to a TCP connection:");
        tcp_debug(tcp, false);
        textColor(TEXT);
        return;
    }

    textColor(TEXT);
    connection->TCP_PrevState = connection->TCP_CurrState;

    if (tcp->RST) // RST
    {
        connection->TCP_PrevState = connection->TCP_CurrState;

        if (connection->TCP_CurrState == SYN_RECEIVED)
        {
            // no send action
            connection->TCP_CurrState = LISTEN;
        }
    }

    switch(connection->TCP_CurrState)
    {
        case LISTEN:
            if (tcp->SYN && !tcp->ACK) // SYN
            {
                connection->tcb.RCV.WND = ntohs(tcp->window);
                connection->tcb.RCV.IRS = ntohl(tcp->sequenceNumber);
                connection->tcb.RCV.NXT = connection->tcb.RCV.IRS + 1;

                srand(timer_getMilliseconds());
                connection->tcb.SND.WND  = STARTWINDOW;
                connection->tcb.SND.ISS  = rand();
                connection->tcb.SND.UNA = connection->tcb.SND.ISS;
                connection->tcb.SND.NXT = connection->tcb.SND.ISS + 1;

                connection->tcb.SEG.WND  = connection->tcb.SND.WND;
                connection->tcb.SEG.SEQ  = connection->tcb.SND.ISS;
                connection->tcb.SEG.ACK  = connection->tcb.RCV.NXT;
                connection->tcb.SEG.CTL  = SYN_ACK_FLAG;
                tcp_sendFlag = true;
                connection->TCP_CurrState = SYN_RECEIVED;
            }
            break;

        case SYN_RECEIVED:
            if (!tcp->SYN && !tcp->FIN && tcp->ACK) // ACK
            {
                connection->TCP_CurrState = ESTABLISHED;
                event_issue(connection->owner->eventQueue, EVENT_TCP_CONNECTED, &connection->ID, sizeof(connection->ID));
            }
            break;

        case SYN_SENT:
            if (tcp->SYN && !tcp->ACK) // SYN
            {
                connection->tcb.RCV.WND = ntohs(tcp->window);
                connection->tcb.RCV.IRS = ntohl(tcp->sequenceNumber);
                connection->tcb.RCV.NXT = connection->tcb.RCV.IRS + 1;  

                connection->tcb.SEG.SEQ = connection->tcb.SND.NXT; // CHECK
                connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
                connection->tcb.SEG.CTL = SYN_ACK_FLAG;
                tcp_sendFlag = true;
                connection->TCP_CurrState = SYN_RECEIVED;
            }
            else if (tcp->SYN && tcp->ACK)  // SYN ACK
            {
                connection->tcb.RCV.WND = ntohs(tcp->window);
                connection->tcb.RCV.IRS = ntohl(tcp->sequenceNumber);
                connection->tcb.RCV.NXT = connection->tcb.RCV.IRS + 1;

                tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp, true);

                connection->TCP_CurrState = ESTABLISHED;
                event_issue(connection->owner->eventQueue, EVENT_TCP_CONNECTED, &connection->ID, sizeof(connection->ID));
            }
            break;

        case ESTABLISHED: // ***** ESTABLISHED ***** DATA TRANSFER ***** ESTABLISHED ***** DATA TRANSFER ***** ESTABLISHED ***** DATA TRANSFER *****
        {
            char*    tcpData       = (char*)( (uintptr_t)tcp + 4 * tcp->dataOffset ); // dataOffset is given as number of DWORDs
            uint32_t tcpDataLength = length - (4 * tcp->dataOffset);
            
            if (!IsPacketAcceptable(tcp, connection, tcpDataLength))
            {
                textColor(ERROR); printf("not acceptable!"); textColor(TEXT);
                // if RST is on, STOP.
                // if RST is off, send segment from queue 
                break; // return; // ??
            }

            if (tcp->RST || tcp->SYN) // RST or SYN
            {
                //if SYN, send segment from queue 
                connection->TCP_CurrState = CLOSED;
                tcp_deleteConnection(connection);
                return;
            }
            
            // http://www.medianet.kent.edu/techreports/TR2005-07-22-tcp-EFSM.pdf  ES2  page 18
            
            if (!tcp->ACK)
            {
                return;
            }            
            else // ACK
            {
                if (! (ntohl(tcp->acknowledgmentNumber) > connection->tcb.SND.UNA && ntohl(tcp->acknowledgmentNumber) <= connection->tcb.SND.NXT))
                {
                    // This does not mean a new ACK

                    if (!(ntohl(tcp->acknowledgmentNumber) == connection->tcb.SND.UNA))
                    {
                        // invalid ACK, drop
                        return;
                    }
                    else
                    {
                        // valid Duplicate ACK
                        connection->tcb.RCV.dACK++;

                        if (connection->tcb.RCV.dACK == 1)
                        {
                            // ignore second duplicate ACK and continue!
                            goto ES3;
                        }
                        else
                        {
                            if (connection->tcb.RCV.dACK == 2) //3rd duplicate ACK
                            {
                                // Release REXMT Timer
                                
                                // Retransmit Lost Segment

                                // SSthresh = max (2, min(CWND, SND.WND/2)) ??
                                goto ES3;
                            }
                            else
                            {
                                // ES4
                                // ...
                                goto ES3; // HACK
                            }
                        }
                    }
                }
                else // This means a new and valid ACK
                {   
                    // Window Update (??)

                    // SND.WND = min(CWND, SSthresh, SEG.LEN)

                    // Remove all ack'ed segments from the Rexmt Queue

                    // SND.UNA = SEG.ACK
                    // ExpBoff = 1 //??

                    // Release REXMT Timer

                    goto ES3; // only for documentation
                

                 ES3:
                        if (ntohl(tcp->sequenceNumber) == connection->tcb.RCV.NXT)
                        {
                            // cf. ES3
                            // handle Out-of-Order RCV buffer (sorted --> RCV buffer)
                        }
                        else
                        {
                            
                            serial_log(1,"rcvd:\tseq:\t%u", ntohl(tcp->sequenceNumber) - connection->tcb.RCV.IRS); 
                            serial_log(1,"\r\nsend Dup-ACK!");
                            tcp_send_DupAck(connection);
                            
                            // Add received data to the temporary Out-of-Order RCV Buffer <--------------------------------- TODO: OO RCV Buffer

                            connection->tcb.RCV.WND += tcpDataLength;
                        }                        
                    
                        if (connection->passive)
                        {
                            textColor(LIGHT_GRAY);
                            printf("data: ");
                            textColor(DATA);
                            for (uint16_t i=0; i<tcpDataLength; i++)
                            {
                                putch(tcpData[i]);
                            }

                            // Analysis
                            textColor(IMPORTANT);
                            printf("\ntcp: %u  tcpData: %u  tcpDataOff: %u\n",
                                    length, tcpDataLength, tcp->dataOffset);
                            textColor(LIGHT_BLUE);
                            for (uint16_t i=0; i<tcpDataLength; i++) // TODO: Is tcpDataLength+4 really correct, or does it belong to the old CRC/Padding-HACK?
                            {
                                printf("%y ", tcpData[i]);
                            }
                            textColor(TEXT);
                        }
                        sleepMilliSeconds(2);

                        
                        // TO BE CHECKED:
                        connection->tcb.RCV.WND =  ntohs(tcp->window); // cf. receiving dup-ACK
                        connection->tcb.SND.UNA =  max(connection->tcb.SND.UNA, ntohl(tcp->acknowledgmentNumber)); // CHECK for unregular packets above

                        // set ack time in tcp out-buffer packets
                        for (element_t* e = connection->outBuffer->head; e != 0; e = e->next)
                        {
                            tcpOut_t* outPacket = e->data;
                            if ( ((outPacket->segment.SEQ + outPacket->segment.LEN) <= ntohl(tcp->acknowledgmentNumber)) && (outPacket->segment.LEN > 0))
                            {
                                 if(outPacket->time_ms_acknowledged == 0) // not acknowledged
                                 {
                                     outPacket->time_ms_acknowledged = timer_getMilliseconds(); // acknowledged
                                     outPacket->remoteAck = ntohl(tcp->acknowledgmentNumber);

                                     // retransmission timeout (RTO)
                                     if (outPacket->remoteAck - (outPacket->segment.SEQ + outPacket->segment.LEN) == 0)
                                     {
                                         calculateRTO(connection, outPacket->time_ms_acknowledged - outPacket->time_ms_transmitted);
                                     }
                                 }
                            }
                        }

                        connection->tcb.RCV.NXT =  ntohl(tcp->sequenceNumber) + tcpDataLength;

                        connection->tcb.SND.WND -= tcpDataLength;

                        connection->tcb.SEG.SEQ =  connection->tcb.SND.NXT;
                        connection->tcb.SEG.ACK =  connection->tcb.RCV.NXT;
                        connection->tcb.SEG.LEN =  0; // send no data
                        connection->tcb.SEG.WND =  connection->tcb.SND.WND;
                        connection->tcb.SEG.CTL =  ACK_FLAG;

                        if (tcpDataLength)
                        {
                            //Fill in-buffer list
                            tcpIn_t* In    = malloc(sizeof(tcpIn_t), 0, "tcp_InBuffer");
                            In->ev         = malloc(sizeof(tcpReceivedEventHeader_t) + tcpDataLength, 0, "tcp_InBuf_data");
                            memcpy(In->ev+1, tcpData, tcpDataLength);
                            In->seq        = ntohl(tcp->sequenceNumber);
                            In->ev->length = tcpDataLength;
                            In->ev->connectionID = connection->ID;
                            list_append(connection->inBuffer, In); // received data ==> inBuffer // CHECK TCP PROCESS

                            // Issue event
                            uint8_t retVal = event_issue(connection->owner->eventQueue, EVENT_TCP_RECEIVED, In->ev, sizeof(tcpReceivedEventHeader_t)+tcpDataLength);
                            connection->tcb.SND.WND += tcpDataLength; // delivered to application

                            uint32_t totalTCPdataSize = 0;

                            mutex_lock(connection->owner->eventQueue->mutex);
                            for(size_t i = 0; ; i++)
                            {
                                event_t* ev = event_peek(connection->owner->eventQueue, i);
                                if(ev == 0) break;
                                if(ev->type == EVENT_TCP_RECEIVED)
                                {
                                    totalTCPdataSize += ev->length - sizeof(tcpReceivedEventHeader_t);
                                }
                            }
                            mutex_unlock(connection->owner->eventQueue->mutex);
                            textColor(MAGENTA);
                            printf("\ntotalTCPdataSize: %u ", totalTCPdataSize);
                            textColor(TEXT);

                            // setting sliding window
                            if (retVal==0)
                            {
                                textColor(SUCCESS); printf("ID. %u event queue OK", In->ev->connectionID);
                                if (totalTCPdataSize <  STARTWINDOW && connection->tcb.SND.WND < MAXWINDOW)
                                    {connection->tcb.SEG.WND = connection->tcb.SND.WND += INCWINDOW;}
                                if (totalTCPdataSize >= STARTWINDOW)
                                    {connection->tcb.SEG.WND = connection->tcb.SND.WND -= DECWINDOW;}
                                if (totalTCPdataSize >= MAXWINDOW  )
                                    {connection->tcb.SEG.WND = connection->tcb.SND.WND  = 0;}
                            }
                            else
                            {
                                textColor(ERROR); printf("ID. %u event queue error: %u", In->ev->connectionID, retVal);
                                connection->tcb.SEG.WND = connection->tcb.SND.WND  = 0;
                            }
                            textColor(TEXT);
                        }
                
                        tcp_sendFlag = true;
                        tcp_checkOutBuffers(connection,true);
                    }
            
                    /// ???
                    if (tcp->FIN) // FIN or FIN ACK
                    {
                        tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp, true);
                        connection->TCP_CurrState = CLOSE_WAIT;
                    }
                }

            break;
        }
        case FIN_WAIT_1:
            if (tcp->FIN && !tcp->ACK) // FIN
            {
                tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp, true);
                connection->TCP_CurrState = CLOSING;
            }
            else if (tcp->FIN && tcp->ACK) // FIN ACK
            {
                tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp, true);
                connection->TCP_CurrState = TIME_WAIT;                
                
                todoList_add(kernel_idleTasks, &scheduledDeleteConnection, &connection, sizeof(connection), connection->tcb.msl + timer_getMilliseconds());
            }
            else if (!tcp->SYN && !tcp->FIN && tcp->ACK) // ACK
            {
                connection->TCP_CurrState = FIN_WAIT_2;
            }
            break;
        case FIN_WAIT_2:
            if (tcp->FIN) // FIN
            {
                tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp, true);
                connection->TCP_CurrState = TIME_WAIT;
                
                todoList_add(kernel_idleTasks, &scheduledDeleteConnection, &connection, sizeof(connection), connection->tcb.msl + timer_getMilliseconds());
            }
            break;
        case CLOSING:
            if (!tcp->SYN && !tcp->FIN && tcp->ACK) // ACK
            {
                connection->TCP_CurrState = TIME_WAIT;
                
                todoList_add(kernel_idleTasks, &scheduledDeleteConnection, &connection, sizeof(connection), connection->tcb.msl + timer_getMilliseconds());
            }
            break;
        case TIME_WAIT:
            textColor(RED);
            printf("Packet received during state TIME_WAIT.");
            textColor(TEXT);
            break;
        case CLOSE_WAIT:
            tcp_sendFlag = tcp_prepare_send_FIN(connection, tcp, true);
            // connection->TCP_CurrState = LAST_ACK;
            connection->TCP_CurrState = CLOSED;
            tcp_deleteFlag = true;
            break;
        case LAST_ACK:
            if (tcp->ACK) // ACK
            {
                connection->TCP_CurrState = CLOSED;
                tcp_deleteFlag = true;
            }
            break;
        default:
            textColor(ERROR);
            printf("This default state should not happen.");
            textColor(TEXT);
            break;
    }//switch (connection->TCP_CurrState)

    /// LOG
    serial_log(1,"recv: ");
    if(tcp->FIN) serial_log(1," FIN");
    if(tcp->SYN) serial_log(1," SYN");
    if(tcp->ACK) serial_log(1," ACK");
    if(tcp->RST) serial_log(1," RST");
    if(tcp->PSH) serial_log(1," PSH");
    if(tcp->URG) serial_log(1," URG");
    serial_log(1,"\tseq:\t%u", ntohl(tcp->sequenceNumber) - connection->tcb.RCV.IRS);
    serial_log(1,"\trcv nxt:\t%u", connection->tcb.RCV.NXT - connection->tcb.RCV.IRS);
    if (tcp->ACK)
    {
        serial_log(1,"\tack:\t%u", ntohl(tcp->acknowledgmentNumber) - connection->tcb.SND.ISS);
        serial_log(1,"\t\tSND.UNA:\t%u", connection->tcb.SND.UNA - connection->tcb.SND.ISS);
    }
    serial_log(1,"\r\n");
    /// LOG

    if (tcp_sendFlag)
    {
        tcp_send(connection, 0, 0);
    }
    tcpShowConnectionStatus(connection);
    if (tcp_deleteFlag)
    {
        tcp_deleteConnection(connection);
    }
}

void tcp_send(tcpConnection_t* connection, void* data, uint32_t length)
{
    textColor(HEADLINE);
    printf("\nTCP send: ");
    textColor(TEXT);

    tcpPacket_t* tcp = malloc(sizeof(tcpPacket_t)+length, 0, "TCP packet");
    memcpy(tcp+1, data, length);

    tcp->sourcePort           = htons(connection->localSocket.port);
    tcp->destPort             = htons(connection->remoteSocket.port);
    tcp->sequenceNumber       = htonl(connection->tcb.SEG.SEQ);
    tcp->acknowledgmentNumber = htonl(connection->tcb.SEG.ACK);
    tcp->dataOffset           = sizeof(tcpPacket_t)>>2; // header length has to be provided as number of DWORDS
    tcp->reserved             = 0;

    tcp->CWR = tcp->ECN = tcp->URG = tcp->ACK = tcp->PSH = tcp->RST = tcp->SYN = tcp->FIN = 0;
    switch (connection->tcb.SEG.CTL)
    {
        case SYN_FLAG:
            tcp->SYN = 1; // SYN
            break;
        case SYN_ACK_FLAG:
            tcp->SYN = tcp->ACK = 1; // SYN ACK
            break;
        case ACK_FLAG:
            tcp->ACK = 1; // ACK
            break;
        case FIN_FLAG:
            tcp->FIN = 1; // FIN
            break;
        case FIN_ACK_FLAG:
            tcp->FIN = tcp->ACK = 1; // FIN ACK
            break;
        case RST_FLAG:
            tcp->RST = 1; // RST
            break;
    }

    tcp->window = htons(connection->tcb.SEG.WND);
    tcp->urgentPointer = 0;

    tcp->checksum = 0; // for checksum calculation
    tcp->checksum = htons(udptcpCalculateChecksum((void*)tcp, length + sizeof(tcpPacket_t), connection->localSocket.IP, connection->remoteSocket.IP, 6));

    ipv4_send(connection->adapter, tcp, length + sizeof(tcpPacket_t), connection->remoteSocket.IP, 6); // tcp protocol: 6
    free(tcp);

    // increase SND.NXT
    if (connection->TCP_CurrState == ESTABLISHED && connection->tcb.retrans == false)
    {
        connection->tcb.SND.NXT += length;
    }
    /// LOG
    serial_log(1,"send: ");
    if(tcp->FIN) serial_log(1," FIN");
    if(tcp->SYN) serial_log(1," SYN");
    if(tcp->ACK) serial_log(1," ACK");
    if(tcp->RST) serial_log(1," RST");
    if(tcp->PSH) serial_log(1," PSH");
    if(tcp->URG) serial_log(1," URG");
    serial_log(1,"\tseq:\t%u", ntohl(tcp->sequenceNumber) - connection->tcb.SND.ISS);
    serial_log(1,"\tseq nxt:\t%u", connection->tcb.SND.NXT - connection->tcb.SND.ISS);
    if (tcp->ACK)
    {
        serial_log(1,"\tack:\t%u", ntohl(tcp->acknowledgmentNumber) - connection->tcb.RCV.IRS);
    }
    serial_log(1,"\r\n");
    /// LOG

    tcp_debug(tcp, true);
}

uint32_t tcp_checkInBuffers(tcpConnection_t* connection, bool showData)
{
    printf("\n\n");
    uint32_t count = 0;
    for (element_t* e = connection->inBuffer->head; e != 0; e = e->next)
    {
        count++;
        tcpIn_t* inPacket = e->data;
        printf("\n seq = %u \tlen = %u\n", inPacket->seq, inPacket->ev->length);
        if (showData)
        {
            for (uint32_t i=0; i<inPacket->ev->length; i++)
            {
                putch( ((char*)(inPacket->ev+1))[i] );
            }
        }
    }
    return count;
}

static uint32_t tcp_deleteInBuffers(tcpConnection_t* connection)
{
    serial_log(1,"\r\ntcp_deleteInBuffers");

    uint32_t count = 0;
    for (element_t* e = connection->inBuffer->head; e != 0; e = e->next)
    {
        count++;
        tcpIn_t* inPacket = e->data;
        free(inPacket->ev);
        free(inPacket);
    }
    list_free(connection->inBuffer);
    connection->inBuffer = 0;

    return count;
}

static uint32_t tcp_deleteOutBuffers(tcpConnection_t* connection)
{
    serial_log(1,"\r\ntcp_deleteOutBuffers");

    uint32_t count = 0;
    for (element_t* e = connection->outBuffer->head; e != 0; e = e->next)
    {
        count++;
        tcpOut_t* outPacket = e->data;
        free(outPacket->data);
        free(outPacket);
    }
    list_free(connection->outBuffer);
    connection->outBuffer = 0;

    return count;
}

uint32_t tcp_checkOutBuffers(tcpConnection_t* connection, bool showData)
{
    printf("\n\n");
    uint32_t count = 0;
    for (element_t* e = connection->outBuffer->head; e != 0; e = e->next)
    {
        count++;
        tcpOut_t* outPacket = e->data;
        if (outPacket->time_ms_acknowledged) // acknowledged
        {
            printf("\nID %u  seq %u  len %u  acked by %u (diff: %u)  RTT=%u ms RTO=%u ms",
                connection->ID, outPacket->segment.SEQ, outPacket->segment.LEN, outPacket->remoteAck,
                outPacket->remoteAck - (outPacket->segment.SEQ + outPacket->segment.LEN),
                outPacket->time_ms_acknowledged - outPacket->time_ms_transmitted,
                connection->tcb.rto);
            textColor(GREEN);
        }
        else
        {
            printf("\nID %u  seq %u len %u (not yet acknowledged)", connection->ID, outPacket->segment.SEQ, outPacket->segment.LEN);
            textColor(DATA);
        }

        if (showData)
        {
            putch('\n');
            for (uint32_t i=0; i<outPacket->segment.LEN; i++)
            {
                putch( ((char*)(outPacket->data))[i] );
            }
        }

        if (outPacket->time_ms_acknowledged) // delete acknowledged list members
        {
            free(outPacket->data);
            free(outPacket);
            serial_log(1,"checkOutBuffers - delAckedOutPacket\r\n");
            list_delete(connection->outBuffer, e->data);
        }
        else // check need for retransmission
        {
            if ((timer_getMilliseconds() - outPacket->time_ms_transmitted) > connection->tcb.rto)
            {
                textColor(LIGHT_BLUE);
                printf("\nretransmission done for seg=%u! RTO will be doubled afterwards.", outPacket->segment.SEQ);
                connection->tcb.SEG.SEQ =  outPacket->segment.SEQ;
                connection->tcb.SEG.ACK =  outPacket->segment.ACK;
                connection->tcb.SEG.LEN =  outPacket->segment.LEN;
                connection->tcb.SEG.CTL =  ACK_FLAG;
                connection->tcb.retrans = true;
                tcp_send(connection, outPacket->data, connection->tcb.SEG.LEN);
                outPacket->time_ms_transmitted = timer_getMilliseconds();
                connection->tcb.retrans = false;
                connection->tcb.rto *= 2;
                if (connection->tcb.rto > 60000)
                    connection->tcb.rto = 60000;
            }
            else
            {
                textColor(TEXT);
                printf("\nWe are still waiting for the ACK");
            }
        }
        textColor(TEXT);
    }
    return count;
}


// http://www.medianet.kent.edu/techreports/TR2005-07-22-tcp-EFSM.pdf  page 41
static bool IsPacketAcceptable(tcpPacket_t* tcp, tcpConnection_t* connection, uint16_t tcpDatalength)
{
    if (tcp->window != 0)
    {
        if (tcpDatalength)
        {
            bool cond1 = ( ntohl(tcp->sequenceNumber) >=  connection->tcb.RCV.NXT  &&
                           ntohl(tcp->sequenceNumber) <   connection->tcb.RCV.NXT + ntohs(tcp->window) );
            bool cond2 = ( ntohl(tcp->sequenceNumber) + tcpDatalength >=   connection->tcb.RCV.NXT ) &&
                           ntohl(tcp->sequenceNumber) + tcpDatalength  < ( connection->tcb.RCV.NXT + ntohs(tcp->window) );
            return ( cond1 || cond2 );
        }
        else // LEN = 0
        {
            return ( ntohl(tcp->sequenceNumber) >= connection->tcb.RCV.NXT &&
                     ntohl(tcp->sequenceNumber) < (connection->tcb.RCV.NXT + ntohs(tcp->window)) );
        }
    }
    else
    {
        if (tcpDatalength)
            return false;
        else
            return (ntohl(tcp->sequenceNumber) == connection->tcb.RCV.NXT);
    }
}

static uint16_t getFreeSocket()
{
    static bool flag = false;
    if(!flag)
    {
        srand(timer_getMilliseconds());
        flag = true;
    }
    return ( 0xC000 + rand()%(0xFFFF-0xC000) );
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

    if(connection->adapter == 0)
    {
        return 0;
    }

    connection->localSocket.IP = connection->adapter->IP;

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

    if(connection == 0)
        return;

    tcpOut_t* Out = malloc(sizeof(tcpOut_t), 0, "tcp_OutBuffer");
    Out->data     = malloc(length, 0, "tcp_OutBuf_data");
    memcpy(Out->data, data, length);

    Out->segment.SEQ = connection->tcb.SND.NXT;
    Out->segment.ACK = connection->tcb.SEG.ACK;
    Out->segment.LEN = length;
    Out->segment.WND = connection->tcb.SEG.WND;
    Out->segment.CTL = connection->tcb.SEG.CTL;

    if (connection->TCP_CurrState == ESTABLISHED)
    {
        connection->tcb.SEG.CTL = ACK_FLAG; // necessary?
        tcp_send(connection, data, length);
    }
    Out->time_ms_transmitted  = timer_getMilliseconds();

    Out->time_ms_acknowledged = 0; // not acknowledged
    list_append(connection->outBuffer, Out); // data to be acknowledged ==> OutBuffer // CHECK TCP PROCESS
}

void tcp_uclose(uint32_t ID)
{
    tcpConnection_t* connection = findConnectionID(ID);
    if(connection)
    {
        tcp_close(connection);
    }
    else
    {
        textColor(ERROR);
        printf("connection with ID %u could not be found and closed.", ID);
        textColor(TEXT);
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
