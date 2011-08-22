#ifndef TEXTGUI_H
#define TEXTGUI_H

#include "os.h"


#define TEXTGUI_OK        1
#define TEXTGUI_YES       2
#define TEXTGUI_NO        3
#define TEXTGUI_ABORTED  10


uint16_t TextGUI_ShowMSG(const char* title, const char* message);
uint16_t TextGUI_AskYN(const char* title, const char* message, uint8_t defaultselected);


#endif
