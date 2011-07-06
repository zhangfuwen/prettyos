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

    iSetCursor(0, 7);
    IP_t IP = {.IP = {82,100,220,68}}; // homepage ehenkes, HTTP
    uint32_t connection = tcp_connect(IP, 80);
    printf("Connected (ID = %u). Wait until connection is established... ", connection);

    event_enable(true);
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);
    for(;;)
    {
        switch(ev)
        {
            case EVENT_NONE:
                //wait(BL_EVENT, (void*)EVENT_TEXT_ENTERED, 0); // TODO: Why does it cause problems?
                break;
            case EVENT_TCP_CONNECTED:
                printf("ESTABLISHED.\n");
                char* pStr = "GET /OS_Dev/PrettyOS.htm HTTP/1.1\r\nHost: www.henkessoft.de\r\nConnection: close\r\n\r\n";
                tcp_send(connection, pStr, strlen(pStr));
                break;
            case EVENT_TCP_RECEIVED:
            {
                tcpReceivedEvent_t* evt = (void*)buffer;
                printf("\npacket received. Length = %u\n:%s", evt->header.length, evt->buffer);
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
            }
            default:
                break;
        }
        ev = event_poll(buffer, 4096, EVENT_NONE);
    }

    tcp_close(connection);
    return(0);
}
