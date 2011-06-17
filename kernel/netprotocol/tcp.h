#ifndef TCP_H
#define TCP_H

#include "netprotocol/networktypes.h"

// http://tools.ietf.org/html/rfc793
// http://www.medianet.kent.edu/techreports/TR2005-07-22-tcp-EFSM.pdf


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
    uint8_t src[4];
    uint8_t dest[4];
    uint8_t res;
    uint8_t prot;
    uint16_t length;
} __attribute__((packed)) tcpPseudoHeader_t;

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
} __attribute__((packed)) tcpSocket_t;

typedef struct
{
    tcpSocket_t localSocket;
    tcpSocket_t remoteSocket;
    tcpTransmissionControlBlock_t tcb;
    TCP_state TCP_PrevState;
    TCP_state TCP_CurrState;
} __attribute__((packed)) tcpConnection_t;

typedef struct
{
    uint32_t SEG_SEQ; // Sequence number
    uint32_t SEG_ACK; // Acknoledgement number
    uint32_t SEG_LEN; // segment length
    uint32_t SEG_WND; // segment windows
    tcpFlags SEG_CTL; // control bits
}  __attribute__((packed)) tcpSegment_t;


// Binds the connection to a local portnumber and IP address.
void tcpBind(struct network_adapter* adapter);

// Set the state of the connection to be LISTEN.
void tcpListen(struct network_adapter* adapter);

// Connects to another host, and set the state of the connection to be SYN_SENT
void tcpConnect(struct network_adapter* adapter, uint16_t srcPort, uint16_t destPort, uint8_t destIP[4]);

void tcpClose(struct network_adapter*, uint16_t srcPort, uint16_t destPort, uint8_t destIP[4]);

void tcpReceive(struct network_adapter* adapter, tcpPacket_t* tcp, uint8_t transmittingIP[4], size_t length);
void tcpSend(struct network_adapter* adapter, void* data, uint32_t length, uint16_t srcPort, uint8_t srcIP[4], uint16_t destPort, uint8_t destIP[4], tcpFlags flags, uint32_t seqNumber, uint32_t ackNumber);

uint16_t getFreeSocket();


#endif
