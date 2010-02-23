#include "os.h"
#include "ipTcpStack.h"

// This is (normally) called from a Network-Card-Driver
// It interprets the whole data (also the Ethernet-Header)

// Parameters:
//  Data  : A pointer to the Data
//  Length: The Size of the Paket

// Internal:
//  TODO
void ipTcpStack_recv(void* Data, uint32_t Length)
{
  struct ethernet* eth;
  struct arp*      arp;
  struct ip*       ip;

  // first we cast our Data pointer into a pointer at our Ethernet-Frame
  eth = (struct ethernet*)Data;
  settextcolor(14,0); printformat("--- TCP-IP stack: ---\n"); settextcolor(15,0);

  // we dump the Data
  for(uint32_t c = 0; c < Length; c++ )
  {
    printformat("%y ", ((char*)Data)[c]);
  }

  // we let the source-mac and dest-mac like they are...
  // DEBUG< we just print them >
  settextcolor(14,0); printformat("\nEthernet header:\n"); settextcolor(15,0);
  printformat("Sender  MAC: %y %y %y %y %y %y\n", eth->send_mac[0], eth->send_mac[1], eth->send_mac[2], eth->send_mac[3], eth->send_mac[4], eth->send_mac[5]);
  printformat("Reciver MAC: %y %y %y %y %y %y\n", eth->recv_mac[0], eth->recv_mac[1], eth->recv_mac[2], eth->recv_mac[3], eth->recv_mac[4], eth->recv_mac[5]);

  // now we check if it is Ethernet 1 or Ethernet 2
  // (but we just throw it away, because we can read the length of the data from the other Layers)
  if( ( (eth->type_len[0] << 8) | eth->type_len[1] ) > 1500 )
  {
    settextcolor(14,0); printformat("\nEthernet 2 Paket\n"); settextcolor(15,0);
  }
  else
  {
    settextcolor(14,0); printformat("\nEthernet 1 Paket\n"); settextcolor(15,0);
  }

  // now we set our arp/ip pointer to the Ethernet-payload
  arp = (struct arp*)( (unsigned long)eth + sizeof(struct ethernet) );
  ip  = (struct ip* )( (unsigned long)eth + sizeof(struct ethernet) );

  // to decide if it is an ip or an arp paket we just look at the ip-version
  if(  (ip->version_ihl & 0xF) == 4 )
  {
    settextcolor(14,0); printformat("IPv4 Paket:\n"); settextcolor(15,0);
  }
  else if( (ip->version_ihl & 0xF) == 6 )
  {
    settextcolor(14,0); printformat("IPv6 Paket:\n"); settextcolor(15,0);
  }
  else
  {
    // we decide _now_ that it could be an arp paket
    // ASK< any other ideas to test for the type of the protocol? >

    // now we check if it is realy an ipv4 ARP paket
    if( ( ( (arp->hardware_adresstype[0] << 8) | arp->hardware_adresstype[1]  ) ==      1 ) &&
        ( ( (arp->protocol_adresstype[0] << 8) | arp->protocol_adresstype[1] )  == 0x2048 ) &&
        ( arp->hardware_adresssize                                              ==      6 ) &&
        ( arp->protocol_adresssize                                              ==      4 )
      )
      {
        settextcolor(14,0); printformat("ARP Paket:\n"); settextcolor(15,0);

        // extract the operation
        uint16_t operation = (arp->operation[0] << 8) | arp->operation[1];

        // print the operation
        if( operation == 1 )
        { // it is an ARP-Request
            settextcolor(14,0); printformat("Operation: Request\n"); settextcolor(15,0);
        }
        else if( operation == 2 )
        { // it is an ARP-Response
            settextcolor(14,0); printformat("Operation: Response\n"); settextcolor(15,0);
        }
      }
      else
      {
          // NOTE< here we ignore silently other paketes that we don't know >
      }
  }
  settextcolor(14,0); printformat("--- TCP-IP stack: ---\n"); settextcolor(15,0);
}

