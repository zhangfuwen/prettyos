#ifndef TCP_H
#define TCP_H

#include "network/network.h"
#include "events.h"

// http://tools.ietf.org/html/rfc793
// http://www.medianet.kent.edu/techreports/TR2005-07-22-tcp-EFSM.pdf

typedef enum {CLOSED, LISTEN, SYN_SENT, SYN_RECEIVED, ESTABLISHED, FIN_WAIT_1, FIN_WAIT_2, CLOSING, CLOSE_WAIT, LAST_ACK, TIME_WAIT} TCP_state;
typedef enum {SYN_FLAG, SYN_ACK_FLAG, ACK_FLAG, FIN_FLAG, FIN_ACK_FLAG, RST_FLAG} tcpFlags;

typedef struct
{
    uint16_t sourcePort;
    uint16_t destPort;
    uint32_t sequenceNumber;
    uint32_t acknowledgmentNumber;
    uint8_t reserved   : 4;
    uint8_t dataOffset : 4;                    // The number of 32 bit words in the TCP Header

    // Flags (6 Bit)
    uint8_t FIN : 1;                           // No more data from sender
    uint8_t SYN : 1;                           // Synchronize sequence numbers
    uint8_t RST : 1;                           // Reset
    uint8_t PSH : 1;                           // Push
    uint8_t ACK : 1;                           // Acknowledgment
    uint8_t URG : 1;                           // Urgent
    // only shown in wireshark
    uint8_t ECN : 1;                           // ECN (reserved)
    uint8_t CWR : 1;                           // CWR (reserved)

    uint16_t window;
    uint16_t checksum;
    uint16_t urgentPointer;
} __attribute__((packed)) tcpPacket_t;

typedef struct
{
    uint32_t SEQ; // Sequence number
    uint32_t ACK; // Acknoledgement number
    uint32_t LEN; // segment length
    uint32_t WND; // segment windows
    tcpFlags CTL; // control bits
} tcpSegment_t;

typedef struct
{
    uint32_t UNA;   // Send Unacknowledged
    uint32_t NXT;   // Send Next
    uint16_t WND;   // Send Window
    uint32_t ISS;   // Initial send sequence number
} tcpSend_t;

typedef struct
{
    uint32_t NXT;   // Sequence number of next received set
    uint16_t WND;   // Receive Window
    uint32_t IRS;   // Initial receive sequence number
} tcpRcv_t;


typedef struct
{
    tcpSend_t SND;
    tcpRcv_t  RCV;
    tcpSegment_t SEG;   // information about segment to be sent next
} tcpTransmissionControlBlock_t;

typedef struct
{
    uint16_t port;
    IP_t     IP;
} tcpSocket_t;

typedef struct
{
    uint32_t ID;
    tcpSocket_t localSocket;
    tcpSocket_t remoteSocket;
    struct network_adapter* adapter;
    tcpTransmissionControlBlock_t tcb;
    TCP_state TCP_PrevState;
    TCP_state TCP_CurrState;
    task_t* owner;
    list_t* inBuffer;
    list_t* outBuffer;
} tcpConnection_t;

typedef struct
{
    uint32_t connection;
    size_t   length;
} __attribute__((packed)) tcpReceivedEventHeader_t;

typedef struct
{
    uint32_t  seq;
    size_t    length;
    tcpReceivedEventHeader_t* ev;	
} tcpIn_t;

typedef struct
{
    size_t    length;
    void*     data;
}__attribute__((packed)) tcpOut_t;


tcpConnection_t* tcp_createConnection();
void tcp_deleteConnection(tcpConnection_t* connection);
void tcp_bind(tcpConnection_t* connection, network_adapter_t* adapter);
void tcp_connect(tcpConnection_t* connection);
void tcp_close(tcpConnection_t* connection);
void tcp_receive(network_adapter_t* adapter, tcpPacket_t* tcp, IP_t transmittingIP, size_t length);
void tcp_send(tcpConnection_t* connection, void* data, uint32_t length);
void tcp_showConnections();
tcpConnection_t* findConnectionID(uint32_t ID);
tcpConnection_t* findConnection(IP_t IP, uint16_t port, network_adapter_t* adapter, bool established);
uint32_t tcp_showInBuffers(tcpConnection_t* connection);

// User functions
uint32_t tcp_uconnect(IP_t IP, uint16_t port);
void     tcp_usend(uint32_t ID, void* data, size_t length);
void     tcp_uclose(uint32_t ID);


#endif
