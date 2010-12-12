#include "gui_window.h"
#include "vbe.h"
#include "kheap.h"

extern ModeInfoBlock_t mib;


uint32_t* double_buffer;
// extern u32int* double_buffer;

BGRA_t WINDOW_COLOUR = {2, 255, 57, 0};
BGRA_t WINDOW_COLOUR_BACKGROUND = {191, 227, 197, 0};
BGRA_t WINDOW_COLOUR_BORDER = { 2, 125, 57, 0};
BGRA_t WINDOW_COLOUR_TOPBAR = {253, 100, 100, 0};
BGRA_t WINDOW_COLOUR_FOCUS_TOPBAR = {127, 255, 0, 0};

#define MAX_WINDOWS 256

volatile window_t current_window;
volatile window_t* window_list;

void init_window_manager()
{
    // We need to initialise the Desktop
    static window_t window;
    window.name = "Desktop";
    window.x = 0;
    window.y = 0;
    window.z = 0;
    window.width = mib.XResolution;
    window.height = mib.YResolution;
    window.parentid = 0;
    window.id = HWND_DESKTOP;
    window.data = double_buffer;

    current_window = window;

    window_list = (window_t*)malloc(sizeof(window_t)*MAX_WINDOWS, 0, "Window List");
    window_list[window.id] = current_window;
}

static uint16_t getnewwid()
{
    static uint32_t wid = 0;
    wid++;
    return wid;
}

void DestroyWindow(uint16_t id)
{
    window_list[id].data = 0;
    window_list[id].name = 0;
    window_list[id].id = 0;
    window_list[id].x = 0;
    window_list[id].y = 0;
    window_list[id].z = 0;
    window_list[id].width = 0;
    window_list[id].height = 0;
    window_list[id].parentid = 0;
}

void CreateWindow(char* windowname, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t parent)
{
    static window_t window;
    window.name = windowname;
    window.x = x;
    window.y = y;
    window.z = 1;
    window.width = width;
    window.height = height;
    window.parentid = parent;
    window.id = getnewwid();

    window.data = malloc((width*height)*(mib.BitsPerPixel/8), 0, "Window buffer"); // Creates buffer for window

    // Fill
    vbe_drawRectFilled(window.x, window.y+20, window.x+window.width, window.y+window.height, WINDOW_COLOUR_BACKGROUND);

    // Topbar
    vbe_drawRectFilled(window.x, window.y, window.x+window.width, window.y+20, WINDOW_COLOUR_TOPBAR);

    // Border
    vbe_drawRect(window.x, window.y, window.x+window.width, window.y+window.height, WINDOW_COLOUR_BORDER);

    // Title
    vbe_drawString(windowname, window.x+1, window.y+1);

    // refresh_screen();

    // And set window focus
    current_window = window;
    window_list[window.id] = current_window;
}
