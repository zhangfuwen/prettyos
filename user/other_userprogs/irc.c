#include "userlib.h"
#include "stdio.h"
#include "string.h"


int main()
{
    setScrollField(7, 46);
    printLine("================================================================================", 0, 0x0B);
    printLine("                       Pretty IRC - Network test program!",                        2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);

    iSetCursor(0, 7);
    IP_t IP = {.IP = {151,189,0,165}}; // euirc
    uint32_t connection = tcp_connect(IP, 6667); // IRC
    printf("\nConnected (ID = %u). Wait until connection is established... ", connection);

    event_enable(true);
    char buffer[2048];
    EVENT_t ev = event_poll(buffer, 2048, EVENT_NONE);

    bool ctrl = false;
    for(;;)
    {
        switch(ev)
        {
            case EVENT_NONE:
                //wait(BL_EVENT, (void*)EVENT_TEXT_ENTERED, 0); // TODO: Why does it cause problems?
                break;
            case EVENT_TCP_CONNECTED:
                printf("ESTABLISHED.\n");
                const char* const pStr = "NICK Pretty00002\r\nUSER Pretty00002 irc.bre.de.euirc.net servername : Pretty00002\r\n";
                tcp_send(connection, (void*)pStr, strlen(pStr));
                break;
            case EVENT_TCP_RECEIVED:
            {
                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;
                printf("\npacket received. Length = %u\n:%s", header->length, data);
                for (size_t i = 0; i < header->length; i++)
                {
                    if (strncmp(data+i, "PING", 4)==0)
                    {
                        data[i+1] = 'O';
                        tcp_send(connection, data+i, header->length - i);
                    }
                }
                break;
            }
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (KEY_t*)buffer;
                switch(*key)
                {
                    case KEY_LCTRL: case KEY_RCTRL:
                        ctrl = true;
                        break;
                    case KEY_ESC:
                        tcp_close(connection);
                        return(0);
                    case KEY_J:
                    {
                        if(ctrl)
                        {
                            printf("\nEnter Channel name: ");
                            getchar(); // Ommit first character, because its a 'j'
                            char str[50];
                            gets(str);
                            char msg[70];
                            snprintf(msg, 70, "JOIN %s\r\n", str);
                            tcp_send(connection, msg, strlen(msg));
                        }
                        break;
                    }
                    case KEY_P: case KEY_L:
                    {
                        if(ctrl)
                        {
                            printf("\nEnter message: ");
                            getchar(); // Ommit first character, because its a 'p' or a 'l'
                            char str[200], msg[240];
                            gets(str);

                            if(*key == KEY_P)
                                snprintf(msg, 240, "PRIVMSG #PrettyOS :%s\r\n", str);
                            else if(*key == KEY_L)
                                snprintf(msg, 240, "PRIVMSG #lost :%s\r\n", str);

                            tcp_send(connection, msg, strlen(msg));
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case EVENT_KEY_UP:
                switch(*(KEY_t*)buffer)
                {
                    case KEY_LCTRL: case KEY_RCTRL:
                        ctrl = false;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
        ev = event_poll(buffer, 4096, EVENT_NONE);
    }

    tcp_close(connection);
    return(0);
}
