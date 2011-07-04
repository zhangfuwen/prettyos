#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"


int main()
{
    setScrollField(7, 46);
    printLine("================================================================================", 0, 0x0B);
    printLine("                     Pretty IRC - Network test program!",                          2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);

    iSetCursor(0, 7);
    IP_t IP = {.IP = {151,189,0,165}}; // euirc
    uint32_t connection = tcp_connect(IP, 6667); // IRC
    printf("\nConnected (ID = %u). Wait until connection is established... ", connection);

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
                char* pStr = "NICK Pretty00001\r\nUSER Pretty00001 irc.bre.de.euirc.net servername : Pretty00001\r\n";
                tcp_send(connection, pStr, strlen(pStr));
				break;
            case EVENT_TCP_RECEIVED:
            {
                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;
                printf("\npacket received. Length = %u\n:%s", header->length, data);
				sleep(2000);				
				char string[header->length];
				strncpy(string,data,header->length);
                for (int i=0; i<header->length; i++)
				{						
					if (strncmp(string+i,"PING",4)==0)
					{
						(string+i)[1] = 'O';						
						tcp_send(connection, (string+i), header->length - i);
					}					
				}	
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
				if(*key == KEY_P)
				{
					tcp_send(connection, "JOIN #PrettyOS\r\n", strlen("JOIN #PrettyOS\r\n"));
				}
				if(*key == KEY_L)
				{
					tcp_send(connection, "JOIN #lost\r\n", strlen("JOIN #lost\r\n"));
				}
				if(*key == KEY_H || *key == KEY_I)
				{
					printf("\nEnter message: ");
					char str[200], msg[240];
					gets(str);
					if(*key == KEY_H)
					{
						strcpy(msg,"PRIVMSG #PrettyOS :");
					}
					if(*key == KEY_I)
					{
						strcpy(msg,"PRIVMSG #lost :");
					}
					const char* msgBehind = "\r\n";
					strcat(msg, str);
					strcat(msg, msgBehind);					
					tcp_send(connection, msg, strlen(msg));
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
