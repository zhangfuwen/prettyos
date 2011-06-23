#ifndef TCP_H
#define TCP_H

#include "network/network.h"

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
    uint32_t SND_UNA;   // Send Unacknowledged
    uint32_t SND_NXT;   // Send Next
    uint16_t SND_WND;   // Send Window
    uint32_t SND_ISS;   // Initial send sequence number
    uint32_t RCV_NXT;   // Sequence number of next received set
    uint16_t RCV_WND;   // Receive Window
    uint32_t RCV_IRS;   // Initial receive sequence number
} __attribute__((packed)) tcpTransmissionControlBlock_t;

typedef struct
{
    uint16_t port;
    uint8_t  IP[4];
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
} tcpConnection_t;

typedef struct
{
    uint32_t SEG_SEQ; // Sequence number
    uint32_t SEG_ACK; // Acknoledgement number
    uint32_t SEG_LEN; // segment length
    uint32_t SEG_WND; // segment windows
    tcpFlags SEG_CTL; // control bits
} __attribute__((packed)) tcpSegment_t;


tcpConnection_t* tcp_createConnection();
void tcp_deleteConnection(tcpConnection_t* connection);
void tcp_bind(tcpConnection_t* connection, network_adapter_t* adapter);
void tcp_connect(tcpConnection_t* connection);
void tcp_close(tcpConnection_t* connection);
void tcp_receive(network_adapter_t* adapter, tcpPacket_t* tcp, uint8_t transmittingIP[4], size_t length);
void tcp_send(tcpConnection_t* connection, void* data, uint32_t length, tcpFlags flags, uint32_t seqNumber, uint32_t ackNumber);
void tcp_showConnections();
tcpConnection_t* findConnectionID(uint32_t ID);

#endif
