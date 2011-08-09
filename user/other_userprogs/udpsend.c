#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"


int main()
{
    setScrollField(7, 46);
    printLine("================================================================================", 0, 0x0B);
    printLine("                     Pretty UDPsend - Network test program!",                      2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);

    iSetCursor(0, 7);

    const char* pStr = "udp send wird getestet.\r\n";
    
    IP_t destIP;
    destIP.IP[0] =  127;
    destIP.IP[1] =    0;
    destIP.IP[2] =    0;
    destIP.IP[3] =    1;

    uint16_t srcPort  = 8084;
    uint16_t destPort = 8085;

    for(uint8_t i=0; i<100; i++)
    {
        udp_send((void*)pStr, strlen(pStr), destIP, srcPort, destPort); 
        printf("\nudp data send."); 
        sleep(1000);
    }
       
    return(0);
}
