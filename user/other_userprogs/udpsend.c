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


    IP_t destIP;
    destIP.IP[0] =  127;
    destIP.IP[1] =    0;
    destIP.IP[2] =    0;
    destIP.IP[3] =    1;

    uint16_t srcPort  = 8084;
    uint16_t destPort = 8085;

    for(uint32_t i=1; i<=500; i++)
    {
        static char String[1000];
        static char number[20];
        strcpy (String,"udp send wird getestet. Count = ");
        strcat(String,itoa(i,number));
        strcat(String,"\r\n");
        udp_send((void*)String, strlen(String), destIP, srcPort, destPort);
        printf("%s",String);
        sleep(50);
    }

    return(0);
}
