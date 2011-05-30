#ifndef TCP_H
#define TCP_H

// http://tools.ietf.org/html/rfc793 <--- Transmission Control Protocol

#include "network/network.h"


// http://de.wikipedia.org/wiki/Transmission_Control_Protocol#Erl.C3.A4uterung

typedef struct
{
    uint16_t sourcePort;
    uint16_t destinationPort;
    uint32_t sequenceNumber;
    uint32_t acknowledgmentNumber;
    uint8_t dataOffset : 4;
    uint8_t reserved   : 4;

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

void tcpRecv();
void tcpSend();

void tcpDebug(tcpPacket_t* tcp);


#endif
