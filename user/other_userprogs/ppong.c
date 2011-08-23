#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

// Parameters
const uint8_t PLAYER_1_HEIGHT = 15;
const uint8_t PLAYER_2_HEIGHT = 35;
const double XSPEEDLOW        = -3;
const double XSPEEDHIGH       =  3;
const double YSPEEDLOW        = -2;  
const double YSPEEDHIGH       =  2;
	

// GFX:
const uint8_t BLINKFREQ    =   100;
const uint8_t SLIDEDELAY   =    10;
const uint8_t COLUMNS      =    80;
const uint8_t LINES        =    50;
const bool DOUBLEBUFFERING =  true;

// Default options:
bool option_sound          = true;
bool option_menuanimation  = true;

// Do not modify anything after this line.

// Menu defines
#define MENU_MAIN 1

#define MENU_GAME 12
#define MENU_OPTIONS 13
#define MENU_CREDITS 14
#define MENU_EXIT 15

#define MENU_SOLOGAME 120
#define MENU_LANGAME 121
#define MENU_INTERNETGAME 122


#define MENU_MAIN_SELECTABLE 4

#define MENU_GAME_SELECTABLE 4
#define MENU_OPTIONS_SELECTABLE 3
#define MENU_CREDITS_SELECTABLE 1
#define MENU_EXIT_SELECTABLE 2

#define MENU_SOLOGAME_SELECTABLE 3
#define MENU_LANGAME_SELECTABLE 3
#define MENU_INTERNETGAME_SELECTABLE 3



// AppMode defines
#define APPMODE_MENU 1
#define APPMODE_GAME 2


// GameMode defines
#define GAMEMODE_NONE 1
#define GAMEMODE_LOCALGAME_1P 2
#define GAMEMODE_LOCALGAME_2P 3
#define GAMEMODE_LANGAME_HOST 4
#define GAMEMODE_LANGAME_JOIN 5
#define GAMEMODE_INTERNETGAME_HOST 6
#define GAMEMODE_INTERNETGAME_JOIN 7


void DrawMainMenu();

void DrawGameMenu();
void DrawOptionsMenu();
void DrawCreditsMenu();
void DrawExitMenu();

void DrawSoloGameMenu();
void DrawLANGameMenu();
void DrawInternetGameMenu();


void DrawMenuStructure();
void DrawMenuPoint(char* name, uint8_t currentlyselected, uint8_t id);

void DrawMenuHeader();
void DrawMenuContentBox(uint16_t addlines);

void DrawMenuSlideIn();
void DrawMenuSlideOut();

void SwitchToMenu(uint8_t switchto);

void Menu_SelectorUp();
void Menu_SelectorDown();

void Menu_Select();


void DrawRect(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2);
void DrawText(char* text, uint16_t x, uint16_t y);
void SetPixel(uint16_t x, uint16_t y);
void SetSpace(uint16_t x, uint16_t y);
void FlipIfNeeded();

void WaitKey();
void clearScreen2();

double random(double lowerbounds, double upperbounds);


void Sound_Denied();
void Sound_OK();
void Sound_Select();
void Sound_Error();


void NotImplementedError();

void Error(char* message, bool critical);


void RenderApp();



void ResetBall();

void RunGame();

void UpdateGame();
void RenderGame();



uint8_t currentmenu;

uint8_t mainmenuselected;

uint8_t gamemenuselected;
uint8_t optionsmenuselected;
uint8_t creditsmenuselected;
uint8_t exitmenuselected;

uint8_t sologamemenuselected;
uint8_t langamemenuselected;
uint8_t internetgamemenuselected;


uint8_t appmode;

uint8_t gamemode;


int16_t player1y;   // Y Position on screen
int16_t player2y;
int16_t player1h;   // Height of player
int16_t player2h;
int16_t player1p;   // Player score
int16_t player2p;

double  ballx;      // Ball X
double  bally;      // Ball Y
double  ballxspeed; // Ball X-Speed
double  ballyspeed; // Ball Y-Speed


bool exitapp;


bool ignorenextkeystroke;


int main()
{
    srand(getCurrentMilliseconds()); // seed

	// Adjust screen
    if (DOUBLEBUFFERING)
    {
        console_setProperties(CONSOLE_FULLSCREEN);
    }
    else
    {
        console_setProperties(CONSOLE_AUTOREFRESH|CONSOLE_FULLSCREEN);
    }
	setScrollField(0,50);
	clearScreen(0x00);

	#if __STDC__ != 1
	printf("\n\nThis App requires a standard C Compiler.\n\n");
	WaitKey();
	return(0);
	#endif

	// Enable Events
	event_enable(true);
    char buffer[8192];
    EVENT_t ev = event_poll(buffer, 8192, EVENT_NONE);

	// Selected menupoints
	mainmenuselected = 0;

	gamemenuselected = 0;
	optionsmenuselected = 0;
	creditsmenuselected = 0;
	exitmenuselected = 0;

	sologamemenuselected = 0;
	langamemenuselected = 0;
	internetgamemenuselected = 0;


	// Clear eventual messages
	clearScreen(0x00);

	// Set to menu
	appmode = APPMODE_MENU;
	gamemode = GAMEMODE_NONE;

	// Play animation
	DrawMenuSlideIn();

	// Initial drawing
	DrawMainMenu();

	// If true the app closes with animation
	exitapp = false;

    while(exitapp == false)
    {
        switch(ev)
        {
            case EVENT_NONE:
			{
				// Re-render App (overwrite any message)
				if (DOUBLEBUFFERING == false)
                {
				    clearScreen2();
                }

				RenderApp();
				FlipIfNeeded();

				/*
				 textColor(0x00);
				 DrawRect(0,0,80,46);
				*/

				if (DOUBLEBUFFERING)
                {
                    clearScreen2();
                }

                waitForEvent(20);
                break;
			}
            case EVENT_TCP_CONNECTED:
            {
			    break;
            }
            case EVENT_TCP_RECEIVED:
            {
                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;

                // printf("\npacket received. Length = %u\n:%s", header->length, data);
                break;
            }
			case EVENT_TCP_CLOSED:
			{
				// textColor(0x0C);
				// printf("Connection closed.");
                break;
			}
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;

                if(*key == KEY_ESC)
                {
					switch(appmode)
                    {
						case APPMODE_MENU:
							switch (currentmenu)
                            {
								case MENU_MAIN:
									SwitchToMenu(MENU_EXIT);
									break;
								case MENU_CREDITS:
								case MENU_OPTIONS:
								case MENU_EXIT:
								case MENU_GAME:
									SwitchToMenu(MENU_MAIN);
									break;
								case MENU_SOLOGAME:
								case MENU_LANGAME:
								case MENU_INTERNETGAME:
									SwitchToMenu(MENU_GAME);
									break;
								default:
									break;
							}
							break;
						default:
							Error("An unknown error occurred (1): %u",appmode);
							break;
					}
				}

				if(*key == KEY_ARRU || *key == KEY_W)
                {
					switch(appmode)
                    {
						case APPMODE_MENU:
							Menu_SelectorUp();
							break;
						default:
							Error("An unknown error occurred (3): %u",appmode);
							break;
					}
                }

				if(*key == KEY_ARRD || *key == KEY_S)
                {
					switch(appmode)
                    {
						case APPMODE_MENU:
							Menu_SelectorDown();
							break;
						default:
							Error("An unknown error occurred (4): %u",appmode);
							break;
					}
				}

				if(*key == KEY_ENTER || *key == KEY_SPACE)
                {
					switch(appmode)
                    {
						case APPMODE_MENU:
							Menu_Select();
							break;
						default:
							Error("An unknown error occurred (5): %u",appmode);
							break;
					}
				}

				if(*key == KEY_E)
                {
					Error("This is a manually triggered error screen.",false);
				}

                break;
            }
            default:
                break;
        }
        ev = event_poll(buffer, 8192, EVENT_NONE);
    }

	DrawMenuSlideOut();

	console_setProperties(CONSOLE_AUTOREFRESH|CONSOLE_AUTOSCROLL|CONSOLE_FULLSCREEN);
	return(0);
}

void RenderGame()
{
	// Draw border around the playfield
	textColor(0x88);
	DrawRect(0,0,COLUMNS,LINES);
	textColor(0x00);
	DrawRect(1,1,COLUMNS-1,LINES-1);

	// Draw line in the middle
	textColor(0x11);
	DrawRect((COLUMNS/2),1,(COLUMNS/2)+1,LINES-1);

	// Draw ball
	textColor(0x0C);
	iSetCursor(ballx-1,bally-1);
	printf("/");
	textColor(0xCC);
	printf("X");
	textColor(0x0C);
	printf("\\");
	iSetCursor(ballx-1,bally);
	textColor(0xCC);
	printf("X");
	printf("X");
	printf("X");
	textColor(0x0C);
	iSetCursor(ballx-1,bally+1);
	printf("\\");
	textColor(0xCC);
	printf("X");
	textColor(0x0C);
	printf("/");

	// Draw players
	textColor(0xAA);
	DrawRect(3,player1y,5,player1y+player1h);
	textColor(0xEE);
	DrawRect(COLUMNS-5,player2y,COLUMNS-3,player2y+player2h);

	// Draw score(s)
	textColor(0x0D);
	iSetCursor(10,5);
	beep(110,300);
    printf("%u : %u",player1p,player2p);
}

void UpdateGame()
{
	ballx = (ballx + ballxspeed);
	bally = (bally + ballyspeed);

	if((bally-3) < 0)
    {
		ballyspeed = fabs(ballyspeed);
		bally = 2;
	}

	if((bally+4) > LINES)
    {
		ballyspeed = (-(fabs(ballyspeed)));
		bally = (LINES - 3);
	}


	if((ballx-2) < 5)
    {
		if((bally-3) > player1y && (bally + 3) < (player1y+player1h))
        {
			ballxspeed = fabs(ballxspeed);
			ballx = 6;
		}
	}

	if((ballx+3) > (COLUMNS - 5))
    {
		if((bally-3) > player2y && (bally + 3) < (player2y+player2h))
        {
			ballxspeed = (-(fabs(ballxspeed)));
			ballx = (COLUMNS - 7);
		}
	}

	if((ballx+3) > COLUMNS && ballxspeed > 0)
    {
		player1p = (player1p + 1);
		ResetBall();
	}

	if((ballx-2) < 0 && ballxspeed < 0)
    {
		player2p = (player2p + 1);
		ResetBall();
	}

	if(keyPressed(KEY_ARRU) || (keyPressed(KEY_W)))
    {
		player1y = (player1y - 3);
	}

	if(keyPressed(KEY_ARRD) || keyPressed(KEY_S))
    {
		player1y = (player1y + 3);
	}

	if(player1y < 3)
    {
		player1y = 3;
	}

	if((player1y+player1h) > (LINES - 3))
    {
		player1y = ((LINES - 3) - player1h);
	}
}

double random(double lowerbounds, double upperbounds)
{
	return ((((rand() & 0x01) << 15)  + rand())/(double)(65535))*(upperbounds-lowerbounds)+lowerbounds; 	
}

void ResetBall()
{	
    sleep(500);
    do
    {
        ballx = (COLUMNS/2);
	    bally = (LINES/2);
	    ballxspeed = random(XSPEEDLOW, XSPEEDHIGH);
	    ballyspeed = random(YSPEEDLOW, YSPEEDHIGH);
    }
    while(ballyspeed == 0);        
}

void RunGame()
{
	switch (gamemode)
    {
		case GAMEMODE_NONE:
			Error("An unknown error occurred.",true);
			break;
		case GAMEMODE_LOCALGAME_1P:
			break;
		default:
			Error("An unknown error occurred (10).",true);
			break;
	}

	char buffer[8192];
    EVENT_t ev = event_poll(buffer, 8192, EVENT_NONE);

	bool exitgame = false;

	player1h = PLAYER_1_HEIGHT;
	player2h = PLAYER_2_HEIGHT;

	player1y = (LINES / 2) - (player1h / 2);
	player2y = (LINES / 2) - (player2h / 2);

	player1p = 0;
	player2p = 0;

	ResetBall();

	while(exitgame == false)
    {
        switch(ev)
        {
            case EVENT_NONE:
			{

				// Re-render App (overwrite any message)
                if (DOUBLEBUFFERING == false)
                {
				    clearScreen2();
                }

				UpdateGame();
				RenderGame();
				FlipIfNeeded();

                if (DOUBLEBUFFERING)
                {
				    clearScreen2();
                }

                waitForEvent(1);
				break;
			}
            case EVENT_TCP_CONNECTED:
            {
				break;
            }
            case EVENT_TCP_RECEIVED:
            {
                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;

                // printf("\npacket received. Length = %u\n:%s", header->length, data);
                break;
            }
			case EVENT_TCP_CLOSED:
			{
				// textColor(0x0C);
				// printf("Connection closed.");
                break;
			}
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;

				if(*key == KEY_ESC)
                {
					exitgame = true;
				}

				/*
				if(*key == KEY_ARRU || *key == KEY_W) 
                {
					switch(appmode) 
                    {
						case APPMODE_MENU:
							Menu_SelectorUp();
							break;
						default:
							Error("An unknown error occurred (3): %u",appmode);
							break;
					}				
                }

				if(*key == KEY_ARRD || *key == KEY_S) 
                {
					switch(appmode) 
                    {
						case APPMODE_MENU:
							Menu_SelectorDown();
							break;
						default:
							Error("An unknown error occurred (4): %u",appmode);
							break;
					}
				}

				if(*key == KEY_ENTER || *key == KEY_SPACE) 
                {
					switch(appmode) 
                    {
						case APPMODE_MENU:
							Menu_Select();
							break;
						default:
							Error("An unknown error occurred (5): %u",appmode);
							break;
					}
				}
				*/

				if(*key == KEY_E)
                {
					Error("This is a manually triggered error screen.",false);
				}

                break;
            }
            default:
                break;
        }
        ev = event_poll(buffer, 8192, EVENT_NONE);
    }
}


void clearScreen2()
{
	clearScreen(0x00);
}


void RenderApp()
{
	switch(appmode)
    {
		case APPMODE_MENU:
			switch (currentmenu)
            {
				case MENU_MAIN:
					DrawMainMenu();
					break;
				case MENU_EXIT:
					DrawExitMenu();
					break;
				case MENU_CREDITS:
					DrawCreditsMenu();
					break;
				case MENU_GAME:
					DrawGameMenu();
					break;
				case MENU_OPTIONS:
					DrawOptionsMenu();
					break;
				case MENU_SOLOGAME:
					DrawSoloGameMenu();
					break;
				case MENU_LANGAME:
					DrawLANGameMenu();
					break;
				case MENU_INTERNETGAME:
					DrawInternetGameMenu();
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void Menu_SelectorUp()
{
	switch (currentmenu)
    {
		case MENU_MAIN:
			if(mainmenuselected == 0)
            {
				mainmenuselected = (MENU_MAIN_SELECTABLE-1);
			}
            else
            {
				mainmenuselected = (mainmenuselected-1);
			}
			Sound_Select();
			break;
		case MENU_EXIT:
			if(exitmenuselected == 0)
            {
				exitmenuselected = (MENU_EXIT_SELECTABLE-1);
			}
            else
            {
				exitmenuselected = (exitmenuselected-1);
			}
			Sound_Select();
			break;
		case MENU_GAME:
			if(gamemenuselected == 0)
            {
				gamemenuselected = (MENU_GAME_SELECTABLE-1);
			}
            else
            {
				gamemenuselected = (gamemenuselected-1);
			}
			Sound_Select();
			break;
		case MENU_CREDITS:
			Sound_Select();
			break;
		case MENU_OPTIONS:
			if(optionsmenuselected == 0)
            {
				optionsmenuselected = (MENU_OPTIONS_SELECTABLE-1);
			}
            else
            {
				optionsmenuselected = (optionsmenuselected-1);
			}
			Sound_Select();
			break;
		case MENU_SOLOGAME:
			if(sologamemenuselected == 0)
            {
				sologamemenuselected = (MENU_SOLOGAME_SELECTABLE-1);
			}
            else
            {
				sologamemenuselected = (sologamemenuselected-1);
			}
			Sound_Select();
			break;
		case MENU_LANGAME:
			if(langamemenuselected == 0)
            {
				langamemenuselected = (MENU_LANGAME_SELECTABLE-1);
			}
            else
            {
				langamemenuselected = (langamemenuselected-1);
			}
			Sound_Select();
			break;
		case MENU_INTERNETGAME:
			if(internetgamemenuselected == 0)
            {
				internetgamemenuselected = (MENU_INTERNETGAME_SELECTABLE-1);
			}
            else
            {
				internetgamemenuselected = (internetgamemenuselected-1);
			}
			Sound_Select();
			break;
		default:
			Sound_Error();
			break;
	}
}

void Menu_SelectorDown()
{
	switch (currentmenu)
    {
		case MENU_MAIN:
			if(mainmenuselected == (MENU_MAIN_SELECTABLE-1))
            {
				mainmenuselected = 0;
			}
            else
            {
				mainmenuselected = (mainmenuselected+1);
			}
			Sound_Select();
			break;
		case MENU_EXIT:
			if(exitmenuselected == (MENU_EXIT_SELECTABLE-1))
            {
				exitmenuselected = 0;
			}
            else
            {
				exitmenuselected = (exitmenuselected+1);
			}
			Sound_Select();
			break;
		case MENU_CREDITS:
			Sound_Select();
			break;
		case MENU_GAME:
			if(gamemenuselected == (MENU_GAME_SELECTABLE-1))
            {
				gamemenuselected = 0;
			}
            else
            {
				gamemenuselected = (gamemenuselected+1);
			}
			Sound_Select();
			break;
		case MENU_OPTIONS:
			if(optionsmenuselected == (MENU_OPTIONS_SELECTABLE-1))
            {
				optionsmenuselected = 0;
			}
            else
            {
				optionsmenuselected = (optionsmenuselected+1);
			}
			Sound_Select();
			break;
		case MENU_SOLOGAME:
			if(sologamemenuselected == (MENU_SOLOGAME_SELECTABLE-1))
            {
				sologamemenuselected = 0;
			}
            else
            {
				sologamemenuselected = (sologamemenuselected+1);
			}
			Sound_Select();
			break;
		case MENU_LANGAME:
			if(langamemenuselected == (MENU_LANGAME_SELECTABLE-1))
            {
				langamemenuselected = 0;
			}
            else
            {
				langamemenuselected = (langamemenuselected+1);
			}
			Sound_Select();
			break;
		case MENU_INTERNETGAME:
			if(internetgamemenuselected == (MENU_INTERNETGAME_SELECTABLE-1))
            {
				internetgamemenuselected = 0;
			}
            else
            {
				internetgamemenuselected = (internetgamemenuselected+1);
			}
			Sound_Select();
			break;
		default:
			Sound_Error();
			break;
	}
}

void Menu_Select()
{
	switch (currentmenu)
    {
		case MENU_MAIN:
			switch (mainmenuselected)
            {
				case 0: // New game
					Sound_OK();
					SwitchToMenu(MENU_GAME);
					break;
				case 1: // Options
					Sound_OK();
					SwitchToMenu(MENU_OPTIONS);
					break;
				case 2:
					Sound_OK();
					SwitchToMenu(MENU_CREDITS);
					break;
				case 3: // Exit
					Sound_OK();
					SwitchToMenu(MENU_EXIT);
					break;
				default:
					break;
			}
			break;
		case MENU_EXIT:
			switch (exitmenuselected)
            {
				case 0: // Yes
					Sound_OK();
					exitapp = true;
					break;
				case 1: // No
					Sound_Denied();
					SwitchToMenu(MENU_MAIN);
					break;
				default:
					break;
			}
			break;
		case MENU_CREDITS:
			switch (creditsmenuselected)
            {
				case 0:
					Sound_OK();
					SwitchToMenu(MENU_MAIN);
					break;
				default:
					break;
			}
			break;
		case MENU_GAME:
			switch (gamemenuselected)
            {
				case 0: // Local
					Sound_OK();
					SwitchToMenu(MENU_SOLOGAME);
					break;
				case 1: // LAN
					Sound_OK();
					SwitchToMenu(MENU_LANGAME);
					break;
				case 2: // Internet
					Sound_OK();
					SwitchToMenu(MENU_INTERNETGAME);
					break;
				case 3: // Main menu
					Sound_OK();
					SwitchToMenu(MENU_MAIN);
					break;
				default:
					break;
			}
			break;
		case MENU_OPTIONS:
			switch (optionsmenuselected)
            {
				case 0: // Sound
					if(option_sound)
                    {
						option_sound = false;
					}
                    else
                    {
						option_sound = true;
					}
					Sound_OK();
					break;
				case 1: // Menu animations
					if(option_menuanimation)
                    {
						option_menuanimation = false;
					}
                    else
                    {
						option_menuanimation = true;
					}
					Sound_OK();
					break;
				case 2:
					Sound_OK();
					SwitchToMenu(MENU_MAIN);
					break;
				default:
					break;
			}
			break;
		case MENU_SOLOGAME:
			switch (sologamemenuselected)
            {
				case 0: // One player
					// NotImplementedError();
					gamemode=GAMEMODE_LOCALGAME_1P;
					appmode=APPMODE_GAME;
					RunGame();
					appmode=APPMODE_MENU;
					break;
				case 1: // Two players
					NotImplementedError();
					break;
				case 2: // GameMenu
					Sound_OK();
					SwitchToMenu(MENU_GAME);
					break;
				default:
					break;
			}
			break;
		case MENU_LANGAME:
			switch (langamemenuselected)
            {
				case 0: // Join game
					NotImplementedError();
					break;
				case 1: // Host game
					NotImplementedError();
					break;
				case 2: // GameMenu
					Sound_OK();
					SwitchToMenu(MENU_GAME);
					break;
				default:
					break;
			}
			break;
		case MENU_INTERNETGAME:
			switch (internetgamemenuselected)
            {
				case 0: // Join game
					NotImplementedError();
					break;
				case 1: // Host game
					NotImplementedError();
					break;
				case 2: // GameMenu
					Sound_OK();
					SwitchToMenu(MENU_GAME);
					break;
			}
			break;
		default:
			// This should not happen!

			// The ; in front of char errormsg[100] exists because of a bug in gcc:
			// http://www.mail-archive.com/gcc-bugs@gcc.gnu.org/msg259382.html
			;char errormsg[100];
			errormsg[0] = '\0';
			sprintf(errormsg,"An unknwown error occurred (2): %u",currentmenu);
			Error(errormsg,true);
			break;
	}
}

void DrawMainMenu()
{
	currentmenu=MENU_MAIN;
	DrawMenuStructure();

	textColor(0x0D);
	DrawText("Main menu",21,24);


	if(mainmenuselected > 127)
    {
		mainmenuselected = (MENU_MAIN_SELECTABLE-1);
	}

	if(mainmenuselected > (MENU_MAIN_SELECTABLE-1))
    {
		mainmenuselected = 0;
	}

	iSetCursor(26,29);
	DrawMenuPoint("New game",mainmenuselected,0);
	iSetCursor(26,31);
	DrawMenuPoint("Options",mainmenuselected,1);
	iSetCursor(26,33);
	DrawMenuPoint("Credits",mainmenuselected,2);
	iSetCursor(26,37);
	DrawMenuPoint("Exit",mainmenuselected,3);

}

void DrawOptionsMenu()
{
	currentmenu=MENU_OPTIONS;
	DrawMenuStructure();

	textColor(0x0D);
	DrawText("Options:",21,24);


	iSetCursor(26,29);
	DrawMenuPoint("Sound",optionsmenuselected,0);

	iSetCursor(38,29);
	if(optionsmenuselected == 0)
    {
		textColor(0x0F);
	}
    else
    {
		textColor(0x08);
	}
	printf("[");
	if(option_sound)
    {
		if(optionsmenuselected == 0)
        {
			textColor(0x0A);
		}
        else
        {
			textColor(0x02);
		}
		printf("YES");
	}
    else
    {
		if(optionsmenuselected == 0)
        {
			textColor(0x0C);
		}
        else
        {
			textColor(0x04);
		}
		printf("NO");
	}
	if(optionsmenuselected == 0)
    {
		textColor(0x0F);
	}
    else
    {
		textColor(0x08);
	}
	printf("]");

	// option_menuanimation

	iSetCursor(26,31);
	DrawMenuPoint("Menu animations",optionsmenuselected,1);

	iSetCursor(48,31);
	if(optionsmenuselected == 1)
    {
		textColor(0x0F);
	}
    else
    {
		textColor(0x08);
	}
	printf("[");
	if(option_menuanimation)
    {
		if(optionsmenuselected == 1)
        {
			textColor(0x0A);
		}
        else
        {
			textColor(0x02);
		}
		printf("YES");
	}
    else
    {
		if(optionsmenuselected == 1)
        {
			textColor(0x0C);
		}
        else
        {
			textColor(0x04);
		}
		printf("NO");
	}
	if(optionsmenuselected == 1)
    {
		textColor(0x0F);
	}
    else
    {
		textColor(0x08);
	}
	printf("]");

	iSetCursor(26,38);
	DrawMenuPoint("Done",optionsmenuselected,2);

}

void DrawSoloGameMenu()
{
	currentmenu=MENU_SOLOGAME;
	DrawMenuStructure();

	textColor(0x0D);
	DrawText("Local game type:",21,24);

	iSetCursor(26,29);
	DrawMenuPoint("1 player",sologamemenuselected,0);

	iSetCursor(26,31);
	DrawMenuPoint("2 players",sologamemenuselected,1);

	iSetCursor(26,38);
	DrawMenuPoint("Return to game menu",sologamemenuselected,2);
}

void DrawLANGameMenu()
{
	currentmenu=MENU_LANGAME;
	DrawMenuStructure();

	textColor(0x0D);
	DrawText("LAN game:",21,24);

	iSetCursor(26,29);
	DrawMenuPoint("Join game",langamemenuselected,0);

	iSetCursor(26,31);
	DrawMenuPoint("Host game",langamemenuselected,1);

	iSetCursor(26,38);
	DrawMenuPoint("Return to game menu",langamemenuselected,2);
}

void DrawInternetGameMenu()
{
	currentmenu=MENU_INTERNETGAME;
	DrawMenuStructure();

	textColor(0x0D);
	DrawText("Internet game:",21,24);

	iSetCursor(26,29);
	DrawMenuPoint("Join game",internetgamemenuselected,0);

	iSetCursor(26,31);
	DrawMenuPoint("Host game",internetgamemenuselected,1);

	iSetCursor(26,38);
	DrawMenuPoint("Return to game menu",internetgamemenuselected,2);
}

void DrawCreditsMenu()
{
	currentmenu=MENU_CREDITS;
	DrawMenuStructure();

	textColor(0x0D);
	DrawText("Credits:",21,24);

	textColor(0x0E);
	DrawText("Created by Christian F. Coors",21,28);

	textColor(0x09);
	DrawText("Copyright (c) 2011",21,31);
	DrawText("Christian F. Coors,",21,33);
	DrawText("All rights reserved",21,35);

	iSetCursor(26,38);
	DrawMenuPoint("Done",creditsmenuselected,0);

}

void DrawExitMenu()
{
	currentmenu=MENU_EXIT;
	DrawMenuStructure();

	textColor(0x0D);
	DrawText("Do you really want to exit?",21,24);

	iSetCursor(26,30);
	DrawMenuPoint("Yes",exitmenuselected,0);
	iSetCursor(26,32);
	DrawMenuPoint("No",exitmenuselected,1);

}

void DrawGameMenu()
{
	currentmenu=MENU_GAME;
	DrawMenuStructure();

	textColor(0x0D);
	DrawText("New game / Gametype:",21,24);


	iSetCursor(26,29);
	DrawMenuPoint("Local game",gamemenuselected,0);
	iSetCursor(26,31);
	DrawMenuPoint("Local network (LAN) game",gamemenuselected,1);
	iSetCursor(26,33);
	DrawMenuPoint("Internet game",gamemenuselected,2);

	iSetCursor(26,36);
	DrawMenuPoint("Return to main menu",gamemenuselected,3);

}

void DrawMenuStructure()
{
	DrawMenuHeader();
	DrawMenuContentBox(0);
}

void DrawMenuPoint(char* name, uint8_t currentlyselected, uint8_t id)
{
	if(currentlyselected == id)
    {
		textColor(0x0E);
	}
    else
    {
		textColor(0x08);
	}
	printf("[ ");

	if( ((getCurrentMilliseconds()/ (1000/BLINKFREQ)) % 2) == 1 && currentlyselected == id)
    {
		printf("X");
	}
    else
    {
		printf(" ");
	}
	printf(" ]");
	if(currentlyselected == id)
    {
		textColor(0x0F);
	}
    else
    {
		textColor(0x08);
	}
	printf(" %s",name);
}

void DrawMenuSlideIn()
{
	if(option_menuanimation)
    {
		for(uint16_t y = 26; y>0; y=(y-2))
        {
			clearScreen2();
			DrawMenuContentBox(y);
			DrawMenuHeader();
			FlipIfNeeded();
			sleep(SLIDEDELAY);
		}
	}
	clearScreen(0x00);
}

void DrawMenuSlideOut()
{
	if(option_menuanimation)
    {
		for(uint16_t y = 0; y<25; y=(y+2))
        {
			clearScreen2();
			DrawMenuContentBox(y);
			DrawMenuHeader();
			FlipIfNeeded();
			sleep(SLIDEDELAY);
		}
	}
	clearScreen(0x00);
}

void SwitchToMenu(uint8_t switchto)
{
	DrawMenuSlideOut();
	if(switchto == MENU_EXIT)
    {
		exitmenuselected = 0;
	}

	if(switchto == MENU_MAIN)
    {
		mainmenuselected = 0;
	}

	if(switchto == MENU_GAME && gamemenuselected == (MENU_GAME_SELECTABLE-1))
    {
		gamemenuselected = 0;
	}

	currentmenu = switchto;
	DrawMenuSlideIn();
}

void DrawMenuContentBox(uint16_t addlines)
{
	iSetCursor(0,22);
	for(uint16_t i = 0; i<addlines; i++)
    {
		printf("\n");
	}

	textColor(0x0F);

	bool do_exit = false;
	for(uint16_t line = 0; line<25;line++)
    {
		if((line+addlines+22)>49)
        {
			do_exit = true;
		}

		textColor(0x0F);

		if(do_exit == false)
        {
			switch (line)
            {
				case 0:
					printf("                   ");
					//intf("/######################################\\\n");
					printf("##################################### \n");
					break;
				case 1:
					printf("                   ");
					printf("#");
					printf("				        ");
					printf("#\n");
					break;
				case 2:
					printf("                   ");
					printf("#");
					printf("			   	         ");
					printf("#\n");
					break;
				case 4:
					printf("                   ");
					printf("#");
					textColor(0x07);
					//intf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
					printf("======================================");
					textColor(0x0F);
					printf("#\n");
					break;
				case 17:
					printf("                    ");
					printf("#");
					printf("				          ");
					printf("#\n");
					break;
				case 18:
					printf("                     ");
					printf("#");
					printf("				          ");
					printf("#\n");
					break;
				case 19:
					printf("                   ");
					//intf("/######################################\\\n");
					printf("   #####################################\n");
					break;
				case 20:
				case 21:
				case 23:
					printf("\n");
					break;
				case 22:
					textColor(0x0E);
					printf("  Use the arrow keys to navigate and Enter or the Spacebar to select.\n");
					textColor(0x0F);
					break;
				case 24:
					textColor(0x09);
					printf("  If you can't use the arrow keys, you can use W and S instead.\n");
					break;
				default:
					printf("                   ");
					printf("#");
					printf("				          ");
					printf("#\n");
					break;
			}
		}
	}
}

void Sound_Denied()
{
	if(option_sound)
    {
		beep(600,150);
		beep(400,150);
	}
}

void Sound_OK()
{
	if(option_sound)
    {
		beep(800,100);
		beep(1000,100);
	}
}

void Sound_Select()
{
	if(option_sound)
    {
		beep(1000,50);
	}
}

void Sound_Error()
{
	if(option_sound) 
    {
		Sound_Denied();
		sleep(500);
	}
}

void DrawRect(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2)
{
	for(uint16_t lx = x; lx<x2; lx++)
    {
		for(uint16_t ly = y; ly<y2; ly++)
        {
			SetPixel(lx,ly);
		}
	}
}

void DrawText(char* text, uint16_t x, uint16_t y)
{
	iSetCursor(x,y);
	printf("%s",text);
}

void SetPixel(uint16_t x, uint16_t y)
{
	iSetCursor(x,y);
	putchar('x');
}

void SetSpace(uint16_t x, uint16_t y)
{
	iSetCursor(x,y);
	putchar(' ');
}

void FlipIfNeeded()
{
    if (DOUBLEBUFFERING)
    {
	    refreshScreen();
    }
}

void WaitKey()
{
	char buffer[8192];
    EVENT_t ev = event_poll(buffer, 8192, EVENT_NONE);

	bool do_exit = false;
    while(do_exit==false)
    {
        switch(ev)
        {
            case EVENT_NONE:
			{
                waitForEvent(10);
                break;
			}
			case EVENT_KEY_DOWN:
				do_exit = true;
				break;
			default:
				break;
		}

		ev = event_poll(buffer, 8192, EVENT_NONE);
	}
}

void NotImplementedError()
{
	Error("This feature is not (yet) implemented.",false);
}

void Error(char* message, bool critical)
{
	if(appmode==APPMODE_MENU)
    {
		DrawMenuSlideOut();
	}
	clearScreen(0x00);
	iSetCursor(10,7);
	textColor(0x0F);
	printf("%s",message);

	iSetCursor(10,9);
	if(critical == false)
    {
		printf("Press any key to continue...");
	}
    else
    {
		printf("Press any key to exit...");
		exitapp = true;
	}

	printf("\n\n\n\n\n\n\n");
	printf("                              ");
	textColor(0x04);
	printf("#####         #####\n");
	printf("                              ");
	printf("#");
	textColor(0x0C);
	printf("#####       #####");
	textColor(0x04);
	printf("#\n");
	printf("                               ");
	textColor(0x04);
	printf("#");
	textColor(0x0C);
	printf("####");
	textColor(0x04);
	printf("#     #");
	textColor(0x0C);
	printf("####");
	textColor(0x04);
	printf("#\n");
	printf("                                ");
	textColor(0x0C);
	printf("#####     #####\n");
	printf("                                ");
	textColor(0x04);
	printf("#");
	textColor(0x0C);
	printf("####");
	textColor(0x04);
	printf("#   #");
	textColor(0x0C);
	printf("####");
	textColor(0x04);
	printf("#\n");
	printf("                                 #");
	textColor(0x0C);
	printf("####");
	textColor(0x04);
	printf("# #");
	textColor(0x0C);
	printf("####");
	textColor(0x04);
	printf("#\n");
	printf("                                  ");
	textColor(0x0C);
	printf("##### #####\n");
	printf("                                  ");
	textColor(0x04);
	printf("#");
	textColor(0x0C);
	printf("#########");
	textColor(0x04);
	printf("#\n");
	printf("                                   #");
	textColor(0x0C);
	printf("#######");
	textColor(0x04);
	printf("#\n");
	printf("                                    ");
	textColor(0x0C);
	printf("#######\n");
	printf("                                    ");
	textColor(0x04);
	printf("#");
	textColor(0x0C);
	printf("#####");
	textColor(0x04);
	printf("#\n");
	printf("                                    #");
	textColor(0x0C);
	printf("#####");
	textColor(0x04);
	printf("#\n");
	printf("                                   #");
	textColor(0x0C);
	printf("#######\n");
	printf("                                   #########\n");
	printf("                                  ");
	textColor(0x04);
	printf("#");
	textColor(0x0C);
	printf("#########");
	textColor(0x04);
	printf("#\n");
	printf("                                 #");
	textColor(0x0C);
	printf("##### #####\n");
	printf("                                 #####");
	textColor(0x04);
	printf("# #");
	textColor(0x0C);
	printf("#####\n");
	printf("                                ");
	textColor(0x04);
	printf("#");
	textColor(0x0C);
	printf("####");
	textColor(0x04);
	printf("#   #");
	textColor(0x0C);
	printf("####");
	textColor(0x04);
	printf("#\n");
	printf("                               #");
	textColor(0x0C);
	printf("#####     #####\n");
	printf("                               #####");
	textColor(0x04);
	printf("#     #");
	textColor(0x0C);
	printf("#####\n");
	printf("                              ");
	textColor(0x04);
	printf("#");
	textColor(0x0C);
	printf("####");
	textColor(0x04);
	printf("#       #");
	textColor(0x0C);
	printf("####");
	textColor(0x04);
	printf("#\n");
	printf("                              ");
	textColor(0x0C);
	printf("#####         #####\n");
	printf("\n");

	FlipIfNeeded();

	Sound_Error();


	// flushEvents(EVENT_TEXT_ENTERED);
	// getchar();

	WaitKey();

	clearScreen(0x00);

	if(appmode==APPMODE_MENU)
    {
		DrawMenuSlideIn();
	}

	iSetCursor(0,0);
	textColor(0x0F);
}

void DrawMenuHeader()
{
	iSetCursor(0,2);

	textColor(0x0E);
	printf("\n");
	printf("    Created: %s, %s from %s",__DATE__,__TIME__,__FILE__);
	printf("\n\n");
	printf("    ");textColor(0x99);
	printf("#######");textColor(0x09);
	printf("                               ");textColor(0x99);
	printf("#######\n");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("              ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("          ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##\n");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("              ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("          ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##\n");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf(" ");textColor(0x99);
	printf("####");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("####");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##########");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf(" ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("####");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("#####");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("#####\n");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf(" ");textColor(0x99);
	printf("###");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf(" ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf(" ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf(" ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##\n");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("#######");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("######");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("#######");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##\n");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("       ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("      ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("       ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##\n");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("       ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("      ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("####");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("       ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##\n");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("       ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("####");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("       ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##\n");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("       ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("####");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("        ");textColor(0x99);
	printf("####");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("   ");textColor(0x99);
	printf("#####\n");textColor(0x09);
	printf("                                    ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("                                 ");textColor(0x99);
	printf("##\n");textColor(0x09);
	printf("                                   ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("                              ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("  ");textColor(0x99);
	printf("##\n");textColor(0x09);
	printf("                                  ");textColor(0x99);
	printf("##");textColor(0x09);
	printf("                                ");textColor(0x99);
	printf("####\n\n");textColor(0x0E);
}

