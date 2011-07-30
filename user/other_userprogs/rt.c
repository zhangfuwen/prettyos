#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"


int main()
{
    setScrollField(7, 46);
    printLine("================================================================================", 0, 0x0B);
    printLine("                     Pretty RemoteControlTest", 2, 0x0B);
	printLine("                   by Cuervo (Christian F. Coors)", 4, 0x0B);
    printLine("--------------------------------------------------------------------------------", 6, 0x0B);
	
    event_enable(true);
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);
	
	
    iSetCursor(0, 7);
    textColor(0x0F);
    printf("Initializing...");
	
	
	textColor(0x0A);
	printf("OK\n");
	textColor(0x0F);
	
    //char hostname[100];
    //gets(hostname);
	
    //IP_t IP = resolveIP(hostname);
	
	printf("Generating connection...");
	IP_t IP = {.iIP = 0};
    uint32_t connection = tcp_connect(IP, 80);
	textColor(0x0A);
	printf("OK\n");
	textColor(0x0F);
	
	
	printf("Awaiting data...\n\n");
	
	textColor(0x0E);
	printf("To use this program, connect to PrettyOS with a webbrowser of your choice.\n\n");
	textColor(0x0C);
	printf("Just enter http://Pre.tty.OS.IP:80/");
	textColor(0x0F);
	
	printf("\n");
	
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
				printf("OK\n");
				textColor(0x0F);
				
				
				char bstr[8000];
				memset(bstr,0,8000);
							
				strcat(bstr,"Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.");
				strcat(bstr,"Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi. Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat. ");
				strcat(bstr,"Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo consequat. Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi. ");
				strcat(bstr,"Nam liber tempor cum soluta nobis eleifend option congue nihil imperdiet doming id quod mazim placerat facer possim assum. Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo consequat. ");
				strcat(bstr,"Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis. ");
				strcat(bstr,"At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, At accusam aliquyam diam diam dolore dolores duo eirmod eos erat, et nonumy sed tempor et et invidunt justo labore Stet clita ea et gubergren, kasd magna no rebum. sanctus sea sed takimata ut vero voluptua. est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat. ");
				strcat(bstr,"Consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus. ");
				strcat(bstr,"Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. ");
				strcat(bstr,"Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi. Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat. ");
				strcat(bstr,"Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo consequat. Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi. ");
				strcat(bstr,"\nEND OF MESSAGE");
								
				printf("\nLength: %u\n",strlen(bstr));
				tcp_send(connection, bstr, strlen(bstr));
				
				/*
				char abc[1460];
				memset(abc,0,1460);
				strcpy(abc,"DUMMYTEXT");
				tcp_send(connection,abc,strlen(abc));
                tcp_send(connection,abc,strlen(abc));
                tcp_send(connection,abc,strlen(abc));
                tcp_send(connection,abc,strlen(abc));
                tcp_send(connection,abc,strlen(abc));
                */
								
				printf("Closing connection...");
				tcp_close(connection);
				textColor(0x0A);
				printf("OK\n");
				textColor(0x0F);
				
				
				printf("Opening new connection...");
				connection = tcp_connect(IP, 80);
				textColor(0x0A);
				printf("OK\n");
				textColor(0x0F);
                break;
            }
            case EVENT_TCP_RECEIVED:
            {				
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
