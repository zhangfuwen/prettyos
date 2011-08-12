#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int main()
{
    setScrollField(13, 46);
    printLine("================================================================================", 0, 0x0B);
    printLine("                           neuer_user FTP Client v0.2                           ", 1, 0x0B);
    printLine("--------------------------------------------------------------------------------", 3, 0x0B);

    iSetCursor(0, 4);
    textColor(0x0F);

    printf("Implemented functions:\n");
    printf("-F1: Get file\t\t\t\t-F2: [Don't use]\n");
    printf("-F3: Rename File\t\t\t-F4: Delete File\n");
    //
    printf("-F5: Create Directory\t\t\t-F6: Remove Directory\n");
    printf("-F7: Change current directory\t\t-F8: Get current directory\n");
    //
    printf("-F9: List files/directories\n");
    //
    printf("-F12: Enter FTP-Command\n");
    printf("--------------------------------------------------------------------------------\n\n");

    event_enable(true);
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);

    char hostname[100];
    char user[100];
    char pass[100];
    printf("Server:\n");
    gets(hostname);
    printf("Benutzer:\n");
    gets(user);
    printf("Passwort:\n");
    gets(pass);

    char command[200];
    size_t waitingDataCommand = 0;
    size_t renaming = 0;
    size_t enterPasvMode = 0;

    IP_t dataIP;
    uint16_t dataPort;
    uint32_t dataConnection = 0;

    IP_t IP = resolveIP(hostname);
    uint32_t control = tcp_connect(IP, 21);
    printf("\nConnected (ID = %u). Wait until connection is established... ", control);

    for (;;)
    {
        switch (ev)
        {
            case EVENT_NONE:
                waitForEvent(0);
                break;
            case EVENT_TCP_CONNECTED:
            {
                textColor(0x0A);
                printf("\nESTABLISHED.\n\n");

                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;
                if (header->connectionID == dataConnection && waitingDataCommand)
                {
                    waitingDataCommand = 0;
                    tcp_send(control, command, strlen(command));
                }
                break;
            }
            case EVENT_TCP_CLOSED:
            {
                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;
                if (header->connectionID == dataConnection)
                {
                    textColor(0x07);
                    printf("Closed dataConnection.\n");
                }
                break;
            }
            case EVENT_TCP_RECEIVED:
            {
                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;

                if (header->connectionID == dataConnection)
                    textColor(0x06);
                else if (header->connectionID == control)
                    textColor(0x09);
                printf("%s\n", data);

                textColor(0x0F);

                if (header->connectionID == control)
                {
                    if (data[0] == '2' && data[1] == '2' && data[2] == '0')
                    {
                        char pStr[200];
                        memset(pStr,0,200);
                        strcat(pStr,"USER ");
                        strcat(pStr,user);
                        strcat(pStr,"\r\n");
                        tcp_send(control, pStr, strlen(pStr));
                    }
                    else if (data[0] == '2' && data[1] == '3' && data[2] == '0')
                    {
                        printf("Loggin successful.\n");
                        tcp_send(control, "OPTS UTF8 ON\r\n", 14);
                    }
                    else if (data[0] == '2' && data[1] == '0' && data[2] == '0')
                    {
                        if (enterPasvMode)
                        {
                            enterPasvMode = 0;
                            tcp_send(control, "PASV\r\n", 6);
                        }
                    }
                    else if (data[0] == '2' && data[1] == '2' && data[2] == '6')
                    {
                        tcp_close(dataConnection);
                    }
                    else if (data[0] == '2' && data[1] == '2' && data[2] == '7')
                    {
                        uint8_t temp[6];
                        uint8_t it = 3;
                        do
                        {
                            if (data[it] == '(')
                                break;
                        }while (it++);
                        for (uint8_t i_start = it+1, i_end = it+1, byte = 0; byte < 6; i_end++)
                        {
                            if (data[i_end] == ')')
                            {
                                temp[byte] = atoi(data+i_start);
                                break;
                            }

                            if (data[i_end] == ',')
                            {
                                data[i_end] = 0;
                                temp[byte] = atoi(data+i_start);
                                i_start = i_end+1;
                                byte++;
                            }
                        }
                        for (int i = 0;i < 4;i++)
                            dataIP.IP[i] = temp[i];
                        dataPort = temp[4]*256+temp[5];

                        dataConnection = tcp_connect(dataIP, dataPort);
                        printf("\nConnected (ID = %u). Wait until connection is established... ", dataConnection);
                    }
                    else if (data[0] == '3' && data[1] == '3' && data[2] == '1')
                    {
                        char pStr[200];
                        memset(pStr,0,200);
                        strcat(pStr,"PASS ");
                        strcat(pStr,pass);
                        strcat(pStr,"\r\n");
                        tcp_send(control, pStr, strlen(pStr));
                    }
                    else if (data[0] == '3' && data[1] == '5' && data[2] == '0')
                    {
                        if (renaming)
                        {
                            renaming = 0;
                            tcp_send(control, command, strlen(command));
                        }
                    }
                }
                else if (header->connectionID == dataConnection)
                {
                }
                break;
            }
            case EVENT_KEY_DOWN:
            {
                textColor(0x0F);
                KEY_t* key = (void*)buffer;
                if (*key == KEY_ESC)
                {
                    printf("quit...");
                    tcp_send(control, "QUIT\r\n", 8);
                    tcp_close(control);
                    return(0);
                }
                else if (*key == KEY_F1)
                {
                    memset(command,0,200);
                    char filename[100];
                    printf("Get file.\nEnter filename:\n");
                    gets(filename);
                    strcat(command,"RETR ");
                    strcat(command,filename);
                    strcat(command,"\r\n");
                    waitingDataCommand = 1;
                    enterPasvMode = 1;
                    tcp_send(control, "TYPE A\r\n", 8);
                }
                else if (*key == KEY_F2)
                {
                    //STOR
                }
                else if (*key == KEY_F3)
                {
                    //Rename
                    memset(command,0,200);
                    char oldFilename[100];
                    char newFilename[100];
                    char tempCommand[200];
                    printf("Rename.\nEnter current filename:\n");
                    gets(oldFilename);
                    printf("Enter new filename:\n");
                    gets(newFilename);
                    
                    strcat(command,"RNTO ");
                    strcat(command,newFilename);
                    strcat(command,"\r\n");

                    strcat(tempCommand,"RNFR ");
                    strcat(tempCommand,oldFilename);
                    strcat(tempCommand,"\r\n");
                    renaming = 1;
                    tcp_send(control, tempCommand, strlen(tempCommand));
                }
                else if (*key == KEY_F4)
                {
                    //Delete
                    memset(command,0,200);
                    char filename[100];
                    printf("Delete file.\nEnter filename:\n");
                    gets(filename);
                    strcat(command,"DELE ");
                    strcat(command,filename);
                    strcat(command,"\r\n");
                    tcp_send(control, command, strlen(command));
                }
                else if (*key == KEY_F5)
                {
                    //Create directory
                    memset(command,0,200);
                    char filename[100];
                    printf("Make directory.\nEnter directory:\n");
                    gets(filename);
                    strcat(command,"MKD ");
                    strcat(command,filename);
                    strcat(command,"\r\n");
                    tcp_send(control, command, strlen(command));
                }
                else if (*key == KEY_F6)
                {
                    //Remove directory
                    memset(command,0,200);
                    char filename[100];
                    printf("Remove directory.\nEnter directory:\n");
                    gets(filename);
                    strcat(command,"RMD ");
                    strcat(command,filename);
                    strcat(command,"\r\n");
                    tcp_send(control, command, strlen(command));
                }
                else if (*key == KEY_F7)
                {
                    //Change current directory
                    memset(command,0,200);
                    char dirname[100];
                    printf("Change current directory.\nEnter directory(without initiating / !):\n");
                    gets(dirname);
                    strcat(command,"CWD ");
                    strcat(command,dirname);
                    strcat(command,"\r\n");
                    tcp_send(control, command, strlen(command));
                }
                else if (*key == KEY_F8)
                {
                    //Get current directory
                    tcp_send(control, "PWD\r\n", 5);
                }
                else if (*key == KEY_F9)
                {
                    //List files/directories
                    memset(command,0,200);
                    strcat(command,"LIST -a\r\n");
                    waitingDataCommand = 1;
                    enterPasvMode = 1;
                    printf("List files/directories.\n");
                    tcp_send(control, "TYPE A\r\n", 8);
                }
                else if (*key == KEY_F12)
                {
                    printf("Enter command:\n");
                    memset(command,0,200);
                    gets(command);
                    strcat(command,"\r\n");
                    tcp_send(control, command, strlen(command));
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
