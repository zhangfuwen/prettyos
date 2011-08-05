/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

/// state diagram: http://upload.wikimedia.org/wikipedia/commons/0/08/TCP_state_diagram.jpg
/// EFSM/SDL:      http://www.medianet.kent.edu/techreports/TR2005-07-22-tcp-EFSM.pdf

#include "tcp.h"
#include "video/console.h"
#include "kheap.h"
#include "util.h"
#include "timer.h"
#include "ipv4.h"
#include "task.h"
#include "serial.h"
#include "todo_list.h"


// Sliding window
static const uint16_t STARTWINDOW    =  6000;
static const uint16_t INCWINDOW      =    50;
static const uint16_t DECWINDOW      =   100;
static const uint16_t MAXWINDOW      = 10000;

static const uint16_t MSL            =  5000; // 5 sec max. segment lifetime  // CHECK
static const uint16_t RTO_STARTVALUE =  3000; // 3 sec  // rfc 2988
static const uint16_t RTO_MAXVALUE   = 60000; // 60 sec // a maximum value MAY be placed on RTO provided it is at least 60 seconds.
static const uint16_t MSS            =  1500 - sizeof(ipv4Packet_t) - sizeof(tcpPacket_t); // Maximum segment size

static const char* const tcpStates[] =
{
    "CLOSED", "LISTEN", "SYN_SENT", "SYN_RECEIVED", "ESTABLISHED",
    "FIN_WAIT_1", "FIN_WAIT_2", "CLOSING", "CLOSE_WAIT", "LAST_ACK", "TIME_WAIT"
};

static list_t*  tcpConnections = 0;


static bool     tcp_IsPacketAcceptable(tcpPacket_t* tcp, tcpConnection_t* connection, uint16_t tcpDatalength);
static uint16_t tcp_getFreeSocket();
static uint32_t tcp_getConnectionID();
static uint32_t tcp_deleteInBuffers(tcpConnection_t* connection, list_t* list);
static uint32_t tcp_deleteOutBuffers(tcpConnection_t* connection);
static uint32_t tcp_checkOutBuffers(tcpConnection_t* connection, bool showData);
static bool     tcp_retransOutBuffer(tcpConnection_t* connection, uint32_t seq);
static void     tcpShowConnectionStatus(tcpConnection_t* connection);
static void     tcp_debug(tcpPacket_t* tcp, bool showWnd);
static uint32_t tcp_logBuffers(tcpConnection_t* connection, bool showData, list_t* list);
static void     tcp_sendFin(tcpConnection_t* connection);
static void     tcp_sendReset(tcpConnection_t* connection, tcpPacket_t* tcp, bool ack, uint32_t length);
static void     tcp_send_DupAck(tcpConnection_t* connection);
static bool     tcp_prepare_send_ACK(tcpConnection_t* connection, tcpPacket_t* tcp);
static void     calculateRTO(tcpConnection_t* connection, uint32_t rtt);
static void     tcp_RemoveAckedPacketsFromOutBuffer(tcpConnection_t* connection, tcpPacket_t* tcp);

static tcpConnection_t* tcp_findConnectionID(uint32_t ID)
{
    if(tcpConnections == 0)
        return(0);

    for(dlelement_t* e = tcpConnections->head; e != 0; e = e->next)
    {
        tcpConnection_t* connection = e->data;
        if(connection->ID == ID)
        {
            return(connection);
        }
    }

    return(0);
}

tcpConnection_t* tcp_findConnection(IP_t IP, uint16_t port, network_adapter_t* adapter, TCP_state state)
{
    if(tcpConnections == 0)
        return(0);

    for(dlelement_t* e = tcpConnections->head; e != 0; e = e->next)
    {
        tcpConnection_t* connection = e->data;

        switch(state)
        {
            case LISTEN:
                if (connection->TCP_CurrState == state && connection->adapter == adapter &&
                  ((connection->remoteSocket.port == port && connection->remoteSocket.IP.iIP == IP.iIP) ||
                   (connection->remoteSocket.port == 0 && connection->remoteSocket.IP.iIP == 0)))
                    return(connection);
                break;
            case TCP_ANY:
                if (connection->adapter == adapter && connection->remoteSocket.port == port && connection->remoteSocket.IP.iIP == IP.iIP)
                    return(connection);
                break;
            default:
                if (connection->adapter == adapter && connection->remoteSocket.port == port && connection->remoteSocket.IP.iIP == IP.iIP && connection->TCP_CurrState == state)
                    return(connection);
                break;
        }
    }
    return(0);
}

tcpConnection_t* tcp_createConnection()
{
    if(tcpConnections == 0)
    {
        tcpConnections = list_create();
    }
    tcpConnection_t* connection    = malloc(sizeof(tcpConnection_t), 0, "tcp connection");
    connection->inBuffer           = list_create();
    connection->OutofOrderinBuffer = list_create();
    connection->outBuffer          = list_create();
    connection->sendBuffer         = list_create();
    connection->owner              = (void*)currentTask;
    connection->ID                 = tcp_getConnectionID();
    connection->TCP_PrevState      = CLOSED;
    connection->TCP_CurrState      = CLOSED;
    connection->tcb.rto            = RTO_STARTVALUE; // for first calculation
    connection->tcb.retrans        = false;
    connection->tcb.msl            = MSL;
    connection->tcb.RCV.dACK       = 0; // duplicate ACKs received

    list_append(tcpConnections, connection);

  #ifdef _TCP_DEBUG_
    textColor(TEXT);
    printf("\nTCP conn. created, ID: %u\n", connection->ID);
  #endif

    return(connection);
}

void tcp_deleteConnection(tcpConnection_t* connection)
{
  #ifdef _TCP_DEBUG_
    printf("\ndeleteConnection: %X ID: %u\n", connection, connection->ID);
  #endif
    if (connection)
    {
        list_delete(tcpConnections, list_find(tcpConnections, connection));

        serial_log(1,"\r\n%u ms ID %u\t tcp_deleteConnection", timer_getMilliseconds(), connection->ID);

        connection->TCP_PrevState = connection->TCP_CurrState;
        connection->TCP_CurrState = CLOSED;

        uint32_t countOUT = tcp_deleteOutBuffers(connection); // free

        list_free(connection->sendBuffer); // CHECK INPUT TO BE SENT

        serial_log(1,"\r\nInBuffers to be deleted:");
        uint32_t countIN  = tcp_deleteInBuffers (connection, connection->inBuffer); // free
        connection->inBuffer = 0;

        serial_log(1,"\r\nOutofOrderInBuffers to be deleted:");
        uint32_t countOutofOrderIN = tcp_deleteInBuffers(connection, connection->OutofOrderinBuffer); // free
        connection->OutofOrderinBuffer = 0;

        serial_log(1,"\r\nDeleted ID %u, countIN: %u, countOutofOrderIN: %u, countOUT (not acked): %u \n", connection->ID, countIN, countOutofOrderIN, countOUT);
        free(connection);
    }
}

static void scheduledDeleteConnection(void* data, size_t length)
{
    tcpConnection_t* connection = *(tcpConnection_t**)data;
  #ifdef _TCP_DEBUG_
    printf("\nscheduledDeleteConnection: %X ID: %u\n", connection, connection->ID);
  #endif

    if (connection && list_find(tcpConnections, connection) != 0)
    {
        tcp_deleteConnection(connection);
    }
}

static void tcp_timeoutDeleteConnection(tcpConnection_t* connection, uint32_t timeMilliseconds)
{
    if (connection)
    {
        todoList_add(kernel_idleTasks, &scheduledDeleteConnection, &connection, sizeof(connection), timeMilliseconds + timer_getMilliseconds());

      #ifdef _TCP_DEBUG_
        textColor(LIGHT_BLUE);
        printf("\nconnection ID %u will be deleted at %u sec runtime.", connection->ID, (timeMilliseconds + timer_getMilliseconds()) / 1000);
        textColor(TEXT);
      #endif
    }
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
    connection->localSocket.port = tcp_getFreeSocket();
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
    if (connection)
    {
        connection->TCP_PrevState = connection->TCP_CurrState;

        switch (connection->TCP_PrevState)
        {
            case ESTABLISHED:
            case SYN_RECEIVED:
                tcp_sendFin(connection);
                connection->TCP_CurrState = FIN_WAIT_1;
                tcp_timeoutDeleteConnection(connection, 2*connection->tcb.msl);
                break;

            case CLOSE_WAIT:
                tcp_sendFin(connection);
                connection->TCP_CurrState = LAST_ACK;
                tcp_timeoutDeleteConnection(connection, 2*connection->tcb.msl);
                break;

            case SYN_SENT:
            case LISTEN:
                connection->TCP_CurrState = CLOSED;
                tcp_deleteConnection(connection);
                break;

            default:
                textColor(ERROR);
                printf("\nClose from unexpected state: %s", tcpStates[connection->TCP_PrevState]);
                textColor(TEXT);
                tcp_deleteConnection(connection);
                break;
        }
    }
}

// This function has to be checked intensively!!!
// cf. http://www.systems.ethz.ch/education/past-courses/fs10/operating-systems-and-networks/material/TCP-Spec.pdf
void tcp_receive(network_adapter_t* adapter, tcpPacket_t* tcp, IP_t transmittingIP, size_t length)
{
    bool tcp_sendFlag   = false;
    bool tcp_deleteFlag = false;

  #ifdef _TCP_DEBUG_
    textColor(HEADLINE);
    printf("\n\nTCP rcvd: ");
  #endif

    tcp_debug(tcp, false);

    // search connection
    tcpConnection_t* connection = 0;
    if (tcp->SYN && !tcp->ACK) // SYN
    {
        connection = tcp_findConnection(transmittingIP, ntohs(tcp->destPort), adapter, LISTEN);
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
        connection = tcp_findConnection(transmittingIP, ntohs(tcp->sourcePort), adapter, TCP_ANY);
    }

    if(connection == 0)
    {
      #ifdef _TCP_DEBUG_
        textColor(RED);
        printf("\nTCP packet received that does not belong to a TCP connection.");
        textColor(TEXT);
      #endif
        return;
    }

    textColor(TEXT);
    connection->TCP_PrevState = connection->TCP_CurrState;

    if (tcp->RST) // RST
    {
        connection->TCP_PrevState = connection->TCP_CurrState;

        if (connection->TCP_CurrState == SYN_RECEIVED)
        {
            connection->TCP_CurrState = LISTEN;
        }
    }

    switch(connection->TCP_CurrState)
    {
        case LISTEN:
            if (tcp->SYN && !tcp->ACK) // SYN
            {
                connection->tcb.RCV.WND  = ntohs(tcp->window);
                connection->tcb.RCV.IRS  = ntohl(tcp->sequenceNumber);
                connection->tcb.RCV.NXT  = connection->tcb.RCV.IRS + 1;

                connection->tcb.SND.WND  = STARTWINDOW;
                connection->tcb.SND.ISS  = rand();
                connection->tcb.SND.UNA  = connection->tcb.SND.ISS;
                connection->tcb.SND.NXT  = connection->tcb.SND.ISS + 1;

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
                connection->tcb.SND.UNA = max(connection->tcb.SND.UNA, ntohl(tcp->acknowledgmentNumber));
                event_issue(connection->owner->eventQueue, EVENT_TCP_CONNECTED, &connection->ID, sizeof(connection->ID));
            }
            break;

        case SYN_SENT:
            connection->tcb.RCV.WND = ntohs(tcp->window);
            connection->tcb.RCV.IRS = ntohl(tcp->sequenceNumber);
            connection->tcb.RCV.NXT = connection->tcb.RCV.IRS + 1;

            if (tcp->SYN && !tcp->ACK) // SYN
            {
                connection->tcb.SEG.SEQ = connection->tcb.SND.NXT; // CHECK
                connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
                connection->tcb.SEG.CTL = SYN_ACK_FLAG;
                tcp_sendFlag = true;
                connection->TCP_CurrState = SYN_RECEIVED;
            }
            else if (tcp->SYN && tcp->ACK)  // SYN ACK
            {
                tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp);
                connection->TCP_CurrState = ESTABLISHED;
                event_issue(connection->owner->eventQueue, EVENT_TCP_CONNECTED, &connection->ID, sizeof(connection->ID));
            }
            break;

        // ***** ESTABLISHED ***** DATA TRANSFER ***** ESTABLISHED ***** DATA TRANSFER ***** ESTABLISHED ***** DATA TRANSFER *************************
        case ESTABLISHED:
        {
            char*    tcpData       = (char*)tcp + 4 * tcp->dataOffset; // dataOffset is given as number of DWORDs
            uint32_t tcpDataLength = length - 4 * tcp->dataOffset;

            if (!tcp_IsPacketAcceptable(tcp, connection, tcpDataLength))
            {
              #ifdef _TCP_DEBUG_
                textColor(ERROR);
                printf("not acceptable!");
                textColor(TEXT);
              #endif

                if (tcp->RST)
                {
                    return;
                }

                tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp);
                break;
            }

            if (tcp->RST || tcp->SYN) // RST or SYN
            {
                //if SYN, send segment from queue
                tcp_deleteConnection(connection);
                return;
            }

            // http://www.medianet.kent.edu/techreports/TR2005-07-22-tcp-EFSM.pdf  ES2  page 18

            if (!tcp->ACK)
            {
                if (tcp->FIN) // FIN
                {
                     tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp);
                     connection->TCP_CurrState = CLOSE_WAIT;
                     break;
                }

                return;
            }
            else // ACK
            {
                if (! (ntohl(tcp->acknowledgmentNumber) >  connection->tcb.SND.UNA && ntohl(tcp->acknowledgmentNumber) <= connection->tcb.SND.NXT ))
                {
                    // This does not mean a new ACK

                    if (ntohl(tcp->acknowledgmentNumber) != connection->tcb.SND.UNA)
                    {
                        serial_log(1,"\r\ninvalid ack - drop!!!\r\n");
                        return; // invalid ACK, drop
                    }

                    // valid Duplicate ACK ?
                    if(tcpDataLength==0 && !tcp->FIN) // react only to empty ACK, not to data exchange, not to FIN ACK
                    {
                        connection->tcb.RCV.dACK++;
                    }

                    if (connection->tcb.RCV.dACK == 1)
                    {
                        serial_log(1,"\r\nignore 1st duplicate ACK and continue!\r\n");
                    }
                    else if (connection->tcb.RCV.dACK == 2) // 2nd duplicate ACK
                    {
                        serial_log(1,"\r\n2nd duplicate ACK\r\n");
                        tcp_retransOutBuffer(connection, ntohl(tcp->acknowledgmentNumber)); // Retransmission                        
                    }
                    else
                    {
                        // TODO: ES4
                        // ...
                    }                   
                }
                else // This means a new and valid ACK
                {
                    tcp_RemoveAckedPacketsFromOutBuffer(connection, tcp); 
                    connection->tcb.SND.UNA = max(connection->tcb.SND.UNA, ntohl(tcp->acknowledgmentNumber));                               
                }// end of: This means a new and valid ACK

                
                if (ntohl(tcp->sequenceNumber) == connection->tcb.RCV.NXT)
                {
                    // handle Out-of-Order IN-Buffer (sorted --> RCV buffer --> app)
                }
                else if (ntohl(tcp->sequenceNumber) > connection->tcb.RCV.NXT)
                {
                    serial_log(1,"%u ms ID %u\trcvd:\tseq:\t%u", timer_getMilliseconds(), connection->ID, ntohl(tcp->sequenceNumber) - connection->tcb.RCV.IRS);
                    serial_log(1," -> send Dup-ACK!");
                    tcp_send_DupAck(connection);

                    // Add received data to the temporary Out-of-Order In-Buffer
                    if (tcpDataLength)
                    {
                        tcpIn_t* In    = malloc(sizeof(tcpIn_t), 0, "tcp_InBuffer");
                        In->ev         = malloc(sizeof(tcpReceivedEventHeader_t) + tcpDataLength, 0, "tcp_InBuf_data");
                        memcpy(In->ev+1, tcpData, tcpDataLength);
                        In->seq        = ntohl(tcp->sequenceNumber);
                        In->ev->length = tcpDataLength;
                        In->ev->connectionID = connection->ID;

                        list_append(connection->OutofOrderinBuffer, In);
                        serial_log(1,"%u ms ID %u\tseq %u send to OutofOrderinBuffer.\r\n", timer_getMilliseconds(), connection->ID, ntohl(tcp->sequenceNumber) - connection->tcb.RCV.IRS);
                        connection->tcb.RCV.WND += tcpDataLength;

                        if (tcp->FIN) // FIN ACK
                        {
                            tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp);
                            connection->TCP_CurrState = CLOSE_WAIT;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else // ntohl(tcp->sequenceNumber) < connection->tcb.RCV.NXT
                {
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
                    #ifdef _TCP_DEBUG_
                    textColor(IMPORTANT);
                    printf("\ntcp: %u  tcpData: %u  tcpDataOff: %u\n", length, tcpDataLength, tcp->dataOffset);
                    textColor(LIGHT_BLUE);
                    for (uint16_t i=0; i<tcpDataLength; i++)
                    {
                        printf("%y ", tcpData[i]);
                    }
                    textColor(TEXT);
                    #endif
                }
                sleepMilliSeconds(2);

                connection->tcb.RCV.ACKforDupACK = connection->tcb.RCV.NXT; // TEST for Dup-ACK

                connection->tcb.RCV.WND = ntohs(tcp->window); // cf. receiving dup-ACK
                connection->tcb.SND.UNA = max(connection->tcb.SND.UNA, ntohl(tcp->acknowledgmentNumber)); // CHECK for unregular packets above

                tcp_RemoveAckedPacketsFromOutBuffer(connection, tcp); // here necessary? yes!

                if (tcpDataLength)
                {
                    connection->tcb.RCV.NXT =  ntohl(tcp->sequenceNumber) + tcpDataLength;
                    connection->tcb.SND.WND -= tcpDataLength;
                }

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
                    #ifdef _TCP_DEBUG_
                    textColor(MAGENTA);
                    printf("\ntotalTCPdataSize: %u ", totalTCPdataSize);
                    textColor(TEXT);
                    #endif

                    // setting sliding window
                    if (retVal==0)
                    {
                        #ifdef _TCP_DEBUG_
                        textColor(SUCCESS);
                        printf("ID. %u event queue OK", In->ev->connectionID);
                        #endif

                        if (totalTCPdataSize <  STARTWINDOW && connection->tcb.SND.WND < MAXWINDOW)
                            connection->tcb.SEG.WND = connection->tcb.SND.WND += INCWINDOW;
                        else if (totalTCPdataSize >= MAXWINDOW)
                            connection->tcb.SEG.WND = connection->tcb.SND.WND  = 0;
                        else if (totalTCPdataSize >= STARTWINDOW)
                            connection->tcb.SEG.WND = connection->tcb.SND.WND -= DECWINDOW;
                    }
                    else
                    {
                        #ifdef _TCP_DEBUG_
                        textColor(ERROR);
                        printf("ID. %u event queue error: %u", In->ev->connectionID, retVal);
                        #endif

                        connection->tcb.SEG.WND = connection->tcb.SND.WND  = 0;
                    }
                    textColor(TEXT);
                }

                if (tcp->FIN) // FIN ACK
                {
                    tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp);
                    connection->TCP_CurrState = CLOSE_WAIT;
                }
                else // no FIN ACK
                {
                    tcp_sendFlag = true;
                    tcp_checkOutBuffers(connection, false);
                }
                
            }// end of: ACK
            break;
        }
        case FIN_WAIT_1:
            if(tcp->FIN) // FIN or FIN ACK
            {
                tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp);
                if(tcp->ACK) // FIN ACK
                {
                    tcp_timeoutDeleteConnection(connection, 2*connection->tcb.msl);
                    connection->TCP_CurrState = TIME_WAIT;
                }
                else // FIN
                {
                    connection->TCP_CurrState = CLOSING;
                }
            }
            else if(!tcp->SYN && tcp->ACK) // No FIN, no SYN, but ACK
            {
                if(length-4*tcp->dataOffset > 0) // ACK with data
                {
                    tcp_timeoutDeleteConnection(connection, 2*connection->tcb.msl); // if there will not come a FIN at FIN_WAIT_2
                }
                connection->TCP_CurrState = FIN_WAIT_2;
            }
            break;

        case FIN_WAIT_2:
            if (tcp->FIN) // FIN
            {
                tcp_sendFlag = tcp_prepare_send_ACK(connection, tcp);
                tcp_timeoutDeleteConnection(connection, 2*connection->tcb.msl);
                connection->TCP_CurrState = TIME_WAIT;
            }
            break;

        case CLOSING:
            if (!tcp->SYN && !tcp->FIN && tcp->ACK) // ACK
            {
                tcp_timeoutDeleteConnection(connection, 2*connection->tcb.msl);
                connection->TCP_CurrState = TIME_WAIT;
            }
            break;

        case TIME_WAIT:
          #ifdef _TCP_DEBUG_
            textColor(RED);
            printf("Packet received during state TIME_WAIT.");
            textColor(TEXT);
          #endif
            break;

        case CLOSE_WAIT: // Passive Close
          #ifdef _TCP_DEBUG_
            printf("Packet received at CLOSE_WAIT?!");
          #endif
            break;

        case LAST_ACK:
            if (tcp->ACK) // ACK
            {
                connection->TCP_CurrState = CLOSED;
                tcp_deleteFlag = true;
            }
            break;

        case CLOSED:
            tcp_sendReset(connection, tcp, tcp->ACK, length);
            break;

        default: // only for test reasons
            textColor(ERROR);
            printf("This default state should not happen.");
            textColor(TEXT);
            break;
    } // switch(connection->TCP_CurrState)

    /// LOG
    serial_log(1,"%u ms ID %u\trecv: ", timer_getMilliseconds(), connection->ID);
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

static bool tcp_prepare_send_ACK(tcpConnection_t* connection, tcpPacket_t* tcp)
{
    connection->tcb.SND.UNA = max(connection->tcb.SND.UNA, ntohl(tcp->acknowledgmentNumber));
    connection->tcb.RCV.WND = ntohs(tcp->window);
    connection->tcb.RCV.NXT = ntohl(tcp->sequenceNumber)+1;
    connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
    connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
    connection->tcb.SEG.CTL = ACK_FLAG;
    return true;
}

static void tcp_send_DupAck(tcpConnection_t* connection)
{
    connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
    connection->tcb.SEG.CTL = ACK_FLAG;
    serial_log(1,"%u ms ID %u\tWe send now Dup-Ack:\r\n", timer_getMilliseconds(), connection->ID);
    serial_log(1,"\tseq:\t%u", connection->tcb.SEG.SEQ - connection->tcb.SND.ISS);
    serial_log(1,"\tack:\t%u\r\n", connection->tcb.RCV.NXT - connection->tcb.RCV.IRS);
    tcp_send(connection, 0, 0);
}

static void tcp_sendFin(tcpConnection_t* connection)
{
    connection->tcb.SEG.CTL = FIN_FLAG;
    connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
    connection->tcb.SEG.ACK = connection->tcb.RCV.NXT;
    connection->tcb.SND.NXT = connection->tcb.SEG.SEQ + 1;
    tcp_send(connection, 0, 0);
}

static void tcp_sendReset(tcpConnection_t* connection, tcpPacket_t* tcp, bool ack, uint32_t length)
{
    if (ack)
    {
        connection->tcb.SEG.SEQ = ntohl(tcp->acknowledgmentNumber);
        connection->tcb.SEG.CTL = RST_ACK_FLAG;
    }
    else
    {
        uint32_t tcpDataLength = length - (4 * tcp->dataOffset);
        connection->tcb.SEG.SEQ = 0;
        connection->tcb.SEG.ACK = ntohl(tcp->sequenceNumber) + tcpDataLength;
        connection->tcb.SEG.CTL = RST_FLAG;
    }
    tcp_send(connection, 0, 0);
}

void tcp_send(tcpConnection_t* connection, void* data, uint32_t length)
{
  #ifdef _TCP_DEBUG_
    textColor(HEADLINE);
    printf("\nTCP send: ");
    textColor(TEXT);
  #endif

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
        case RST_ACK_FLAG:
            tcp->RST = tcp->ACK =1; // RST ACK
            break;
    }

    tcp->window = htons(connection->tcb.SEG.WND);
    tcp->urgentPointer = 0;

    tcp->checksum = 0; // for checksum calculation
    tcp->checksum = htons(udptcpCalculateChecksum(tcp, length + sizeof(tcpPacket_t), connection->localSocket.IP, connection->remoteSocket.IP, 6));

    ipv4_send(connection->adapter, tcp, length + sizeof(tcpPacket_t), connection->remoteSocket.IP, 6); // tcp protocol: 6

    // increase SND.NXT
    if (connection->TCP_CurrState == ESTABLISHED && connection->tcb.retrans == false)
    {
        connection->tcb.SND.NXT += length;
    }
    /// LOG
    serial_log(1,"%u ms ID %u\tsend: ", timer_getMilliseconds(), connection->ID);
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
    free(tcp);
}

static uint32_t tcp_deleteInBuffers(tcpConnection_t* connection, list_t* list)
{
    uint32_t count = 0;
    if (connection)
    {
        tcp_logBuffers(connection, false, list); // --> COM1

        for (dlelement_t* e = list->head; e != 0; e = e->next)
        {
            count++;
            tcpIn_t* inPacket = e->data;
            free(inPacket->ev);
            free(inPacket);
        }
        list_free(list);
    }
    return count;
}

static uint32_t tcp_deleteOutBuffers(tcpConnection_t* connection)
{
    uint32_t count = 0;
    if (connection)
    {
        serial_log(1,"\r\ntcp_deleteOutBuffers");

        for (dlelement_t* e = connection->outBuffer->head; e != 0; e = e->next)
        {
            count++;
            tcpOut_t* outPacket = e->data;
            free(outPacket->data);
            free(outPacket);
        }
        list_free(connection->outBuffer);
        connection->outBuffer = 0;
    }
    return count;
}

 static bool tcp_retransOutBuffer(tcpConnection_t* connection, uint32_t seq)
 {
    for (dlelement_t* e = connection->outBuffer->head; e != 0; e = e->next)
    {
        tcpOut_t* outPacket = e->data;
        if (outPacket->segment.SEQ == seq) // searched packet found
        {
          #ifdef _TCP_DEBUG_
            textColor(LIGHT_BLUE);
            printf("\ndup-ack triggered retransmission done for seq = %u.", outPacket->segment.SEQ - connection->tcb.SND.ISS);
          #endif

            serial_log(1,"\r\nID %u\t dup-ack triggered retransmission done for seq = %u.\r\n", connection->ID, outPacket->segment.SEQ - connection->tcb.SND.ISS);
            connection->tcb.SEG.SEQ = outPacket->segment.SEQ;
            connection->tcb.SEG.ACK = outPacket->segment.ACK;
            connection->tcb.SEG.LEN = outPacket->segment.LEN;
            connection->tcb.SEG.CTL = ACK_FLAG;
            connection->tcb.retrans = true;
            tcp_send(connection, outPacket->data, connection->tcb.SEG.LEN);
            outPacket->time_ms_transmitted = timer_getMilliseconds();
            connection->tcb.retrans = false;
            connection->tcb.RCV.dACK = 0;
            connection->tcb.rto = min(2*connection->tcb.rto, RTO_MAXVALUE);
            return true;
        }
    }
  #ifdef _TCP_DEBUG_
    textColor(ERROR);
    printf("\nPacket for requested retransmission not found.");
  #endif
    serial_log(1,"Packet for requested retransmission not found.\r\n");
    textColor(TEXT);
    return false;
 }

static uint32_t tcp_checkOutBuffers(tcpConnection_t* connection, bool showData)
{
  #ifdef _TCP_DEBUG_
    printf("\n\n");
  #endif

    uint32_t count = 0;
    for (dlelement_t* e = connection->outBuffer->head; e != 0; e = e->next)
    {
        count++;
        //tcpOut_t* outPacket = e->data;

     #ifdef _TCP_DEBUG_
        printf("\nID %u  seq %u len %u (not yet acknowledged)", connection->ID, outPacket->segment.SEQ - connection->tcb.SND.ISS, outPacket->segment.LEN);
        if (showData)
        {
            textColor(DATA);
            putch('\n');
            for (uint32_t i=0; i<outPacket->segment.LEN; i++)
            {
                putch( ((char*)outPacket->data)[i] );
            }
            textColor(TEXT);
        }
      #endif

        // check need for retransmission
        /*
        if ((timer_getMilliseconds() - outPacket->time_ms_transmitted) > 2*connection->tcb.rto)
        {
            serial_log(1,"\r\nrto (%u ms) triggered retransmission done for seq=%u.\r\n", connection->tcb.rto, outPacket->segment.SEQ - connection->tcb.SND.ISS);
            connection->tcb.SEG.SEQ =  outPacket->segment.SEQ;
            connection->tcb.SEG.ACK =  outPacket->segment.ACK;
            connection->tcb.SEG.LEN =  outPacket->segment.LEN;
            connection->tcb.SEG.CTL =  ACK_FLAG;
            connection->tcb.retrans = true;
            tcp_send(connection, outPacket->data, connection->tcb.SEG.LEN);
            outPacket->time_ms_transmitted = timer_getMilliseconds();
            connection->tcb.retrans = false;
            connection->tcb.rto = min(2*connection->tcb.rto, RTO_MAXVALUE);
        }
        else
        {
            textColor(TEXT);
          #ifdef _TCP_DEBUG_
            printf("\nWe are still waiting for the ACK");
          #endif
        }
        */
    }
    return count;
}

static void tcp_RemoveAckedPacketsFromOutBuffer(tcpConnection_t* connection, tcpPacket_t* tcp)
{
    for (dlelement_t* e = connection->outBuffer->head; e != 0;)
    {
        tcpOut_t* outPacket = e->data;
        if ( ((outPacket->segment.SEQ + outPacket->segment.LEN) <= ntohl(tcp->acknowledgmentNumber)) && (outPacket->segment.LEN > 0))
        {
            // Refresh retransmission timeout (RTO)
            if (ntohl(tcp->acknowledgmentNumber) - (outPacket->segment.SEQ + outPacket->segment.LEN) == 0)
            {
                calculateRTO(connection, timer_getMilliseconds() - outPacket->time_ms_transmitted);
            }
            e = list_delete(connection->outBuffer, e->data); // Remove packet.
        }
        else
            e = e->next;
    }
}

// http://www.medianet.kent.edu/techreports/TR2005-07-22-tcp-EFSM.pdf  page 41
static bool tcp_IsPacketAcceptable(tcpPacket_t* tcp, tcpConnection_t* connection, uint16_t tcpDatalength)
{
    if (tcp->window != 0)
    {
        if (tcpDatalength)
        {
            return( (ntohl(tcp->sequenceNumber) >=  connection->tcb.RCV.NXT  &&
                         ntohl(tcp->sequenceNumber) < connection->tcb.RCV.NXT + ntohs(tcp->window)) ||
                    (ntohl(tcp->sequenceNumber) + tcpDatalength >= connection->tcb.RCV.NXT &&
                         ntohl(tcp->sequenceNumber) + tcpDatalength < ( connection->tcb.RCV.NXT + ntohs(tcp->window))) );
        }

        // tcpDatalength == 0
        return ( ntohl(tcp->sequenceNumber) >= connection->tcb.RCV.NXT &&
                 ntohl(tcp->sequenceNumber) < (connection->tcb.RCV.NXT + ntohs(tcp->window)) );
    }

    // tcp->window == 0
    if (tcpDatalength)
        return false;

    // tcp->window == 0 && tcpDatalength == 0
    return (ntohl(tcp->sequenceNumber) == connection->tcb.RCV.NXT);
}

// RTO calculation (RFC 2988)
// Variables: RTO  = retransmission timeout,   RTT    = round-trip time,
//            SRTT = smoothed round-trip time, RTTVAR = round-trip time variation
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
    connection->tcb.rto = min(connection->tcb.rto, RTO_MAXVALUE);
}

static uint16_t tcp_getFreeSocket()
{
    return(0xC000 + rand() % (0xFFFF-0xC000));
}

static uint32_t tcp_getConnectionID()
{
    static uint16_t ID = 1;
    return ID++;
}


// Debug functions
void tcp_showConnections()
{
    if(tcpConnections == 0)
        return;

    textColor(TABLE_HEADING);
    printf("\nID\tIP\t\tSrc\tDest\tAddr\t\tState");
    printf("\n--------------------------------------------------------------------------------");
    textColor(TEXT);
    for(dlelement_t* e = tcpConnections->head; e != 0; e = e->next)
    {
        tcpConnection_t* connection = e->data;
        printf("%u\t%I\t%u\t%u\t%X\t%s\n", connection->ID, connection->adapter->IP, connection->localSocket.port, connection->remoteSocket.port, connection, tcpStates[connection->TCP_CurrState]);
    }
    textColor(TABLE_HEADING);
    printf("--------------------------------------------------------------------------------\n");
    textColor(TEXT);
}

#ifdef _TCP_DEBUG_
static void printFlag(uint8_t b, const char* s)
{
    textColor(b ? LIGHT_GREEN : GRAY);
    printf("%s ", s);
}
#endif

static void tcp_debug(tcpPacket_t* tcp, bool showWnd)
{
  #ifdef _TCP_DEBUG_
    textColor(IMPORTANT);
    printf("%u ==> %u   ", ntohs(tcp->sourcePort), ntohs(tcp->destPort));
    textColor(TEXT);
    printFlag(tcp->URG, "URG"); printFlag(tcp->ACK, "ACK"); printFlag(tcp->PSH, "PSH");
    printFlag(tcp->RST, "RST"); printFlag(tcp->SYN, "SYN"); printFlag(tcp->FIN, "FIN");
    if (showWnd)
    {
        textColor(LIGHT_GRAY);
        printf("  WND = %u  ", ntohs(tcp->window));
        textColor(TEXT);
    }
  #endif
}

static void tcpShowConnectionStatus(tcpConnection_t* connection)
{
  #ifdef _TCP_DEBUG_
    textColor(IMPORTANT);
    putch(' ');
    puts(tcpStates[connection->TCP_CurrState]);
    textColor(TEXT);
  #endif
  #ifdef _NETWORK_DATA_
    printf("   conn. ID: %u   src port: %u\n", connection->ID, connection->localSocket.port);
    printf("SND.UNA = %u, SND.NXT = %u, SND.WND = %u", connection->tcb.SND.UNA, connection->tcb.SND.NXT, connection->tcb.SND.WND);
  #endif
}

static uint32_t tcp_logBuffers(tcpConnection_t* connection, bool showData, list_t* list)
{
    serial_log(1,"\r\n------------------------------------");
    uint32_t count = 0;
    for (dlelement_t* e = list->head; e != 0; e = e->next)
    {
        count++;
        tcpIn_t* inPacket = e->data;
        serial_log(1,"\r\n seq = %u\tlen = %u\tseq.nxt: %u", inPacket->seq - connection->tcb.RCV.IRS, inPacket->ev->length, inPacket->seq - connection->tcb.RCV.IRS + inPacket->ev->length);
        if (showData)
        {
            for (uint32_t i=0; i<inPacket->ev->length; i++)
            {
                serial_write(1,((char*)(inPacket->ev+1))[i]);
            }
        }
    }
    serial_log(1,"\r\n------------------------------------\r\n");
    return count;
}


// User functions
uint32_t tcp_uconnect(IP_t IP, uint16_t port)
{
    tcpConnection_t* connection     = tcp_createConnection();
    connection->remoteSocket.IP.iIP = IP.iIP;
    connection->remoteSocket.port   = port;
    connection->adapter             = network_getFirstAdapter();

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

bool tcp_usend(uint32_t ID, void* data, size_t length) // data exchange in state ESTABLISHED
{
    tcpConnection_t* connection = tcp_findConnectionID(ID);

    if(connection == 0)
    {
        textColor(ERROR);
        printf("Data could not be sent because there was no connection with ID %u.\n", ID);
        textColor(TEXT);
        return false;
    }

    if (connection->TCP_CurrState != ESTABLISHED)
    {
        textColor(ERROR);
        printf("Data are not sent outside from state ESTABLISHED.\n");
        textColor(TEXT);
        return false;
    }

    if (length == 0)
    {
        textColor(ERROR);
        printf("No data (length == 0)!\n");
        textColor(TEXT);
        return false;
    }

    if (length <= MSS && length <= connection->tcb.RCV.WND && list_isEmpty(connection->sendBuffer))
    {
        connection->tcb.SEG.CTL = ACK_FLAG;
        connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
        tcp_send(connection, data, length);

        // send to outBuffer
        tcpOut_t* outPacket = malloc(sizeof(tcpOut_t), 0, "tcp_OutBuffer");
        outPacket->data     = malloc(length, 0, "tcp_OutBuf_data");
        memcpy(outPacket->data, data, length);
        outPacket->segment.SEQ = connection->tcb.SND.NXT;
        outPacket->segment.ACK = connection->tcb.SEG.ACK;
        outPacket->segment.LEN = length;
        outPacket->segment.WND = connection->tcb.SEG.WND;
        outPacket->segment.CTL = connection->tcb.SEG.CTL;
        outPacket->time_ms_transmitted = timer_getMilliseconds();
        list_append(connection->outBuffer, outPacket); // sent data to be acknowledged ==> OutBuffer
    }
    else // we cannot send directly and have to handle sendBuffer
    {
        do
        {
            if (!list_isEmpty(connection->sendBuffer)) // sendBuffer is not empty
            {
                if (((tcpSendBufferPacket*)connection->sendBuffer->head->data)->length <= MSS &&
                    ((tcpSendBufferPacket*)connection->sendBuffer->head->data)->length <= connection->tcb.RCV.WND)
                {
                    connection->tcb.SEG.CTL = ACK_FLAG;
                    connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
                    tcp_send(connection, ((tcpSendBufferPacket*)connection->sendBuffer->head->data)->data, ((tcpSendBufferPacket*)connection->sendBuffer->head->data)->length);

                    list_delete(connection->sendBuffer,connection->sendBuffer->head);
                }
                else
                {
                    // first element in sendBuffer is too large
                    size_t sendSize = min(MSS, connection->tcb.RCV.WND);
                    tcpSendBufferPacket* packet = malloc(sizeof(tcpSendBufferPacket),0,"tcpSendBufPkt");
                    packet->data   = malloc(((tcpSendBufferPacket*)(connection->sendBuffer->head->data))->length - sendSize, 0, "tcpSendBufPkt"); // new size w/o sendSize
                    packet->length = ((tcpSendBufferPacket*)(connection->sendBuffer->head->data))->length - sendSize;
                    memcpy(packet->data, (void*)(((uintptr_t)((tcpSendBufferPacket*)(connection->sendBuffer->head->data))->data) + sendSize), ((tcpSendBufferPacket*)(connection->sendBuffer->head->data))->length - sendSize);

                    connection->tcb.SEG.CTL = ACK_FLAG;
                    connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
                    tcp_send(connection, ((tcpSendBufferPacket*)(connection->sendBuffer->head->data))->data, sendSize);

                    free(((tcpSendBufferPacket*)connection->sendBuffer->head->data)->data);
                    free(connection->sendBuffer->head->data);
                    connection->sendBuffer->head->data = packet;
                }
            }
            else // sendBuffer is empty
            {
                size_t sendSize = min(MSS, connection->tcb.RCV.WND);
                tcpSendBufferPacket* packet = malloc(sizeof(tcpSendBufferPacket),0,"tcpSendBufPkt");
                packet->data   = malloc(length - sendSize, 0, "tcpSendBufPkt");
                packet->length = length - sendSize;
                memcpy(packet->data, (void*)((uintptr_t)data + sendSize), length - sendSize);

                connection->tcb.SEG.CTL = ACK_FLAG;
                connection->tcb.SEG.SEQ = connection->tcb.SND.NXT;
                tcp_send(connection, data, sendSize);

                list_append(connection->sendBuffer, packet);
            }
            // send to outBuffer
            tcpOut_t* outPacket = malloc(sizeof(tcpOut_t), 0, "tcp_OutBuffer");
            outPacket->data     = malloc(length, 0, "tcp_OutBuf_data");
            memcpy(outPacket->data, data, length);
            outPacket->segment.SEQ = connection->tcb.SND.NXT;
            outPacket->segment.ACK = connection->tcb.SEG.ACK;
            outPacket->segment.LEN = length;
            outPacket->segment.WND = connection->tcb.SEG.WND;
            outPacket->segment.CTL = connection->tcb.SEG.CTL;
            outPacket->time_ms_transmitted = timer_getMilliseconds();
            list_append(connection->outBuffer, outPacket); // sent data to be acknowledged =OutBuffer
        }
        while (!list_isEmpty(connection->sendBuffer)); // sendBuffer is not empty
    }
    return true;
}

bool tcp_uclose(uint32_t ID)
{
    tcpConnection_t* connection = tcp_findConnectionID(ID);
    if(connection)
    {
        tcp_close(connection);
        return true;
    }
    else
    {
        textColor(ERROR);
        printf("connection with ID %u could not be found and closed.", ID);
        textColor(TEXT);
        return false;
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
