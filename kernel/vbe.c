/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
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
ModeInfoBlock_t* mob    = (ModeInfoBlock_t*) (0x1300);

BitmapHeader_t bitmapHeader;
BitmapHeader_t* bh = &bitmapHeader;

BitmapHeader_t* bh_get	= (BitmapHeader_t*) (0x1500);

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
	memcpy((void*)vgaIB, (void*)pVga, sizeof(VgaInfoBlock_t));
	memcpy((void*)mib, (void*)mob, sizeof(ModeInfoBlock_t));
	 

	printf("\nDEBUG print: VgaInfoBlock_t\n");
	printf("sizeof VgaInfoBlock_t:        %X\n",sizeof (VgaInfoBlock_t));
	
	printf("VESASignature:          %s\n", vgaIB->VESASignature);
	printf("VESAVersion:            %u\n", vgaIB->VESAVersion);
	printf("OEMStringPtr:           %X\n", vgaIB->OEMStringPtr);
	printf("Capabilities:           %u\n", vgaIB->Capabilities);
	printf("VideoModePtr:           %u\n", vgaIB->VideoModePtr);
	printf("TotalMemory:            %u\n", vgaIB->TotalMemory);
	printf("reserved:               %u\n", vgaIB->reserved[236]);

	
	waitForKeyStroke();
	
	printf("\nDEBUG print: ModeInfoBlock_t\n");
	printf("sizeof ModeInfoBlock_t:      %X\n",sizeof (ModeInfoBlock_t));
	
	printf("ModeAttributes:        %i\n", mib->ModeAttributes);
	printf("WinAAttributes:        %i\n", mib->WinAAttributes);
	printf("WinBAttributes:        %i\n", mib->WinBAttributes);
	printf("WinGranularity:        %i\n", mib->WinGranularity);
	printf("WinSize:               %i\n", mib->WinSize);
	printf("WinASegment:           %i\n", mib->WinASegment);
	printf("WinBSegment:           %i\n", mib->WinBSegment);
	printf("WinFuncPtr:            %X\n", mib->WinFuncPtr);
	printf("BytesPerScanLine:      %i\n", mib->BytesPerScanLine);
	printf("XResolution:           %u\n", mib->XResolution);
	printf("YResolution:           %u\n", mib->YResolution);
	printf("XCharSize:             %u\n", mib->XCharSize);
	printf("YCharSize:             %u\n", mib->YCharSize);
	printf("NumberOfPlanes:        %i\n", mib->NumberOfPlanes);
	printf("BitsPerPixel:          %i\n", mib->BitsPerPixel);
	printf("NumberOfBanks:         %i\n", mib->NumberOfBanks);
	printf("MemoryModel:           %i\n", mib->MemoryModel);
	printf("BankSize:              %i\n", mib->BankSize);
	printf("NumberOfImagePages:    %i\n", mib->NumberOfImagePages);
	printf("res1:                  %i\n", mib->res1);
	printf("RedMaskSize:           %i\n", mib->RedMaskSize);
	printf("RedFieldPosition:      %i\n", mib->RedFieldPosition);
	printf("GreenMaskSize:         %i\n", mib->GreenMaskSize);
	printf("GreenFieldPosition:    %i\n", mib->GreenFieldPosition);
	printf("BlueMaskSize:          %i\n", mib->BlueMaskSize);
	printf("BlueFieldPosition:     %i\n", mib->BlueFieldPosition);
	printf("RsvdMaskSize:          %i\n", mib->RsvdMaskSize);
	printf("RsvdFieldPosition:     %i\n", mib->RsvdFieldPosition);
	printf("DirectColorModeInfo:   %i\n", mib->DirectColorModeInfo);
	printf("res2:                  %i\n", mib->res2);
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

// http://www.karig.net/os/001c.html
void bitmap()
{
	// memcpy((void*)SCREEN, (void*)0x00117f99, 32000); // 64000 // sizeof(SCREEN));
	// memcpy((void*)SCREEN, (void*)0x1540, 32000);
	// memcpy((void*)SCREEN, (void*)0x1500, sizeof(BitmapHeader_t));
	
	/*
	int x = 0, y = 0;
	for(y=0;y<256;y++)
	  for(x=0;x<128;x++)
	   SCREEN[x+y*mib->XResolution]=(uint8_t*)0x00117f99+sizeof(BitmapHeader_t)[x+y*128];
		// SCREEN[x+y*mib->XResolution]=bitmap[x+y*128];
	*/
}

void bitmapDebug()
{
	memcpy((void*)bh, (void*)bh_get, sizeof(BitmapHeader_t));
	printf("\nDEBUG print: BitmapHeader\n");
	printf("sizeof BitmapHeader_t:      %X\n",sizeof (BitmapHeader_t));
	printf("Type:                       %u\n", bh->Type);
	printf("Reserved:                   %u\n", bh->Reserved);
	printf("Offset:                     %u\n", bh->Offset);
	printf("headerSize:                 %u\n", bh->headerSize);
	printf("Width:                      %u\n", bh->Width);
	printf("Height:                     %u\n", bh->Height);
	printf("Planes:                     %u\n", bh->Planes);
	printf("BitsPerPixel:               %u\n", bh->BitsPerPixel);
	printf("Compression:                %u\n", bh->Compression);
	printf("SizeImage:                  %u\n", bh->SizeImage);
	printf("XPixelsPerMeter:            %u\n", bh->XPixelsPerMeter);
	printf("YPixelsPerMeter:            %u\n", bh->YPixelsPerMeter);
	printf("ColorsUsed:                 %u\n", bh->ColorsUsed);
	printf("ColorsImportant:            %u\n", bh->ColorsImportant);

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
