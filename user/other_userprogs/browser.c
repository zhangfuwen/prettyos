#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"


int main()
{
    setScrollField(7, 46);
    printLine("================================================================================", 0, 0x0B);
    printLine("                     Pretty Browser - Network test program!",                      2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);
	
	event_enable(true);
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);
	
	
	
    iSetCursor(0, 7);
	
	textColor(0x0F);
	printf("Enter the address (no subdirs yet!):\n");
	char hostname[100];
	gets(hostname);
	
    IP_t IP = resolveIP(hostname);
	
	/*
    char strIP[4][10];
    for (uint8_t i = 0; i<4; i++)
    {        
        printf("IP[%u]: ",i);
        IP.IP[i] = atoi(gets(strIP[i]));
    }
    printf("\n%u.%u.%u.%u\n", IP.IP[0],IP.IP[1],IP.IP[2],IP.IP[3]);
    */
	
    uint32_t connection = tcp_connect(IP, 80);
    printf("\nConnected (ID = %u). Wait until connection is established... ", connection);
    
    
    for(;;)
    {
        switch(ev)
        {
            case EVENT_NONE:
                waitForEvent(0);
                break;
            case EVENT_TCP_CONNECTED:
            {
                printf("ESTABLISHED.\n");
                
                char pStr[200];
				memset(pStr,0,200);
                strcat(pStr,"GET / HTTP/1.1\r\nHost: ");
				strcat(pStr,hostname);
                strcat(pStr,"\r\nConnection: close\r\n\r\n");
                textColor(0x0A);
                printf("%s",pStr);
                textColor(0x0F);
                tcp_send(connection, pStr, strlen(pStr));
                break;
            }
            case EVENT_TCP_RECEIVED:
            {
                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;
                printf("\npacket received. Length = %u\n:%s", header->length, data);
                break;
            }
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
                if(*key == KEY_ESC)
                {
                    tcp_close(connection);
                    return(0);
                }
                break;
            }
            default:
                break;
        }
        ev = event_poll(buffer, 4096, EVENT_NONE);
    }

    tcp_close(connection);
    return(0);
}
