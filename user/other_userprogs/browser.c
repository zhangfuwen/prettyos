#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "dns_help.h"

int main()
{
    setScrollField(7, 46);
    printLine("================================================================================", 0, 0x0B);
    printLine("                      Pretty Browser - Network test program!                    ", 2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);
    printLine("                    F5 - reload; F6 - new file; F7 - new host                   ", 5, 0x0F);
    printLine("--------------------------------------------------------------------------------", 6, 0x0B);

    event_enable(true);
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);

    iSetCursor(0, 7);

    char* data;

    textColor(0x0F);
    printf("Enter the address (no subdirs yet!):\n");
    char hostname[100];
    gets(hostname);
    printf("Enter filename (Don't forget / ):\n");
    char filename[100];
    gets(filename);

    IP_t IP = getAddrByName(hostname);

    printf("%u.%u.%u.%u", IP.IP[0], IP.IP[1], IP.IP[2], IP.IP[3]);
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
                strcat(pStr,"GET ");
                strcat(pStr,filename);
                strcat(pStr," HTTP/1.1\r\nHost: ");
                strcat(pStr,hostname);
                strcat(pStr,"\r\nConnection: close\r\n\r\n");
                textColor(0x0A);
                puts(pStr);
                textColor(0x0F);
                tcp_send(connection, pStr, strlen(pStr));
                break;
            }
            case EVENT_TCP_RECEIVED:
            {
                textColor(0x06);
                tcpReceivedEventHeader_t* header = (void*)buffer;
                data = (void*)(header+1);
                data[header->length] = 0;
                printf("\n%s", data);
                textColor(0x0F);
                break;
            }
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
                switch(*key)
                {
                    case KEY_ESC:
                        tcp_close(connection);
                        return(0);
                    case KEY_F5:
                        printf("Reload...\n");
                        tcp_close(connection);
                        connection = tcp_connect(IP, 80);
                        printf("\nConnected (ID = %u). Wait until connection is established... ", connection);
                        break;
                    case KEY_F6:
                        printf("\nEnter filename (Don't forget / ):\n");
                        gets(filename);
                        tcp_close(connection);
                        connection = tcp_connect(IP, 80);
                        printf("\nConnected (ID = %u). Wait until connection is established... ", connection);
                        break;
                    case KEY_F7:
                        printf("\nEnter hostname:\n");
                        gets(hostname);
                        printf("\nEnter filename (Don't forget / ):\n");
                        gets(filename);
                        IP = getAddrByName(hostname);
                        tcp_close(connection);
                        connection = tcp_connect(IP, 80);
                        printf("\nConnected (ID = %u). Wait until connection is established... ", connection);
                        break;
                    default:
                        break;
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