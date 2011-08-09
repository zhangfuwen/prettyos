#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"

int main()
{
    setScrollField(7, 46);
    printLine("================================================================================", 0, 0x0B);
    printLine("                           neuer_user FTP Client v0.1                           ", 2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);

    event_enable(true);
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);

    iSetCursor(0, 7);
    textColor(0x0F);

    char hostname[100];
    char user[100];
    char pass[100];
    printf("Server:\n");
    gets(hostname);
    printf("Benutzer:\n");
    gets(user);
    printf("Passwort:\n");
    gets(pass);

    uint32_t dataConnection = 0;
    IP_t dataIP;
    uint16_t dataPort;

    IP_t IP = resolveIP(hostname);
    uint32_t control = tcp_connect(IP, 21);
    printf("\nConnected (ID = %u). Wait until connection is established... ", control);

    for(;;)
    {
        switch(ev)
        {
            case EVENT_NONE:
                waitForEvent(0);
                break;
            case EVENT_TCP_CONNECTED:
            {
                textColor(0x0A);
                printf("\nESTABLISHED.\n\n");
                break;
            }
            case EVENT_TCP_CLOSED:
            {
                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;
                if(header->connectionID == dataConnection)
                {
                    textColor(0x07);
                    printf("Closed data-connection.\n");
                }
                break;
            }
            case EVENT_TCP_RECEIVED:
            {
                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;

                if(header->connectionID == dataConnection)
                    textColor(0x0C);
                else if(header->connectionID == control)
                    textColor(0x09);
                printf("%s\n", data);

                textColor(0x0F);

                if(header->connectionID == control)
                {
                    if(data[0] == '2' && data[1] == '2' && data[2] == '0')
                    {
                        char pStr[200];
                        memset(pStr,0,200);
                        strcat(pStr,"USER ");
                        strcat(pStr,user);
                        strcat(pStr,"\r\n");
                        tcp_send(control, pStr, strlen(pStr));
                    }
                    else if(data[0] == '3' && data[1] == '3' && data[2] == '1')
                    {
                        char pStr[200];
                        memset(pStr,0,200);
                        strcat(pStr,"PASS ");
                        strcat(pStr,pass);
                        strcat(pStr,"\r\n");
                        tcp_send(control, pStr, strlen(pStr));
                    }
                    else if(data[0] == '2' && data[1] == '3' && data[2] == '0')
                    {
                        printf("Loggin successful.\n");
                    }
                    else if(data[0] == '2' && data[1] == '2' && data[2] == '6')
                    {
                        tcp_close(dataConnection);
                    }
                    else if(data[0] == '2' && data[1] == '2' && data[2] == '7')
                    {
                        uint8_t temp[6];
                        uint8_t it = 3;
                        do
                        {
                            if(data[it] == '(')
                                break;
                        }while(it++);
                        for(uint8_t i_start = it+1, i_end = it+1, byte = 0; byte < 6; i_end++)
                        {
                            if(data[i_end] == ')')
                            {
                                temp[byte] = atoi(data+i_start);
                                break;
                            }

                            if(data[i_end] == ',')
                            {
                                data[i_end] = 0;
                                temp[byte] = atoi(data+i_start);
                                i_start = i_end+1;
                                byte++;
                            }
                        }
                        for(int i = 0;i < 4;i++)
                            dataIP.IP[i] = temp[i];
                        dataPort = temp[4]*256+temp[5];

                        dataConnection = tcp_connect(dataIP, dataPort);
                        printf("\nConnected (ID = %u). Wait until connection is established... ", dataConnection);
                    }
                }
                else if(header->connectionID == dataConnection)
                {
                }
                break;
            }
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
                if(*key == KEY_ESC)
                {
                    printf("quit...");
                    tcp_send(control, "QUIT\r\n", 8);
                    tcp_close(control);
                    return(0);
                }
                else if(*key == KEY_F8)
                {
                    textColor(0x0F);
                    printf("Enter command:\n");
                    char pStr[200];
                    memset(pStr,0,200);
                    char command[50];
                    gets(command);
                    strcat(pStr,command);
                    strcat(pStr,"\r\n");
                    tcp_send(control, pStr, strlen(pStr));
                }
                break;
            }
            default:
                break;
        }
        ev = event_poll(buffer, 4096, EVENT_NONE);
    }

    tcp_close(control);
    return(0);
}
