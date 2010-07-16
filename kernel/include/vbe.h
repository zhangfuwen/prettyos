#ifndef VBE_H
#define VBE_H

// http://www.petesqbsite.com/sections/tutorials/tuts/vbe3.pdf

#include "os.h"

/*
Once you have set vesa up you will find it as easy as vga modes to program for.
First you test for vesa (in realmode) and set it up like this:
Code:

mov   dword [VESAInfo_Signature],'VBE2'
mov   ax,4f00h   ; Is Vesa installed ?
mov   di,VESA_Info    ; This is the address of our info block.
int   10h

cmp   ax,004Fh   ; Is vesa installed ?,
jne   near NoVesa2    ; If not print a mesage & quit.

mov   ax,4f01h   ; Get Vesa Mode information.
mov   di,Mode_Info   ; This is the address of how info block.
mov   cx,0x4101   ; 4112h = 32/24bit ; 0x4101 = 8bit ;4111h = 15bit (640*480)
and   cx,0xfff
int   10h

cmp   dword [VESAInfo_Signature], 'VESA'
jne   near NoVesa2

cmp   byte [VESAInfo_Version+1], 2
jb    NoVesa2   ; VESA version below 2.0

The above code will test for vesa2 and set up 640x480 x 256color, or jump to
error code to print a error message
(error message is not included in the above code).
If the above code is sucessfull it fills out the "VESA_Info" and "Mode_Info".

Once in pmode to plot a pixel we do this
Code:

mov  edi,[ModeInfo_PhysBasePtr]   ; This is = to mov   edi,0xA0000 in vga
add  edi, 640*6    ; This is = to x = 0, y = 6
mov  al,0x09   ; Color of pixel
stosb   ; Put whats in al at es:edi

To fill the screen to white, we do this:
Code:

mov   edi,[ModeInfo_PhysBasePtr]       
mov   ecx,640*480   ; Size of screen
mov   al,0xff   ; This is for the color of one pixel
rep   stosb

NOTE: we could make the above code a little faster by doing this
Code:

mov   edi,[ModeInfo_PhysBasePtr]       
mov   ecx,640*480/4   ; Size of screen
mov   eax,0xffffffff    ; This is for the color of one pixel
rep   stosd

Here are some things to remember 
1.  you should start like the above, but latter use off screen buffers, 
2. if you use a 24bit mode some graphic cards use 24bit others use 32bit, 
you need to test for this and use differant code. Also unless you can go 
to real mode or call realmode int's you will be stuck in the vesa mode you
set up in realmode.
*/

// SuperVGA information block 

typedef struct 
{
    uint8_t     VESASignature[4];       // 'VESA' 4 byte signature          
    uint16_t VESAVersion;            // VBE version number               
    
    // char   _far *OEMStringPtr;   // Pointer to OEM string            
	char*   OEMStringPtr;           // Pointer to OEM string            
    long    Capabilities;           // Capabilities of video card       
    
    // uint16_t   _far *VideoModePtr;  // Pointer to supported modes       
	uint16_t*  VideoModePtr;           // Pointer to supported modes       
    uint16_t   TotalMemory;            // Number of 64kb memory blocks     
    uint8_t    reserved[236];             // Pad to 256 byte block size       
} VgaInfoBlock;

// SuperVGA mode information block 

typedef struct 
{
    uint16_t   ModeAttributes;         // Mode attributes                  
    uint8_t       WinAAttributes;         // Window A attributes              
    uint8_t       WinBAttributes;         // Window B attributes              
    uint16_t   WinGranularity;         // Window granularity in k          
    uint16_t   WinSize;                // Window size in k                 
    uint16_t   WinASegment;            // Window A segment                 
    uint16_t   WinBSegment;            // Window B segment                 
    
    // void    _far *WinFuncPtr;    // Pointer to window function       
	// void*   WinFuncPtr;          // Pointer to window function       
    uint16_t   BytesPerScanLine;       // Bytes per scanline               
    uint16_t   XResolution;            // Horizontal resolution            
    uint16_t   YResolution;            // Vertical resolution              
    uint8_t    XCharSize;              // Character cell width             
    uint8_t    YCharSize;              // Character cell height            
    uint8_t    NumberOfPlanes;         // Number of memory planes          
    uint8_t    BitsPerPixel;           // Bits per pixel                   
    uint8_t    NumberOfBanks;          // Number of CGA style banks        
    uint8_t    MemoryModel;            // Memory model type                
    uint8_t    BankSize;               // Size of CGA style banks          
    uint8_t    NumberOfImagePages;     // Number of images pages           
    uint8_t    res1;                   // Reserved                         
    uint8_t    RedMaskSize;            // Size of direct color red mask    
    uint8_t    RedFieldPosition;       // Bit posn of lsb of red mask      
    uint8_t    GreenMaskSize;          // Size of direct color green mask  
    uint8_t    GreenFieldPosition;     // Bit posn of lsb of green mask    
    uint8_t    BlueMaskSize;           // Size of direct color blue mask   
    uint8_t    BlueFieldPosition;      // Bit posn of lsb of blue mask     
    uint8_t    RsvdMaskSize;           // Size of direct color res mask    
    uint8_t    RsvdFieldPosition;      // Bit posn of lsb of res mask      
    uint8_t    DirectColorModeInfo;    // Direct color mode attributes     
    uint8_t    res2[216];              // Pad to 256 byte block size       
} ModeInfoBlock;

typedef enum 
{
    memPL       = 3,                // Planar memory model              
    memPK       = 4,                // Packed pixel memory model        
    memRGB      = 6,                // Direct color RGB memory model    
    memYUV      = 7,                // Direct color YUV memory model    
} memModels;

uint32_t getVgaInfo(VgaInfoBlock* vgaInfo);
uint32_t getModeInfo(uint32_t mode, ModeInfoBlock* modeInfo);
uint32_t getVBEMode(void);
void setVBEMode(uint32_t mode);
void setBank(uint32_t bank);

void availableModes(void);
void initGraphics(uint32_t x,uint32_t y);

// unsigned uint8_t* pixel = vram + y*pitch + x*pixelwidth;

void putPixel(uint32_t x,uint32_t y,uint32_t color);

// rendering one of the character, given its font_data

// void draw_char(unsigned char* screen, where, font_char*);
// void draw_string(unsigned char* screen, where, char* input);

#endif
