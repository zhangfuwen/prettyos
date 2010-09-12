/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "vbe.h"
#include "util.h"
#include "task.h"
#include "paging.h"
#include "font.h"

ModeInfoBlock_t mib;
VgaInfoBlock_t  vgaIB;
BitmapHeader_t  bh;

BitmapHeader_t* bh_get;

BGRQuadPacked_t* ScreenPal = (BGRQuadPacked_t*)0x1600;

uint8_t* SCREEN = (uint8_t*)0xE0000000; // video memory for supervga
position_t curPos = {0, 0};


// vm86
extern uintptr_t vm86_com_start;
extern uintptr_t vm86_com_end;

// bmp
extern BMPInfo_t bmp_start;
extern BMPInfo_t bmp_end;


void setVgaInfoBlock(VgaInfoBlock_t* VIB)
{
    memcpy(&vgaIB, VIB, sizeof(VgaInfoBlock_t));
}

void getModeInfo(uint16_t mode)
{
    *(uint16_t*)0x3600 = 0x4000|mode;
    waitForTask(create_vm86_task(VM86_MODEINFOBLOCK));
    memcpy(&mib, (void*)0x3600, sizeof(ModeInfoBlock_t));
}

ModeInfoBlock_t* getCurrentModeInfo()
{
    return(&mib);
}

void getVgaIB()
{
    waitForTask(create_vm86_task(VM86_VGAINFOBLOCK));
}

void switchToVideomode(uint16_t mode)
{
    *(uint16_t*)0x3600 = 0x4000|mode;
    waitForTask(create_vm86_task(VM86_SWITCH_TO_VIDEO));
	getModeInfo(mode);
    videomode = VM_VBE;
}

void switchToTextmode()
{
    waitForTask(create_vm86_task(VM86_SWITCH_TO_TEXT));
    videomode = VM_TEXT;
    refreshUserScreen();
}

//void setDisplayStart(uint16_t *xpos, uint16_t *ypos)
//{
//    // memcpy(*(0x1800), xpos, sizeof(xpos));
//    // memcpy(*(0x1802), ypos, sizeof(ypos));
//    waitForTask(create_vm86_task(VM86_SETDISPLAYSTART));
//}

uint32_t getDisplayStart()
{
    waitForTask(create_vm86_task(VM86_GETDISPLAYSTART));
    return (*(uint32_t*)0x1300);
    // [0x1300]; First Displayed Scan Line
    // [0x1302]; First Displayed Pixel in Scan Line
}

void printPalette(BGRQuadPacked_t* RGB)
{
    uint32_t xpos = 0;
    uint32_t ypos = 0;
    for(uint32_t j=0; j<256; j++)
    {
        vbe_drawRectFilled(xpos, ypos, xpos+5, ypos+5, j);
        xpos +=5;
        if(xpos >= 255)
        {
            ypos += 5;
            xpos = 0;
        }
    }
}

void setPalette(BGRQuadPacked_t* RGB)
{
    /*
    Format of VESA VBE palette entry:
    Offset   Size    Description
    00h      BYTE    red
    01h      BYTE    green
    02h      BYTE    blue
    03h      BYTE    alpha or alignment byte
    */
    ScreenPal = RGB;
    waitForTask(create_vm86_task(VM86_SETPALETTE));
}

// http://wiki.osdev.org/VGA_Hardware#VGA_Registers
void Set_DAC_C(uint8_t PaletteColorNumber, uint8_t Red, uint8_t Green, uint8_t Blue)
{
    outportb(0x03C6, 0xFF);
    outportb(0x03C8, PaletteColorNumber);
    outportb(0x03C9, Red);
    outportb(0x03C9, Green);
    outportb(0x03C9, Blue);
}

void Get_DAC_C(uint8_t PaletteColorNumber, uint8_t* Red, uint8_t* Green, uint8_t* Blue)
{
    outportb(0x03C6, 0xFF);
    outportb(0x03C7, PaletteColorNumber);
    *Red   = inportb(0x03C9);
    *Green = inportb(0x03C9);
    *Blue  = inportb(0x03C9);
}

void Write_DAC_C_Palette(uint8_t StartColor, uint8_t NumOfColors, uint8_t *Palette)
{
    outportb(0x03C6, 0xFF);
    outportb(0x03C8, StartColor); // first color to be input
    for(uint32_t i=0; i<NumOfColors*3; i++)
    {
        outportb(0x03C9, Palette[i]<<2);
    }
}

void Read_DAC_C_Palette(uint8_t StartColor, uint8_t NumOfColors, uint8_t* Palette)
{
    outportb(0x03C6, 0xFF);
    outportb(0x03C7, StartColor); // first color to be read
    for(uint32_t i=0; i<NumOfColors*3; i++)
    {
        Palette[i]=inportb(0x03C9);
    }
}

/*void setDACPalette(BGRQuadPacked_t* RGB)
{
    waitForTask(create_vm86_task(VM86_SETDACPALETTE));
}

uint32_t getDACPalette()
{
    waitForTask(create_vm86_task(VM86_GETDACPALETTE));
    return 0;
}*/

void setVideoMemory()
{
     uint32_t numberOfPages = vgaIB.TotalMemory * 0x10000 / PAGESIZE;
     SCREEN = (uint8_t*)paging_acquirePciMemory(mib.PhysBasePtr, numberOfPages);

     printf("\nSCREEN (phys): %X SCREEN (virt): %X\n",mib.PhysBasePtr, SCREEN);
     printf("\nVideo Ram %u MiB\n",vgaIB.TotalMemory/0x10);
}

inline void vbe_setPixel(uint32_t x, uint32_t y, uint32_t color)
{
    SCREEN[(y * mib.XResolution + x) * mib.BitsPerPixel/8] = color;
}

uint32_t vbe_getPixel(uint32_t x, uint32_t y)
{
    return SCREEN[(y * mib.XResolution + x) * mib.BitsPerPixel/8];
}

void vbe_drawBitmap(uint32_t xpos, uint32_t ypos, BMPInfo_t* bitmap)
{
    if(mib.BitsPerPixel == 8)
    {
        for(uint32_t j=0; j<256; j++)
        {
            // transfer from bitmap palette to packed RAMDAC palette
            Set_DAC_C(j, bitmap->bmicolors[j].red   >> 2,
                         bitmap->bmicolors[j].green >> 2,
                         bitmap->bmicolors[j].blue  >> 2);
        }
    }

    uint8_t* pixel = (uint8_t*)bitmap + sizeof(BMPInfo_t) + bitmap->header.Width * bitmap->header.Height;
    for(uint32_t y=0; y<bitmap->header.Height; y++)
    {
        for(uint32_t x=bitmap->header.Width; x>0; x--)
        {
            if(mib.BitsPerPixel/8 == 3)
            {
                SCREEN[((xpos+x) + (ypos+y) * mib.XResolution) * mib.BitsPerPixel/8 + 0] = bitmap->bmicolors[*pixel].red;
                SCREEN[((xpos+x) + (ypos+y) * mib.XResolution) * mib.BitsPerPixel/8 + 1] = bitmap->bmicolors[*pixel].green;
                SCREEN[((xpos+x) + (ypos+y) * mib.XResolution) * mib.BitsPerPixel/8 + 2] = bitmap->bmicolors[*pixel].blue;
            }
            else
            {
                vbe_setPixel(xpos+x, ypos+y, *pixel);
            }
            pixel -= bitmap->header.BitsPerPixel/8;
        }
    }
}

void vbe_drawBitmapTransparent(uint32_t xpos, uint32_t ypos, BMPInfo_t* bitmap)
{
    if(mib.BitsPerPixel == 8)
    {
        for(uint32_t j=0; j<256; j++)
        {
            // transfer from bitmap palette to packed RAMDAC palette
            Set_DAC_C (j, bitmap->bmicolors[j].red   >> 2,
                          bitmap->bmicolors[j].green >> 2,
                          bitmap->bmicolors[j].blue  >> 2);
        }
    }

    uint8_t* pixel = (uint8_t*)bitmap + sizeof(BMPInfo_t) + bitmap->header.Width * bitmap->header.Height;
    for(uint32_t y=0; y<bitmap->header.Height; y++)
    {
        for(uint32_t x=bitmap->header.Width; x>0; x--)
        {
            if(bitmap->bmicolors[*pixel].red != 255 && bitmap->bmicolors[*pixel].green != 255 && bitmap->bmicolors[*pixel].blue != 255) // 0xF6 == WHITE
            {
                if(mib.BitsPerPixel/8 == 3)
                {
                    SCREEN[((xpos+x) + (ypos+y) * mib.XResolution) * mib.BitsPerPixel/8 + 0] = bitmap->bmicolors[*pixel].red;
                    SCREEN[((xpos+x) + (ypos+y) * mib.XResolution) * mib.BitsPerPixel/8 + 1] = bitmap->bmicolors[*pixel].green;
                    SCREEN[((xpos+x) + (ypos+y) * mib.XResolution) * mib.BitsPerPixel/8 + 2] = bitmap->bmicolors[*pixel].blue;
                }
                else
                {
                    vbe_setPixel(xpos+x, ypos+y, *pixel);
                }
            }
            pixel -= bitmap->header.BitsPerPixel/8;
        }
    }
}

void vbe_drawScaledBitmap(uint32_t newSizeX, uint32_t newSizeY, BMPInfo_t* bitmap)
{
    if(mib.BitsPerPixel == 8)
    {
        for(uint32_t j=0; j<256; j++)
        {
            // transfer from bitmap palette to packed RAMDAC palette
            Set_DAC_C (j, bitmap->bmicolors[j].red   >> 2,
                          bitmap->bmicolors[j].green >> 2,
                          bitmap->bmicolors[j].blue  >> 2);
        }
    }

    uint8_t* pixel = (uint8_t*)bitmap + sizeof(BMPInfo_t) + bitmap->header.Width * bitmap->header.Height;

    float FactorX = (float)newSizeX / (float)bitmap->header.Width;
    float FactorY = (float)newSizeY / (float)bitmap->header.Height;

    float OverflowX = 0, OverflowY = 0;
    for(uint32_t y = 0; y < newSizeY; y += (uint32_t)FactorY)
    {
        for(int32_t x = newSizeX-1; x >= 0; x -= (int32_t)FactorX)
        {
            pixel -= bitmap->header.BitsPerPixel/8;

            uint16_t rowsX = (uint16_t)FactorX + (OverflowX >= 1 ? 1:0);
            uint16_t rowsY = (uint16_t)FactorY + (OverflowY >= 1 ? 1:0);
            for(uint16_t i = 0; i < rowsX; i++)
            {
                for(uint16_t j = 0; j < rowsY; j++)
                {
                    if(x-i > 0)
                    {
                        if(mib.BitsPerPixel/8 == 3)
                        {
                            SCREEN[((x-i) + (y+j) * mib.XResolution) * mib.BitsPerPixel/8 + 0] = bitmap->bmicolors[*pixel].red;
                            SCREEN[((x-i) + (y+j) * mib.XResolution) * mib.BitsPerPixel/8 + 1] = bitmap->bmicolors[*pixel].green;
                            SCREEN[((x-i) + (y+j) * mib.XResolution) * mib.BitsPerPixel/8 + 2] = bitmap->bmicolors[*pixel].blue;
                        }
                        else
                        {
                            vbe_setPixel(x-i, y+j, *pixel);
                        }
                    }
                }
            }

            if(OverflowX >= 1)
            {
                OverflowX--;
                x--;
            }
            OverflowX += FactorX-(int)FactorX;
        }
        if(OverflowY >= 1)
        {
            OverflowY--;
            y++;
        }
        OverflowY += FactorY-(int)FactorY;
    }
}

// draws a line using Bresenham's line-drawing algorithm, which uses no multiplication or division.
void vbe_drawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color)
{
    uint32_t dx=x2-x1;      // the horizontal distance of the line
    uint32_t dy=y2-y1;      // the vertical distance of the line
    uint32_t dxabs=fabs(dx);
    uint32_t dyabs=fabs(dy);
    uint32_t sdx=sgn(dx);
    uint32_t sdy=sgn(dy);
    uint32_t x=dyabs>>1;
    uint32_t y=dxabs>>1;
    uint32_t px=x1;
    uint32_t py=y1;

    SCREEN[(py<<8)+(py<<6)+px]=color;

    if (dxabs>=dyabs) // the line is more horizontal than vertical
    {
        for(uint32_t i=0;i<dxabs;i++)
        {
            y+=dyabs;
            if (y>=dxabs)
            {
                y-=dxabs;
                py+=sdy;
            }
            px+=sdx;
            vbe_setPixel(px,py,color);
        }
    }
    else // the line is more vertical than horizontal
    {
        for(uint32_t i=0;i<dyabs;i++)
        {
            x+=dxabs;
            if (x>=dyabs)
            {
                x-=dyabs;
                px+=sdx;
            }
            py+=sdy;
            vbe_setPixel(px,py,color);
        }
    }
}

// Draws a rectangle by drawing all lines by itself.
void vbe_drawRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, uint32_t color)
{
    if (top>bottom)
    {
        uint32_t temp=top;
        top=bottom;
        bottom=temp;
    }
    if (left>right)
    {
        uint32_t temp=left;
        left=right;
        right=temp;
    }

    uint32_t top_offset = (top<<8)+(top<<6);
    uint32_t bottom_offset = (bottom<<8)+(bottom<<6);

    for(uint32_t i=left; i<=right; i++)
    {
        SCREEN[top_offset+i]=color;
        SCREEN[bottom_offset+i]=color;
    }
    for(uint32_t i=top_offset; i<=bottom_offset; i+=mib.XResolution) //SCREEN_WIDTH
    {
        SCREEN[left+i]=color;
        SCREEN[right+i]=color;
    }
}

// Draws a rectangle by drawing all lines by itself. Filled
void vbe_drawRectFilled(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, uint32_t color)
{
    for(int i = left; i < right; i++)
    {
        for(int j = top; j < bottom; j++)
        {
            SCREEN[(i + j * mib.XResolution) * mib.BitsPerPixel/8] = color;
        }
    }
}

// http://en.wikipedia.org/wiki/Circle#Cartesian_coordinates
void vbe_drawCircle(uint32_t xm, uint32_t ym, uint32_t radius, uint32_t color)
{
    for (uint32_t i=0; i<=2*radius; i++)
    {
        uint32_t x  = xm - radius + i;
        uint32_t y1 = ym + (uint32_t)sqrt(radius*radius - (x-xm)*(x-xm));
        uint32_t y2 = ym - (uint32_t)sqrt(radius*radius - (x-xm)*(x-xm));
        vbe_setPixel(x, y1, 9);
        vbe_setPixel(x, y2, 9);
    }
}

void vgaDebug()
{
    printf("\nDEBUG print: VgaInfoBlock, size: %x\n\n", sizeof(VgaInfoBlock_t));
    printf("VESA-Signature:         %s\n",     vgaIB.VESASignature);
    printf("VESA-Version:           %u.%u\n", (vgaIB.VESAVersion&0xFF00)>>8,vgaIB.VESAVersion&0xFF); // 01 02 ==> 1.2
    printf("Capabilities:           %X\n",     vgaIB.Capabilities);
    printf("Video Memory (MiB):     %u\n",     vgaIB.TotalMemory/0x10); // number of 64 KiB blocks of memory on the video card
    printf("OEM-String (address):   %X\n",     vgaIB.OEMStringPtr);
    printf("Video Modes Ptr:        %X\n",     vgaIB.VideoModePtr);

    // TEST Video Ram Paging
    printf("\nSCREEN (phys): %X SCREEN (virt): %X\n", mib.PhysBasePtr, SCREEN);

    // TEST
    //printf("OemVendorNamePtr:       %X\n",     vgaIB.OemVendorNamePtr);
    //printf("OemProductNamePtr:      %X\n",     vgaIB.OemProductNamePtr);
    //printf("OemProductRevPtr:       %X\n",     vgaIB.OemProductRevPtr);

    textColor(0x0E);
    printf("\nVideo Modes:\n");
    for (uint16_t i=0; i < 256; i++)
    {
        if(vgaIB.VideoModePtr[i] == 0xFFFF) break; // End of modelist
        getModeInfo(vgaIB.VideoModePtr[i]);
        textColor(0x0E);
        printf("\n%x = %ux%ux%u", vgaIB.VideoModePtr[i], mib.XResolution, mib.YResolution, mib.BitsPerPixel);
    }
    printf("\n");
    textColor(0x0F);

    waitForKeyStroke();

    printf("\nDEBUG print: ModeInfoBlock, size: %x\n\n", sizeof(ModeInfoBlock_t));
    printf("WinAAttributes:        %u\n", mib.WinAAttributes);
    printf("WinBAttributes:        %u\n", mib.WinBAttributes);
    printf("WinGranularity:        %u\n", mib.WinGranularity);
    printf("WinSize:               %u\n", mib.WinSize);
    printf("WinASegment:           %u\n", mib.WinASegment);
    printf("WinBSegment:           %u\n", mib.WinBSegment);
    printf("WinFuncPtr:            %X\n", mib.WinFuncPtr);
    printf("BytesPerScanLine:      %u\n", mib.BytesPerScanLine);
    printf("XResolution:           %u\n", mib.XResolution);
    printf("YResolution:           %u\n", mib.YResolution);
    printf("XCharSize:             %u\n", mib.XCharSize);
    printf("YCharSize:             %u\n", mib.YCharSize);
    printf("NumberOfPlanes:        %u\n", mib.NumberOfPlanes);
    printf("BitsPerPixel:          %u\n", mib.BitsPerPixel);
    printf("NumberOfBanks:         %u\n", mib.NumberOfBanks);
    printf("MemoryModel:           %u\n", mib.MemoryModel);
    printf("BankSize:              %u\n", mib.BankSize);
    printf("NumberOfImagePages:    %u\n", mib.NumberOfImagePages);
    printf("RedMaskSize:           %u\n", mib.RedMaskSize);
    printf("RedFieldPosition:      %u\n", mib.RedFieldPosition);
    printf("GreenMaskSize:         %u\n", mib.GreenMaskSize);
    printf("GreenFieldPosition:    %u\n", mib.GreenFieldPosition);
    printf("BlueMaskSize:          %u\n", mib.BlueMaskSize);
    printf("BlueFieldPosition:     %u\n", mib.BlueFieldPosition);
    printf("RsvdMaskSize:          %u\n", mib.RsvdMaskSize);
    printf("RsvdFieldPosition:     %u\n", mib.RsvdFieldPosition);
    printf("OffScreenMemOffset:    %u\n", mib.OffScreenMemOffset);
    printf("OffScreenMemSize:      %u\n", mib.OffScreenMemSize);
    printf("DirectColorModeInfo:   %u\n", mib.DirectColorModeInfo);
    printf("Physical Memory Base:  %X\n", mib.PhysBasePtr);
}

/*char ISValidBitmap(char* fname)
{
    BMPINFO bmpinfo;
    FILE *fp;
    if((fp = fopen(fname,"rb+"))==0)
    {
        printf("Unable open the file %s",fname,"!!");
        return 0;
    }

    fread(&bmpinfo,sizeof(bmpinfo),1,fp);
    fclose(fp);
    if(!(bmpinfo.bmiheader.bftype[0]=='B' &&
    bmpinfo.bmiheader.bftype[1]=='M'))
    {
        printf("can't read the file: not a valid BMP file!");
        return 0;
    }

    if(!bmpinfo.bmiheader.bicompression==0)
    {
        printf("can't read the file: should not be a RLR encoded!!");
        return 0;
    }

    if(!bmpinfo.bmiheader.bibitcount==8)
    {
        printf("can't read the file: should be 8-bit per color format!!");
        return 0;
    }

    return 1;
}*/

/*void showbitmap(char* infname,int xs,int ys)
{
    BMPINFO bmpinfo;
    RGB pal[256];
    FILE *fpt;
    int i,j,w,h,c,bank;
    unsigned char byte[1056];
    long addr;
    unsigned int k;
    if((fpt=fopen(infname,"rb+"))==0)
    {
        printf("Error opening file ");
        getch();
        return 1;
    }

    fread(&bmpinfo,sizeof(bmpinfo),1,fpt);
    fseek(fpt,bmpinfo.bmiheader.bfoffbits,SEEK_SET);
    w = bmpinfo.bmiheader.biwidth;
    h = bmpinfo.bmiheader.biheight;

    for(i=0;i<=255;i++)
    {
        pal[i].red = bmpinfo.bmicolors[i].red/4;
        pal[i].green = bmpinfo.bmicolors[i].green/4;
        pal[i].blue = bmpinfo.bmicolors[i].blue/4;
    }

    vinitgraph(VGALOW);
    setwidth(1000);
    SetPalette(pal);

    for(i=0;i<h;i++)
    {
        fread(&byte[0],sizeof(unsigned char),w,fpt);

        for(j=0;j<w;j++)
        {
            c= (int ) byte[j];
            addr= (long) (ys+h-i)*bytesperline+xs+j;
            bank = (int ) (addr >>16);
            if(curbank!= bank)
            {
                curbank =bank;
                bank<<=bankshift;
                _BX=0;
                _DX=bank;
                bankswitch();
                _BX=1;
                bankswitch();
            }
            *(screenptr+(addr & 0xFFFF)) = (char ) c;
        }
    }

    fclose(fpt);
    getch();
    vclosegraph();
    return 0;
}*/

void bitmapDebug() // TODO: make it bitmap-specific
{
    memcpy(&bh, bh_get, sizeof(BitmapHeader_t));

    printf("\nDEBUG print: BitmapHeader, size: %x\n\n", sizeof(BitmapHeader_t));
    printf("Type:                  %u\n", bh.Type);
    printf("Reserved:              %u\n", bh.Reserved);
    printf("Offset:                %u\n", bh.Offset);
    printf("Header Size:           %u\n", bh.headerSize);
    printf("Width:                 %u\n", bh.Width);
    printf("Height:                %u\n", bh.Height);
    printf("Planes:                %u\n", bh.Planes);
    printf("Bits Per Pixel:        %u\n", bh.BitsPerPixel);
    printf("Compression:           %u\n", bh.Compression);
    printf("Image Size:            %u\n", bh.SizeImage);
    printf("X-Pixels Per Meter:    %u\n", bh.XPixelsPerMeter);
    printf("Y-Pixels Per Meter:    %u\n", bh.YPixelsPerMeter);
    printf("Colors Used:           %u\n", bh.ColorsUsed);
    printf("Colors Important:      %u\n", bh.ColorsImportant);
}

void vbe_drawChar(char font_char)
{
    uint8_t uc = AsciiToCP437((uint8_t)font_char); // no negative values
    uint32_t xFont = 8, yFont = 16; // This info should be achievable from the font.h

    // mib.XResolution;
    // mib.YResolution;

    switch (uc)
    {
        case 0x08: // backspace: move the cursor one space backwards and delete
            /*move_cursor_left();
            putch(' ');
            move_cursor_left();*/
            break;
        case 0x09: // tab: increment cursor.x (divisible by 8)
            curPos.x = (curPos.x + xFont*8) & ~(xFont*8 - 1);
            /*if (currentConsole->cursor.x>=COLUMNS)
            {
                ++currentConsole->cursor.y;
                currentConsole->cursor.x=0;
                scroll();
            }*/
            break;
        case '\r': // r: cursor back to the margin
            curPos.x = 0;
            break;
        case '\n': // newline: like 'cr': cursor to the margin and increment cursor.y
            curPos.x = 0;
            curPos.y += yFont;
            break;
        default:
            if(curPos.x+xFont >= mib.XResolution) vbe_drawChar('\n');
            if (uc != 0)
            {
                for(uint32_t y=0; y < yFont; y++)
                {
                    for(uint32_t x=0; x<xFont; x++)
                    {
                        //SCREEN[ (xpos+x) + (ypos+y) * mib.XResolution * mib.BitsPerPixel/8 ] = font[(x + xFont*uc) + (yFont-y-1) * 2048];
                        vbe_setPixel(curPos.x+x, curPos.y+y, font[(x + xFont*uc) + (yFont-y-1) * 2048]);
                    }
                }
            }
            curPos.x += 8;
            break;
    }
}

void vbe_drawString(const char* text, uint32_t xpos, uint32_t ypos)
{
    curPos.x = xpos;
    curPos.y = ypos;
    for (; *text; vbe_drawChar(*text), ++text);
}

void vbe_bootscreen()
{
    textColor(0x09);
    printf("       >>>>>   Press 's' to skip VBE-Test or any key to continue   <<<<<\n\n");
    textColor(0x0F);
    if(getch() == 's')
    {
        return;
    }

    memcpy((void*)0x100, &vm86_com_start, (uintptr_t)&vm86_com_end - (uintptr_t)&vm86_com_start);

    #ifdef _VM_DIAGNOSIS_
    printf("vm86 binary code at 0x100: ");
    memshow((void*)0x100, (uintptr_t)&vm86_com_end - (uintptr_t)&vm86_com_start);
    printf("\n");
    #endif

    bh_get = (BitmapHeader_t*)&bmp_start;

    getVgaIB();

    setVgaInfoBlock((VgaInfoBlock_t*)0x3400);

    textColor(0x0E);
    printf("Select Resolution (given by its number):\n");
    textColor(0x0F);
    printf("1. 640x480x256\n");
    printf("2. 800x600x256\n");
    printf("3. 1024x768x256 (Default mode WORKING!)\n");
    printf("4. 1024x768x32k (TESTING!)\n");
    printf("5. 1024x768x16M (TESTING!)\n");
    uint8_t selectMode = getch();
    switch(selectMode)
    {
        case '1':
            getModeInfo(MODE_640x480x8);
            break;
        case '2':
            getModeInfo(MODE_800x600x8);
            break;
        case '3':
            getModeInfo(MODE_1024x768x8);
            break;
        case '4':
            getModeInfo(MODE_1024x768x16);
            break;
        case '5': default:
            getModeInfo(MODE_1024x768x24);
            break;
    }

    vgaDebug();

    setVideoMemory();

    switch(selectMode)
    {
        case '1':
            switchToVideomode(MODE_640x480x8);
            break;
        case '2':
            switchToVideomode(MODE_800x600x8);
            break;
        case '3':
            switchToVideomode(MODE_1024x768x8);
            break;
        case '4':
            switchToVideomode(MODE_1024x768x16);
            break;
        case '5': default:
            switchToVideomode(MODE_1024x768x24);
            break;
    }

    vbe_drawLine(0, mib.YResolution/2 + 1, mib.XResolution, mib.YResolution/2 + 1, 0x09); // FPU
    vbe_drawLine(mib.XResolution/2, 0, mib.XResolution/2, mib.YResolution, 0x09); // FPU

    vbe_drawCircle(mib.XResolution/2, mib.YResolution/2, mib.YResolution/2, 0x01); // FPU
    waitForKeyStroke();

    // vbe_drawBitmap(320,0,&cursor_start);

    vbe_drawBitmap(0, 0, &bmp_start);
    waitForKeyStroke();

    printPalette(ScreenPal);

    vbe_drawString("PrettyOS started in March 2009.\nThis hobby OS tries to be a possible access for beginners in this area.", 0, 400);
    waitForKeyStroke();

    vbe_drawScaledBitmap(mib.XResolution, mib.YResolution, (BMPInfo_t*)&bmp_start);

    uint32_t displayStart = getDisplayStart();
    waitForKeyStroke();

    switchToTextmode();

    printf("\nFirst Displayed Scan Line: %u, First Displayed Pixel in Scan Line: %u", (displayStart & 0xFFFF0000)>>16, displayStart & 0xFFFF);

    waitForKeyStroke();
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
