#include "userlib.h"
#include "stdio.h"
#include "string.h"


int main()
{
    setScrollField(7, 46);
    printLine("================================================================================", 0, 0x0B);
    printLine("                     Pretty UDPsend - Network test program!",                      2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);

    iSetCursor(0, 7);


    IP_t destIP = {.IP = {127, 0, 0, 1}};

    uint16_t srcPort  = 8084;
    uint16_t destPort = 8085;

    char String[1000];
    for (uint32_t i=1; i<=500; i++)
    {
        sprintf(String, "udp send wird getestet. Count = %u\r\n", i);
        udp_send((void*)String, strlen(String), destIP, srcPort, destPort);
        puts(String);
        sleep(50);
    }

    return(0);
}
