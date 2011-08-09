#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"


void sendstring_c(uint32_t connection, char* msg);
void setPixel(uint16_t x, uint16_t y);
void InterpretPackets();
void RenderGame();
void DrawRect(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2);



const uint32_t portwrite = 8085;
const uint32_t portread = 8084;


uint16_t gamemode = 0;
uint32_t bufferlen = 0;

uint32_t totalrecieved = 0;


uint8_t inbuffer[5000];
// uint8_t inbuffer2[5000];

uint16_t ballx = 0;
uint16_t bally = 0;
uint16_t ballr = 0;

uint16_t p1p = 0;
uint16_t p2p = 0;

uint16_t p1y = 0;
uint16_t p2y = 0;

uint16_t p1h = 0;
uint16_t p2h = 0;

uint16_t countdown = 0;

uint32_t connection = 0;
uint32_t connectionread = 0;

uint32_t connections = 0;

uint16_t rekcounter = 0;

bool connected = false;

char pStr[3];

uint16_t keydown = 0;
uint16_t keyup = 0;

uint16_t adjustbytes = 6;

int main()
{
    
	setScrollField(0, 40);
	//printLine("================================================================================", 0, 0x0B);
	//printLine("                     Pretty Browser - Network test program!",                      2, 0x0B);
	//printLine("--------------------------------------------------------------------------------", 4, 0x0B);
	
	//char tempstr[100];
	
    event_enable(true);
    char buffer[16384];
    EVENT_t ev = event_poll(buffer, 16384, EVENT_NONE);


    iSetCursor(0, 1);
	textColor(0x0C);
	printf("This App requires the PrettyPongGameServer.\n\n");
	
    textColor(0x0F);
    printf("Enter host IP of PrettyPongGameServer:\n");
    char enteredip[100];
    gets(enteredip);
	//strcpy(enteredip,"127.0.0.1\0");
	
	
    IP_t IP = stringToIP(enteredip); //resolveIP(hostname);
	IP_t IP2 = stringToIP(enteredip);
	
	
	
	/*
	IP.IP[0]=127;
	IP.IP[1]=0;
	IP.IP[2]=0;
	IP.IP[3]=1;
	*/
	
	
	gamemode = 0;
	/*
	 0: Waiting for connection
	 1: Waiting for welcome message
	 2: Preparing for game
	 3: Game
	 */
	
	
	bufferlen = 0;
	
	memset(inbuffer,0,5000);
	// memset(inbuffer2,0,5000);
	
	
	ballx = 0;
	bally = 0;
	ballr = 0;
	
	p1p = 0;
	p2p = 0;
	
	p1y = 0;
	p2y = 0;
	
	p1h = 0;
	p2h = 0;
	
	countdown = 0;
	
	connections = 0;
	
    connection = tcp_connect(IP, portwrite);
	connectionread = 0;
    printf("\nConnecting to Server... ");
	connected=false;
	
	
    for(;;)
    {
        switch(ev)
        {
            case EVENT_NONE:
				if(connected == true) {
					
					InterpretPackets();
					
					rekcounter=0;
					
					
					while((bufferlen>40)) {
						textColor(0x0B);
						rekcounter=(rekcounter+1);
						printf("\n\nSuperExtraInterpret %u",rekcounter);
						InterpretPackets();
					}
					
					rekcounter=0;
					
					while((bufferlen>10) && (rekcounter<5)) {
						textColor(0x0B);
						rekcounter=(rekcounter+1);
						printf("\n\nExtraInterpret %u",rekcounter);
						InterpretPackets();
					}
					
					
					
					
					memset(pStr,0,3);
					strcpy(pStr,"k");
					pStr[1]=keyup;
					pStr[2]=keydown;
					tcp_send(connection,pStr,3);
					
					
					
					RenderGame();
					
					
					textColor(0x0D);
					printf("\n\n\nINBUFFER-LEN: %u; TOTALRECIEVED: %u",bufferlen,totalrecieved);
					
				} else {
					textColor(0x0C);
					if((getCurrentSeconds()/1%2)==0) {
						textColor(0x0A);
					}
					printf("\nNot (yet) connected to the server...");
					
				}
				
                waitForEvent(30);
                break;
            case EVENT_TCP_CONNECTED:
            {
                printf("OK\n\n");
				connections=(connections+1);
				if(connections>1) {
					setScrollField(0, 5);
					clearScreen(0x00);
					iSetCursor(1,1);
					gamemode=1;
					connected=true;
					printf("Awaiting welcome message from server... ");
				} else {
					connectionread = tcp_connect(IP2, portread);
					printf("Awaiting second connection to connect...");
				}
				
				/*
                 char pStr[200];
                 memset(pStr,0,200);
                 strcat(pStr,"GET / HTTP/1.1\r\nHost: ");
                 //strcat(pStr,hostname);
                 strcat(pStr,"\r\nConnection: close\r\n\r\n");
                 textColor(0x0A);
                 puts(pStr);
                 textColor(0x0F);
                 //tcp_send(connection, pStr, strlen(pStr));
                 break;
				 */
				
            }
            case EVENT_TCP_RECEIVED:
            {
                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
				uint32_t hlen = header->length;
                data[header->length] = 0;
				
				/*
				for(uint32_t o=0; o<(header->length) && hlen==0; o++) {
					if(data[o] == 0) {
						hlen=o;
					}
				}
				*/
				 
				// if(hlen==0) {
					hlen=header->length;
				// }
				
				memcpy(inbuffer+bufferlen,data,hlen);
				
				bufferlen=(bufferlen+(hlen));
				
				totalrecieved=(totalrecieved+(hlen));
				
				// strcat(inbuffer,data);
				
                // printf("\npacket received. Length = %u\n:%s", header->length, data);
                break;
            }
			case EVENT_TCP_CLOSED:
			{
				textColor(0x0C);
				printf("\n\n\n\n\n\nConnection closed.");
				connection=0;
				connectionread=0;
				connected=false;
			}
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
				
				
                if(*key == KEY_ESC)
                {
					sendstring_c(connection,"q");
                    tcp_close(connection);
					tcp_close(connectionread);
                    return(0);
                }
				
				if(*key == KEY_ARRU || *key == KEY_W) {
					keyup = 1;
				}
				
				if(*key == KEY_ARRD || *key == KEY_S) {
					keydown = 1;
				}
                break;
            }
			case EVENT_KEY_UP:
			{
				KEY_t* key = (void*)buffer;
				
				
				if(*key == KEY_ARRU || *key == KEY_W) {
					keyup = 0;
				}
				
				if(*key == KEY_ARRD || *key == KEY_S) {
					keydown = 0;
				}
			}
            default:
                break;
        }
        ev = event_poll(buffer, 16384, EVENT_NONE);
    }

    //tcp_close(connection);
    return(0);
}


void sendstring_c(uint32_t con, char* msg) {
	uint16_t leng=strlen(msg);
	char pStrS[leng];
	strcpy(pStrS,msg);
	tcp_send(con,pStrS,leng);
}


void setPixel(uint16_t x, uint16_t y) {
	// Cursor
	iSetCursor(x,y);
	// put
	putchar('x');
}

void RenderGame() {
	position_t cur;
	getCursor(&cur);
	
	clearScreen(0x00);
	
	
	iSetCursor(ballx,bally+adjustbytes);
	textColor(0x0C);
	printf("/\\");
	iSetCursor(ballx,bally+1+adjustbytes);
	textColor(0x0C);
	printf("\\/");
	
	
	textColor(0x0E);
	DrawRect(1,(p1y+adjustbytes),3,((p1y+adjustbytes)+p1h));
	
	textColor(0x0A);
	DrawRect(77,(p2y+adjustbytes),79,((p2y+adjustbytes)+p2h));
	
	textColor(0x0F);
	
	setCursor(cur);
}

void DrawRect(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2) {
	for(uint16_t lx = x; lx<x2; lx++) {
		for(uint16_t ly = y; ly<y2; ly++) {
			setPixel(lx,ly);
		}
	}
}

void InterpretPackets() {
	// printf("\n\nInterpretPackets(); entered\n");
	
	if(bufferlen<1) {
		// printf("No data to interpret.\n");
		return;
	}
	
	switch (gamemode) {
		case 1:
			if(bufferlen<26) {
				textColor(0x0A);
				printf(" x");
			} else {
				
				char header[30];
				memcpy(header,inbuffer,26);
				
				if(strncmp(header,"PrettyPongGameServer_AJ154",26) != 0) {
					textColor(0x0C);
					printf("\n\nUnknown Server version!\n");
					
					printf("Waiting for a second packet...\n");
					
					// bufferlen
					
					
					memmove(inbuffer, inbuffer+1, (bufferlen-1));
					bufferlen=(bufferlen-1);
					
					textColor(0x0F);
				} else {
					printf("OK\n\n");
					
					memmove(inbuffer, inbuffer+26, (bufferlen-26));
					bufferlen=(bufferlen-26);
					
					gamemode = 2;
					clearScreen(0x00);
					sendstring_c(connection,"y");
					gamemode = 3;
				}
				
			}
			
			break;
		case 3:
			
			if(bufferlen<1) {
				textColor(0x0A);
				// printf("No Data to interpret!\n");
			} else {
				switch (inbuffer[0]) {
					case 'b':
						if(bufferlen<3) {
							textColor(0x0A);
							printf(" o");
						} else {
							
							switch (inbuffer[1]) {
								case 'x':
									textColor(0x0F);
									// printf(" BallX: ");
									
									ballx=inbuffer[2];
									
									// printf("%u",ballx);
									
									
									memmove(inbuffer, inbuffer+3, (bufferlen-3));
									bufferlen=(bufferlen-3);
									
									break;
								case 'y':
									textColor(0x0F);
									// printf(" BallY: ");
									
									bally=inbuffer[2];
									
									// printf("%u",bally);
									
									
									memmove(inbuffer, inbuffer+3, (bufferlen-3));
									bufferlen=(bufferlen-3);
									
									break;
								case 'r':
									textColor(0x0F);
									// printf(" BallR: ");
									
									ballr=inbuffer[2];
									
									// printf("%u",ballr);
									
									
									memmove(inbuffer, inbuffer+3, (bufferlen-3));
									bufferlen=(bufferlen-3);
									
									break;
								default:
									textColor(0x0C);
									printf(" Invalid B-Package: %s\n",inbuffer[1]);
									
									char entereds[100];
									gets(entereds);
									
									memmove(inbuffer, inbuffer+1, (bufferlen-1));
									bufferlen=(bufferlen-1);
									
									break;
							}
						}
						
						break;
					case '1':
						if(bufferlen<3) {
							textColor(0x0A);
							// printf(" o");
						} else {
							switch (inbuffer[1]) {
								case 'p':
									
									textColor(0x0F);
									// printf(" P1P: ");
									
									p1p=inbuffer[2];
									
									// printf("%u\n",p1p);
									
									
									memmove(inbuffer, inbuffer+3, (bufferlen-3));
									bufferlen=(bufferlen-3);
									
									break;
									
								case 'y':
									
									textColor(0x0F);
									// printf(" P1Y: ");
									
									p1y=inbuffer[2];
									
									// printf("%u",p1y);
									
									
									memmove(inbuffer, inbuffer+3, (bufferlen-3));
									bufferlen=(bufferlen-3);
									
									break;
								case 'h':
									
									textColor(0x0F);
									// printf(" P1H: ");
									
									p1h=inbuffer[2];
									
									// printf("%u",p1h);
									
									
									memmove(inbuffer, inbuffer+3, (bufferlen-3));
									bufferlen=(bufferlen-3);
									
									break;
								default:
									
									textColor(0x0C);
									printf(" Invalid P1-Package: %s\n",inbuffer[1]);
									
									char entereds[100];
									gets(entereds);
									
									memmove(inbuffer, inbuffer+1, (bufferlen-1));
									bufferlen=(bufferlen-1);
									
									break;
							}
							
							
						}
						break;
						
					case '2':
						if(bufferlen<3) {
							textColor(0x0A);
							// printf("o");
						} else {
							switch (inbuffer[1]) {
								case 'p':
									
									textColor(0x0F);
									// printf(" P2P: ");
									
									p2p=inbuffer[2];
									
									// printf("%u",p2p);
									
									
									memmove(inbuffer, inbuffer+3, (bufferlen-3));
									bufferlen=(bufferlen-3);
									
									break;
									
								case 'y':
									
									textColor(0x0F);
									// printf(" P2Y: ");
									
									p2y=inbuffer[2];
									
									// printf("%u",p2y);
									
									
									memmove(inbuffer, inbuffer+3, (bufferlen-3));
									bufferlen=(bufferlen-3);
									
									break;
								case 'h':
									
									textColor(0x0F);
									// printf(" P2H: ");
									
									p2h=inbuffer[2];
									
									// printf("%u",p2h);
									
									
									memmove(inbuffer, inbuffer+3, (bufferlen-3));
									bufferlen=(bufferlen-3);
									
									break;
								default:
									
									textColor(0x0C);
									printf(" Invalid P2-Package: %s\n",inbuffer[1]);
									
									char entereds[100];
									gets(entereds);
									
									memmove(inbuffer, inbuffer+1, (bufferlen-1));
									bufferlen=(bufferlen-1);
									
									break;
							}
							
							
						}
						break;
						
					case 'c':
						if(bufferlen<3) {
							textColor(0x0A);
							// printf("o");
						} else {
							switch (inbuffer[1]) {
								case 'n':
									textColor(0x0F);
									// printf(" Countdown: ");
									
									countdown = inbuffer[2];
									
									// printf("%u",countdown);
									
									memmove(inbuffer, inbuffer+3, (bufferlen-3));
									bufferlen=(bufferlen-3);
									
									break;
								default:
									textColor(0x0C);
									printf(" Invalid C-Package: %s\n",inbuffer[1]);
									
									char entereds[100];
									gets(entereds);
									
									memmove(inbuffer, inbuffer+1, (bufferlen-1));
									bufferlen=(bufferlen-1);
									
									break;
							}
						}
						break;
					case 'g':
						if(bufferlen<3) {
							textColor(0x0A);
							// printf("o");
						} else {
							switch (inbuffer[1]) {
								case 'o':
									textColor(0x0F);
									// printf(" Go!");
									
									// 1 0-Byte has arrived..^^
									memmove(inbuffer, inbuffer+3, (bufferlen-3));
									bufferlen=(bufferlen-3);
									
									break;
								default:
									textColor(0x0C);
									printf(" Invalid G-Package: %s\n",inbuffer[1]);
									
									char entereds[100];
									gets(entereds);
									
									memmove(inbuffer, inbuffer+1, (bufferlen-1));
									bufferlen=(bufferlen-1);
									
									break;
							}
						}
						break;
					case 0:
						textColor(0x0E);
						// printf(" No Data");
						
						memmove(inbuffer, inbuffer+1, (bufferlen-1));
						bufferlen=(bufferlen-1);
						
						break;
					case 'x':
						textColor(0x09);
						printf(" x");
						
						memmove(inbuffer, inbuffer+1, (bufferlen-1));
						bufferlen=(bufferlen-1);
						
						break;
					case 13:
					case 10:
						textColor(0x0A);
						// printf(" r or n");
						
						memmove(inbuffer, inbuffer+1, (bufferlen-1));
						bufferlen=(bufferlen-1);
						
						break;
					default:
						textColor(0x0C);
						
						printf(" Unknown Data: ");
						printf("%u-",inbuffer[0]);
						printf("%u-",inbuffer[1]);
						printf("%u",inbuffer[2]);
						
						// memshow()
						
						char entereds[100];
						gets(entereds);
						
						
						memmove(inbuffer, inbuffer+1, (bufferlen-1));
						bufferlen=(bufferlen-1);
						
						break;
				}
				
			}
			
		default:
			break;
	}
	
	// printf("\n\nInterpretPackets(); exited\n");
}
