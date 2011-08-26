#ifndef PONG_H
#define PONG_H

// Menu defines
#define MENU_MAIN                      1
#define MENU_GAME                     12
#define MENU_OPTIONS                  13
#define MENU_CREDITS                  14
#define MENU_EXIT                     15

#define MENU_SOLOGAME                120
#define MENU_LANGAME                 121
#define MENU_INTERNETGAME            122

#define MENU_MAIN_SELECTABLE           4

#define MENU_GAME_SELECTABLE           4
#define MENU_OPTIONS_SELECTABLE        3
#define MENU_CREDITS_SELECTABLE        1
#define MENU_EXIT_SELECTABLE           2

#define MENU_SOLOGAME_SELECTABLE       3
#define MENU_LANGAME_SELECTABLE        3
#define MENU_INTERNETGAME_SELECTABLE   3

// AppMode defines
#define APPMODE_MENU                   1
#define APPMODE_GAME                   2

// GameMode defines
#define GAMEMODE_NONE                  1
#define GAMEMODE_LOCALGAME_1P          2
#define GAMEMODE_LOCALGAME_2P          3
#define GAMEMODE_LANGAME_HOST          4
#define GAMEMODE_LANGAME_JOIN          5
#define GAMEMODE_INTERNETGAME_HOST     6
#define GAMEMODE_INTERNETGAME_JOIN     7

// Functions
void DrawMainMenu();
void DrawGameMenu();
void DrawOptionsMenu();
void DrawCreditsMenu();
void DrawExitMenu();
void DrawSoloGameMenu();
void DrawLANGameMenu();
void DrawInternetGameMenu();
void DrawMenuStructure();
void DrawMenuPoint(char* name, uint8_t currentlySelected, uint8_t id);
void DrawMenuHeader();
void DrawMenuContentBox(uint16_t addlines);
void DrawMenuSlideIn();
void DrawMenuSlideOut();

void SwitchToMenu(uint8_t switchto);

void Menu_SelectorUp();
void Menu_SelectorDown();
void Menu_Select();

void DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void DrawText(char* text, uint16_t x, uint16_t y);
void SetPixel(uint16_t x, uint16_t y);
void SetSpace(uint16_t x, uint16_t y);
void FlipIfNeeded();
void clearScreen2();

void Sound_Denied();
void Sound_OK();
void Sound_Select();
void Sound_Error();
void Sound_Goal();
void Sound_GotIt();

void NotImplementedError();
void Error(char* message, bool critical);

void RenderApp();
void ResetBall();
void RunGame();
void UpdateGame();
void RenderGame();

void GetGameControl();
void WaitKey();

double random(double lower, double upper);

#endif
