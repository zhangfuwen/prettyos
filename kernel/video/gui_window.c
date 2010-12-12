#include "gui_window.h"
#include "vbe.h"
#include "kheap.h"

int wid = 0;
uint32_t g_wd = 1024; // mib.XResultion
uint32_t g_ht = 768; // mib.YResultion
uint32_t g_bpp = 16; // mib.BitsPerPixel

uint32_t double_buffer;
// extern u32int* double_buffer;

BGRA_t WINDOW_COLOUR = {2, 255, 57, 0};
BGRA_t WINDOW_COLOUR_BACKGROUND = {191, 227, 197, 0};
BGRA_t WINDOW_COLOUR_BORDER = { 2, 125, 57, 0};
BGRA_t WINDOW_COLOUR_TOPBAR = {44, 95, 131, 0x00};
BGRA_t WINDOW_COLOUR_FOCUS_TOPBAR = {127, 255, 0, 0x00};

#define MAX_WINDOWS 256

volatile window_t current_window;
volatile window_t* window_list;

void init_window_manager()
{
     //We need to initialise the Desktop
     static window_t window;
     window.name = "Desktop";
     window.x = 0;
     window.y = 0;
     window.z = 0;
     window.width = g_wd;
     window.height = g_ht;
     window.parentid = 0;
     window.id = HWND_DESKTOP;
     window.data = (unsigned long*)double_buffer;
     
     current_window = window;
     
     window_list = (window_t*)malloc(sizeof(window_t)*MAX_WINDOWS, 0, "Window List");
     window_list[window.id] = current_window;
}

static uint16_t getnewwid()
{
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
	 
     window.data = (unsigned long*)malloc((width*height)*(g_bpp/8), 0, "Window buffer"); //Creates buffer for window
     
     // Fill
	 vbe_drawRectFilled(window.x, 10, window.width, window.height, WINDOW_COLOUR_BACKGROUND);
	 
     // Topbar
     vbe_drawRectFilled(window.x, window.y, window.width, 10, WINDOW_COLOUR_TOPBAR);

     // Border
     int i;
     for(i=x;i<=window.width;i++)
	 {
		  vbe_setPixel(i, window.y, WINDOW_COLOUR_BORDER); // top
	 }
	 
     for(i=x;i<=window.width;i++)
	 {
		  vbe_setPixel(i, window.height, WINDOW_COLOUR_BORDER); // bottom
	 }
	 
     for(i=y;i<=window.height;i++)
     {
		  vbe_setPixel(x, i, WINDOW_COLOUR_BORDER); // left
		  vbe_setPixel(window.width, i, WINDOW_COLOUR_BORDER); //right

     }
     
	 
     /* Title */
	 vbe_drawString(windowname, window.x, window.y); 

     // refresh_screen();
     
     //And set window focus
     current_window = window;
     window_list[window.id] = current_window;
}
