/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "vbe.h"
#include "util.h"
#include "task.h"
#include "video.h"

ModeInfoBlock_t modeInfoBlock;
ModeInfoBlock_t* mib = &modeInfoBlock;

VgaInfoBlock_t vgaInfoBlock;
VgaInfoBlock_t* vgaIB = &vgaInfoBlock;

VgaInfoBlock_t* pVga 	= (VgaInfoBlock_t*) (0x1000);
	
uint8_t* SCREEN = (uint8_t*) 0xA0000;

void switchToVideomode()
{
    memset((void*) 0xA0000, 0, 0xB8000 - 0xA0000);
    create_vm86_task(VM86_SWITCH_TO_VIDEO);
    waitForKeyStroke();
}

void switchToTextmode()
{
    create_vm86_task(VM86_SWITCH_TO_TEXT);
    waitForKeyStroke();
    refreshUserScreen();
}
void vgaDebug()
{
	printf("\ndebug: print VgaInfoBlock_t\n");
	printf("debug: %s\n", pVga->VESASignature);
	printf("debug: %i\n", pVga->VESAVersion);
	printf("debug: %s\n", pVga->OEMStringPtr);
	printf("debug: %i\n", pVga->Capabilities);
	printf("debug: %i\n", pVga->VideoModePtr);
	printf("debug: %i\n", pVga->TotalMemory);
	printf("debug: %i\n", pVga->reserved[236]);
}

void initGraphics(uint32_t x, uint32_t y, uint32_t pixelwidth)
{
    mib->XResolution  = x;
    mib->YResolution  = y;
    mib->BitsPerPixel = pixelwidth; // 256 colors
}

void setPixel(uint32_t x, uint32_t y, uint32_t color)
{
    // unsigned uint8_t* pixel = vram + y*pitch + x*pixelwidth;
    SCREEN[y * mib->XResolution + x * mib->BitsPerPixel/8] = color;
}

float sgn(float x)
{
  if (x < 0)
    return -1;
  else if (x > 0)
    return 1;
  else
    return 0;
}

uint32_t abs(uint32_t arg)
{
    if (arg < 0)
    arg = -arg;
    return (arg);
}

/* line  (DON`T USE IT, IT CRASH!)
*    draws a line using Bresenham's line-drawing algorithm, which uses 
*    no multiplication or division.
*/

void line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color)
{
  uint32_t i,dx,dy,sdx,sdy,dxabs,dyabs,x,y,px,py;

  dx=x2-x1;      // the horizontal distance of the line
  dy=y2-y1;      // the vertical distance of the line
  dxabs=fabs(dx);
  dyabs=fabs(dy);
  sdx=sgn(dx);
  sdy=sgn(dy);
  x=dyabs>>1;
  y=dxabs>>1;
  px=x1;
  py=y1;

  SCREEN[(py<<8)+(py<<6)+px]=color;

  if (dxabs>=dyabs) // the line is more horizontal than vertical
  {
    for(i=0;i<dxabs;i++)
    {
      y+=dyabs;
      if (y>=dxabs)
      {
        y-=dxabs;
        py+=sdy;
      }
      px+=sdx;
      setPixel(px,py,color);
    }
  }
  else // the line is more vertical than horizontal
  {
    for(i=0;i<dyabs;i++)
    {
      x+=dxabs;
      if (x>=dyabs)
      {
        x-=dyabs;
        px+=sdx;
      }
      py+=sdy;
      setPixel(px,py,color);
    }
  }
}

/* rect  
*    Draws a rectangle by drawing all lines by itself.
*
*/

void rect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, uint32_t color)
{
  uint32_t top_offset,bottom_offset,i,temp; //word

  if (top>bottom)
  {
    temp=top;
    top=bottom;
    bottom=temp;
  }
  if (left>right)
  {
    temp=left;
    left=right;
    right=temp;
  }

  top_offset=(top<<8)+(top<<6);
  bottom_offset=(bottom<<8)+(bottom<<6);

  for(i=left;i<=right;i++)
  {
    SCREEN[top_offset+i]=color;
    SCREEN[bottom_offset+i]=color;
  }
  for(i=top_offset;i<=bottom_offset;i+=mib->XResolution) //SCREEN_WIDTH
  {
    SCREEN[left+i]=color;
    SCREEN[right+i]=color;
  }
}

void drawCircle(uint32_t xm, uint32_t ym, uint32_t radius, uint32_t color)
{
    // http://en.wikipedia.org/wiki/Circle#Cartesian_coordinates
    for (uint32_t i=0; i<=2*radius; i++)
    {
        uint32_t x  = xm - radius + i;
        uint32_t y1 = ym + (uint32_t) sqrt(radius*radius - (x-xm)*(x-xm));
        uint32_t y2 = ym - (uint32_t) sqrt(radius*radius - (x-xm)*(x-xm));
        setPixel(x, y1, 9);
        setPixel(x, y2, 9);
    }
}

/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
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
