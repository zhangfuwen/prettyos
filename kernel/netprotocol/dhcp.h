#ifndef DHCP_H
#define DHCP_H

#include "network/network.h"

// http://tools.ietf.org/html/rfc2131 <--- Dynamic Host Configuration Protocol
// ftp://ftp.efo.ru/pub/wiznet/W5100_App%20note_DHCP.pdf

// DCHP is running on PORT 67
// DHCP packets are in Type 0x800 = IPv4

/*
   DHCP clarifies the interpretation of the 'siaddr' field as the
   address of the server to use in the next step of the client's
   bootstrap process.  A DHCP server may return its own address in the
   'siaddr' field, if the server is prepared to supply the next
   bootstrap service (e.g., delivery of an operating system executable
   image).  A DHCP server always returns its own address in the 'server
   identifier' option.

   FIELD      OCTETS       DESCRIPTION
   -----      ------       -----------

   op            1  Message op code / message type.
                    1 = BOOTREQUEST, 2 = BOOTREPLY
   htype         1  Hardware address type, see ARP section in "Assigned
                    Numbers" RFC; e.g., '1' = 10mb ethernet.
   hlen          1  Hardware address length (e.g.  '6' for 10mb
                    ethernet).
   hops          1  Client sets to zero, optionally used by relay agents
                    when booting via a relay agent.
   xid           4  Transaction ID, a random number chosen by the
                    client, used by the client and server to associate
                    messages and responses between a client and a
                    server.
   secs          2  Filled in by client, seconds elapsed since client
                    began address acquisition or renewal process.
   flags         2  Flags (see figure 2).
   ciaddr        4  Client IP address; only filled in if client is in
                    BOUND, RENEW or REBINDING state and can respond
                    to ARP requests.
   yiaddr        4  'your' (client) IP address.
   siaddr        4  IP address of next server to use in bootstrap;
                    returned in DHCPOFFER, DHCPACK by server.
   giaddr        4  Relay agent IP address, used in booting via a
                    relay agent.
   chaddr       16  Client hardware address.
   sname        64  Optional server host name, null terminated string.
   file        128  Boot file name, null terminated string; "generic"
                    name or null in DHCPDISCOVER, fully qualified
                    directory-path name in DHCPOFFER.
   options     var  Optional parameters field.  See the options
                    documents for a list of defined options.

           Table 1:  Description of fields in a DHCP message

   The 'options' field is now variable length. A DHCP client must be
   prepared to receive DHCP messages with an 'options' field of at least
   length 312 octets.  This requirement implies that a DHCP client must
   be prepared to receive a message of up to 576 octets, the minimum IP
*/

typedef struct dhcp     // complete length: 576 (0x0240) 
{
    uint8_t  op;        // DHCP_BOOTREQEUST or DHCP_BOOTREPLY
    uint8_t  htype;     // DHCP_HTYPE10MB
    uint8_t  hlen;      // DHCP_HLENETHERNET
    uint8_t  hops;      // DHCP_HOPS
    uint32_t xid;       // DHCP_XID
    uint16_t secs;      // DHCP_SECS
    uint16_t flags;     // DHCP_FLAGSBROADCAST
    uint8_t  ciaddr[4];
    uint8_t  yiaddr[4];
    uint8_t  siaddr[4];
    uint8_t  giaddr[4];
    uint8_t  chaddr[16];
    char     sname[64];
    char     file[128];
    uint8_t  options[340]; 
} __attribute__((packed)) dhcp_t;

/*
   Message         Use
   --------------------------------------------------------------------

   DHCPDISCOVER -  Client broadcast to locate available servers.

   DHCPOFFER    -  Server to client in response to DHCPDISCOVER with
                   offer of configuration parameters.

   DHCPREQUEST  -  Client message to servers either (a) requesting
                   offered parameters from one server and implicitly
                   declining offers from all others, (b) confirming
                   correctness of previously allocated address after,
                   e.g., system reboot, or (c) extending the lease on a
                   particular network address.

   DHCPACK      -  Server to client with configuration parameters,
                   including committed network address.

   DHCPNAK      -  Server to client indicating client's notion of network
                   address is incorrect (e.g., client has moved to new
                   subnet) or client's lease as expired

   DHCPDECLINE  -  Client to server indicating network address is already
                   in use.

   DHCPRELEASE  -  Client to server relinquishing network address and
                   cancelling remaining lease.

   DHCPINFORM   -  Client to server, asking only for local configuration
                   parameters; client already has externally configured
                   network address.
*/

void DHCP_AnalyzeServerMessage();
void DHCP_Discover(network_adapter_t* adapter);
void DHCP_Request();

/*
When a client is initialized for the first time after it is configured
to receive DHCP information, it initiates a conversation with the server.
Below is a summary table of the conversation between client and server,
which is followed by a packet-level description of the process:

   Source     Dest        Source     Dest              Packet
   MAC addr   MAC addr    IP addr    IP addr           Description
   -----------------------------------------------------------------
   Client     Broadcast   0.0.0.0    255.255.255.255   DHCP Discover
   DHCPsrvr   Broadcast   DHCPsrvr   255.255.255.255   DHCP Offer
   Client     Broadcast   0.0.0.0    255.255.255.255   DHCP Request
   DHCPsrvr   Broadcast   DHCPsrvr   255.255.255.255   DHCP ACK
*/

#endif
