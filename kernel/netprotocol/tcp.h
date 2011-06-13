#ifndef TCP_H
#define TCP_H

#include "network/network.h"

// http://tools.ietf.org/html/rfc793 
// http://www.medianet.kent.edu/techreports/TR2005-07-22-tcp-EFSM.pdf

typedef enum {SYN_FLAG, SYN_ACK_FLAG, ACK_FLAG, FIN_FLAG, RST_FLAG} tcpFlags;

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


// Binds the connection to a local portnumber and IP address.
void tcpBind();

// Set the state of the connection to be LISTEN.
void tcpListen();

// Connects to another host.
void tcpConnect();

void tcpReceive(network_adapter_t* adapter, tcpPacket_t* tcp, uint8_t transmittingIP[4]);
void tcpSend(network_adapter_t* adapter, void* data, uint32_t length, uint16_t srcPort, uint8_t srcIP[4], uint16_t destPort, uint8_t destIP[4], tcpFlags flags, uint32_t seqNumber, uint32_t ackNumber);


#endif
