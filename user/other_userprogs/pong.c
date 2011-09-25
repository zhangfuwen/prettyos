/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"
#include "pong.h"


// Parameters:
const uint8_t PLAYER_1_HEIGHT = 15;
const uint8_t PLAYER_2_HEIGHT = 15;
const uint8_t PLAYER_2_HEIGHT_SHORT = 1;
const double  XSPEEDLOW       =  1.5;
const double  XSPEEDHIGH      =  2.5;
const double  YSPEEDLOW       = -2.0;
const double  YSPEEDHIGH      =  2.0;
const uint8_t MAXSCORE        = 10;

// GFX:
const bool DOUBLEBUFFERING =  true;
const uint8_t BLINKFREQ    =   100;
const uint8_t SLIDEDELAY   =    10;
const uint8_t COLUMNS      =    80;
const uint8_t LINES        =    50;
const uint8_t SCOREX       =    10;
const uint8_t SCOREY       =     5;

// Default options:
bool option_sound          = true;
bool option_menuanimation  = true;

///////////////// Do not modify anything behind this line ///////////////////////////

// Variables
uint8_t currentmenu;
uint8_t main_menu_selected;
uint8_t game_menu_selected;
uint8_t options_menu_selected;
uint8_t credits_menu_selected;
uint8_t exit_menu_selected;
uint8_t sologame_menu_selected;
uint8_t langame_menu_selected;
uint8_t internetgame_menu_selected;

uint8_t appmode;
uint8_t gamemode;

int16_t player1y;       // Y Position on screen
int16_t player2y;
int16_t player1h;       // Height of player
int16_t player2h;
int16_t player1score;   // Player score
int16_t player2score;
int16_t player1game;    // Player games
int16_t player2game;

double  ballx;          // Ball X
double  bally;          // Ball Y
double  ballxspeed;     // Ball X-Speed
double  ballyspeed;     // Ball Y-Speed
bool    kickoff_to_the_right = true; // true (right), false (left)

bool    exitapp;

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

    // Enable Events
    event_enable(true);
    char buffer[8192];
    EVENT_t ev = event_poll(buffer, 8192, EVENT_NONE);

    // Selected menupoints
    main_menu_selected         = 0;
    game_menu_selected         = 0;
    options_menu_selected      = 0;
    credits_menu_selected      = 0;
    exit_menu_selected         = 0;
    sologame_menu_selected     = 0;
    langame_menu_selected      = 0;
    internetgame_menu_selected = 0;


    // Clear eventual messages
    clearScreen(0x00);

    // Set to menu
    appmode  = APPMODE_MENU;
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

                if(*key == KEY_ARRU || *key == KEY_W || *key == KEY_P)
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

                if(*key == KEY_ARRD || *key == KEY_S || *key == KEY_L)
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

// Play Game

void RenderGame()
{
    // Draw border around the playfield
    textColor(0x88); DrawRect(0, 0, COLUMNS,   LINES  );
    textColor(0x00); DrawRect(1, 1, COLUMNS-1, LINES-1);

    // Draw line in the middle
    textColor(0x11); DrawRect((COLUMNS/2),1,(COLUMNS/2)+1,LINES-1);

    // Draw players
    textColor(0xAA); DrawRect(3,         player1y, 5,         player1y+player1h);
    textColor(0xEE); DrawRect(COLUMNS-5, player2y, COLUMNS-3, player2y+player2h);

    // Draw ball
    iSetCursor(ballx-1,bally-1);
    textColor(0x0C); putchar('/');   // x-1
    textColor(0xCC); putchar('X');   // x
    textColor(0x0C); putchar('\\');  // x+1

    iSetCursor(ballx-1,bally);
    textColor(0xCC); putchar('X');   // x-1
                     putchar('X');   // x
                     putchar('X');   // x+1

    iSetCursor(ballx-1,bally+1);
    textColor(0x0C); putchar('\\');  // x-1
    textColor(0xCC); putchar('X');   // x
    textColor(0x0C); putchar('/');   // x+1

    // Draw Score and Game
    if (player1score == MAXSCORE)
    {
        player1game++;
    }

    if (player2score == MAXSCORE)
    {
        player2game++;
    }

    iSetCursor(SCOREX,SCOREY);
    textColor(0x07); printf("score: ");
    textColor(0x0D); printf("%u", player1score);
    textColor(0x07); printf(" : ");
    textColor(0x0D); printf("%u", player2score);

    iSetCursor(SCOREX,SCOREY+1);
    textColor(0x07); printf("game:  ");
    textColor(0x0D); printf("%u", player1game);
    textColor(0x07); printf(" : ");
    textColor(0x0D); printf("%u", player2game);

    if ( (player1score == MAXSCORE) || (player2score == MAXSCORE) )
    {
        player1score = player2score = 0;
        beep(220,100);
        beep(440,100);
        beep(880,100);

      #ifdef AI
        player1h -= 2*player1game;
        if (player1h < 5)
        {
            player1h = 5;
        }
      #endif
    }
}

void UpdateGame()
{
    ballx = (ballx + ballxspeed);
    bally = (bally + ballyspeed);

    //////////////////////
    // collision / goal //
    //////////////////////

    if((bally-2) < 0) // collision upper wall
    {
        ballyspeed = -ballyspeed;
        bally = 2;
    }

    if((bally+2) > LINES) // collision lower wall
    {
        ballyspeed = -ballyspeed;
        bally = (LINES - 3);
    }

    if( ballx > 5 && ballx < 7 && bally >= player1y && bally <= player1y + player1h ) // player1 got it
    {
        Sound_GotIt();

        if (ballxspeed < 0)
        {
            ballxspeed = -ballxspeed;
            ballx++;
            ballyspeed += random(-0.2,0.2);
        }
    }

    if( ballx > COLUMNS - 7 && ballx < COLUMNS - 5 && bally >= player2y && bally <= player2y + player2h ) // player2 got it
    {
        Sound_GotIt();

        if (ballxspeed > 0)
        {
            ballxspeed = -ballxspeed;
            ballx--;
            ballyspeed += random(-0.2,0.2);
        }
    }

    if( ballx < 2 && ballxspeed < 0 ) // goal left
    {
        player2score++;
        kickoff_to_the_right = true;
        ResetBall();
    }

    if( ballx > COLUMNS - 2 && ballxspeed > 0 ) // goal right
    {
        player1score++;
        kickoff_to_the_right = false;
        ResetBall();
    }

    GetGameControl(); // get input from player
}

void ResetBall()
{
    Sound_Goal();
    sleep(500);

    do
    {
        ballx = (COLUMNS/2);
        bally = (LINES/2);
        ballxspeed = random(XSPEEDLOW, XSPEEDHIGH);
        ballyspeed = random(YSPEEDLOW, YSPEEDHIGH);
    }
    while( (ballyspeed > -0.5 && ballyspeed < 0.5) || ballxspeed == 0);

    if((kickoff_to_the_right == true && ballxspeed<0) || (kickoff_to_the_right == false && ballxspeed>0))
    {
        ballxspeed = -(ballxspeed);
    }
}

void RunGame()
{
    switch (gamemode)
    {
        case GAMEMODE_NONE:
            Error("An unknown error occurred.",true);
            break;
        case GAMEMODE_LOCALGAME_1P:
            player2h = PLAYER_2_HEIGHT_SHORT;
            break;
        case GAMEMODE_LOCALGAME_2P:
            player2h = PLAYER_2_HEIGHT;
            break;
        default:
            Error("An unknown error occurred (10).",true);
            break;
    }

    char buffer[8192];
    EVENT_t ev = event_poll(buffer, 8192, EVENT_NONE);

    bool exitgame = false;

    player1h = PLAYER_1_HEIGHT;
    player1y = (LINES / 2) - (player1h / 2);

    player2y = (LINES / 2) - (player2h / 2);

    player1score = player1game = 0;
    player2score = player2game = 0;

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

// Menu

void Menu_SelectorUp()
{
    switch (currentmenu)
    {
        case MENU_MAIN:
            if(main_menu_selected == 0)
            {
                main_menu_selected = (MENU_MAIN_SELECTABLE-1);
            }
            else
            {
                main_menu_selected = (main_menu_selected-1);
            }
            Sound_Select();
            break;
        case MENU_EXIT:
            if(exit_menu_selected == 0)
            {
                exit_menu_selected = (MENU_EXIT_SELECTABLE-1);
            }
            else
            {
                exit_menu_selected = (exit_menu_selected-1);
            }
            Sound_Select();
            break;
        case MENU_GAME:
            if(game_menu_selected == 0)
            {
                game_menu_selected = (MENU_GAME_SELECTABLE-1);
            }
            else
            {
                game_menu_selected = (game_menu_selected-1);
            }
            Sound_Select();
            break;
        case MENU_CREDITS:
            Sound_Select();
            break;
        case MENU_OPTIONS:
            if(options_menu_selected == 0)
            {
                options_menu_selected = (MENU_OPTIONS_SELECTABLE-1);
            }
            else
            {
                options_menu_selected = (options_menu_selected-1);
            }
            Sound_Select();
            break;
        case MENU_SOLOGAME:
            if(sologame_menu_selected == 0)
            {
                sologame_menu_selected = (MENU_SOLOGAME_SELECTABLE-1);
            }
            else
            {
                sologame_menu_selected = (sologame_menu_selected-1);
            }
            Sound_Select();
            break;
        case MENU_LANGAME:
            if(langame_menu_selected == 0)
            {
                langame_menu_selected = (MENU_LANGAME_SELECTABLE-1);
            }
            else
            {
                langame_menu_selected = (langame_menu_selected-1);
            }
            Sound_Select();
            break;
        case MENU_INTERNETGAME:
            if(internetgame_menu_selected == 0)
            {
                internetgame_menu_selected = (MENU_INTERNETGAME_SELECTABLE-1);
            }
            else
            {
                internetgame_menu_selected = (internetgame_menu_selected-1);
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
            if(main_menu_selected == (MENU_MAIN_SELECTABLE-1))
            {
                main_menu_selected = 0;
            }
            else
            {
                main_menu_selected = (main_menu_selected+1);
            }
            Sound_Select();
            break;
        case MENU_EXIT:
            if(exit_menu_selected == (MENU_EXIT_SELECTABLE-1))
            {
                exit_menu_selected = 0;
            }
            else
            {
                exit_menu_selected = (exit_menu_selected+1);
            }
            Sound_Select();
            break;
        case MENU_CREDITS:
            Sound_Select();
            break;
        case MENU_GAME:
            if(game_menu_selected == (MENU_GAME_SELECTABLE-1))
            {
                game_menu_selected = 0;
            }
            else
            {
                game_menu_selected = (game_menu_selected+1);
            }
            Sound_Select();
            break;
        case MENU_OPTIONS:
            if(options_menu_selected == (MENU_OPTIONS_SELECTABLE-1))
            {
                options_menu_selected = 0;
            }
            else
            {
                options_menu_selected = (options_menu_selected+1);
            }
            Sound_Select();
            break;
        case MENU_SOLOGAME:
            if(sologame_menu_selected == (MENU_SOLOGAME_SELECTABLE-1))
            {
                sologame_menu_selected = 0;
            }
            else
            {
                sologame_menu_selected = (sologame_menu_selected+1);
            }
            Sound_Select();
            break;
        case MENU_LANGAME:
            if(langame_menu_selected == (MENU_LANGAME_SELECTABLE-1))
            {
                langame_menu_selected = 0;
            }
            else
            {
                langame_menu_selected = (langame_menu_selected+1);
            }
            Sound_Select();
            break;
        case MENU_INTERNETGAME:
            if(internetgame_menu_selected == (MENU_INTERNETGAME_SELECTABLE-1))
            {
                internetgame_menu_selected = 0;
            }
            else
            {
                internetgame_menu_selected = (internetgame_menu_selected+1);
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
            switch (main_menu_selected)
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
            switch (exit_menu_selected)
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
            switch (credits_menu_selected)
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
            switch (game_menu_selected)
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
            switch (options_menu_selected)
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
            switch (sologame_menu_selected)
            {
                case 0: // One player
                    gamemode=GAMEMODE_LOCALGAME_1P;
                    appmode=APPMODE_GAME;
                    RunGame();
                    appmode=APPMODE_MENU;
                    break;
                case 1: // Two players
                    gamemode=GAMEMODE_LOCALGAME_2P;
                    appmode=APPMODE_GAME;
                    RunGame();
                    appmode=APPMODE_MENU;
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
            switch (langame_menu_selected)
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
            switch (internetgame_menu_selected)
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


    if(main_menu_selected > 127)
    {
        main_menu_selected = (MENU_MAIN_SELECTABLE-1);
    }

    if(main_menu_selected > (MENU_MAIN_SELECTABLE-1))
    {
        main_menu_selected = 0;
    }

    iSetCursor(26,29);
    DrawMenuPoint("New game",main_menu_selected,0);
    iSetCursor(26,31);
    DrawMenuPoint("Options",main_menu_selected,1);
    iSetCursor(26,33);
    DrawMenuPoint("Credits",main_menu_selected,2);
    iSetCursor(26,37);
    DrawMenuPoint("Exit",main_menu_selected,3);

}

void DrawOptionsMenu()
{
    currentmenu=MENU_OPTIONS;
    DrawMenuStructure();

    textColor(0x0D);
    DrawText("Options:",21,24);


    iSetCursor(26,29);
    DrawMenuPoint("Sound",options_menu_selected,0);

    iSetCursor(38,29);
    if(options_menu_selected == 0)
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
        if(options_menu_selected == 0)
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
        if(options_menu_selected == 0)
        {
            textColor(0x0C);
        }
        else
        {
            textColor(0x04);
        }
        printf("NO");
    }
    if(options_menu_selected == 0)
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
    DrawMenuPoint("Menu animations",options_menu_selected,1);

    iSetCursor(48,31);
    if(options_menu_selected == 1)
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
        if(options_menu_selected == 1)
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
        if(options_menu_selected == 1)
        {
            textColor(0x0C);
        }
        else
        {
            textColor(0x04);
        }
        printf("NO");
    }
    if(options_menu_selected == 1)
    {
        textColor(0x0F);
    }
    else
    {
        textColor(0x08);
    }
    printf("]");

    iSetCursor(26,38);
    DrawMenuPoint("Done",options_menu_selected,2);

}

void DrawSoloGameMenu()
{
    currentmenu=MENU_SOLOGAME;
    DrawMenuStructure();

    textColor(0x0D);   DrawText("Local game type:",21,24);

    iSetCursor(26,29); DrawMenuPoint("1 player"            , sologame_menu_selected,0);
    iSetCursor(26,31); DrawMenuPoint("2 players"           , sologame_menu_selected,1);
    iSetCursor(26,38); DrawMenuPoint("Return to game menu" , sologame_menu_selected,2);
}

void DrawLANGameMenu()
{
    currentmenu=MENU_LANGAME;
    DrawMenuStructure();

    textColor(0x0D);    DrawText("LAN game:",21,24);

    iSetCursor(26,29);  DrawMenuPoint("Join game",langame_menu_selected,0);
    iSetCursor(26,31);  DrawMenuPoint("Host game",langame_menu_selected,1);
    iSetCursor(26,38);  DrawMenuPoint("Return to game menu",langame_menu_selected,2);
}

void DrawInternetGameMenu()
{
    currentmenu=MENU_INTERNETGAME;
    DrawMenuStructure();

    textColor(0x0D);    DrawText("Internet game:",21,24);

    iSetCursor(26,29);  DrawMenuPoint("Join game",internetgame_menu_selected,0);
    iSetCursor(26,31);  DrawMenuPoint("Host game",internetgame_menu_selected,1);
    iSetCursor(26,38);  DrawMenuPoint("Return to game menu",internetgame_menu_selected,2);
}

void DrawCreditsMenu()
{
    currentmenu=MENU_CREDITS;
    DrawMenuStructure();

    textColor(0x0D);
    DrawText("Credits:",21,24);

    textColor(0x09);
    DrawText("Copyright (c) 2011"   ,21,30);
    DrawText("The PrettyOS Project" ,21,32);
    DrawText("All rights reserved"  ,21,34);

    iSetCursor(26,38);
    DrawMenuPoint("Done",credits_menu_selected,0);

}

void DrawExitMenu()
{
    currentmenu=MENU_EXIT;
    DrawMenuStructure();

    textColor(0x0D);   DrawText("Do you really want to exit?",21,24);

    iSetCursor(26,30); DrawMenuPoint("Yes",exit_menu_selected,0);
    iSetCursor(26,32); DrawMenuPoint("No",exit_menu_selected,1);
}

void DrawGameMenu()
{
    currentmenu = MENU_GAME;
    DrawMenuStructure();

    textColor(0x0D);    DrawText("New game / Gametype:",21,24);

    iSetCursor(26,29);  DrawMenuPoint("Local game",game_menu_selected,0);
    iSetCursor(26,31);  DrawMenuPoint("Local network (LAN) game",game_menu_selected,1);
    iSetCursor(26,33);  DrawMenuPoint("Internet game",game_menu_selected,2);
    iSetCursor(26,36);  DrawMenuPoint("Return to main menu",game_menu_selected,3);
}

void DrawMenuStructure()
{
    DrawMenuHeader();
    DrawMenuContentBox(0);
}

void DrawMenuPoint(char* name, uint8_t currentlySelected, uint8_t id)
{
    if(currentlySelected == id)
    {
        textColor(0x0E);
    }
    else
    {
        textColor(0x08);
    }
    printf("[ ");

    if( ((getCurrentMilliseconds()/ (1000/BLINKFREQ)) % 2) == 1 && currentlySelected == id)
    {
        printf("X");
    }
    else
    {
        printf(" ");
    }
    printf(" ]");

    if(currentlySelected == id)
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
        exit_menu_selected = 0;
    }

    if(switchto == MENU_MAIN)
    {
        main_menu_selected = 0;
    }

    if(switchto == MENU_GAME && game_menu_selected == (MENU_GAME_SELECTABLE-1))
    {
        game_menu_selected = 0;
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
                    printf("#####################################\n");
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
                    printf("  If you can't use the arrow keys, you can use W and S or P and L instead.\n");
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


// GFX

void clearScreen2()
{
    clearScreen(0x00);
}

void DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    for(uint16_t lx = x1; lx<x2; lx++)
    {
        for(uint16_t ly = y1; ly<y2; ly++)
        {
            SetPixel(lx,ly);
        }
    }
}

void DrawText(char* text, uint16_t x, uint16_t y)
{
    iSetCursor(x,y);
    puts(text);
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

// Input

void GetGameControl()
{
    switch(gamemode)
    {
        case GAMEMODE_LOCALGAME_1P:
        {
            // Player 1

            if(keyPressed(KEY_ARRU) || keyPressed(KEY_P) || (keyPressed(KEY_W)))
            {
                player1y = (player1y - 3);
            }

            if(keyPressed(KEY_ARRD) || keyPressed(KEY_L) || keyPressed(KEY_S))
            {
                player1y = (player1y + 3);
            }

            if(player1y < 2)
            {
                player1y = 2;
            }

            if((player1y+player1h) > (LINES - 2))
            {
                player1y = ((LINES - 2) - player1h);
            }

            // Player 2 (AI)

            player2y = bally - player2h/2; // AI Strategy

            if(player2y < 2)
            {
                player2y = 2;
            }

            if((player2y+player2h) > (LINES - 2))
            {
                player2y = ((LINES - 2) - player2h);
            }

            break;
        }

        case GAMEMODE_LOCALGAME_2P:
        {
            // Player 1

            if(keyPressed(KEY_W))
            {
                player1y = (player1y - 3);
            }

            if(keyPressed(KEY_S))
            {
                player1y = (player1y + 3);
            }

            if(player1y < 2)
            {
                player1y = 2;
            }

            if((player1y+player1h) > (LINES - 2))
            {
                player1y = ((LINES - 2) - player1h);
            }

            // Player 2

            if(keyPressed(KEY_P))
            {
                player2y = (player2y - 3);
            }

            if(keyPressed(KEY_L))
            {
                player2y = (player2y + 3);
            }

            if(player2y < 2)
            {
                player2y = 2;
            }

            if((player2y+player2h) > (LINES - 2))
            {
                player2y = ((LINES - 2) - player2h);
            }
            break;
        }
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

// Error

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
    puts(message);

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

// Sound

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

void Sound_Goal()
{
    if(option_sound)
    {
        beep(200,50);
    }
}

void Sound_GotIt()
{
    if(option_sound)
    {
        beep(800,20);
    }
}

/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
