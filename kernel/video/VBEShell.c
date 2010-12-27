/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// #include "userlib.h"
// #include "string.h"
// #include "stdio.h"
// #include "../user/user_tools/userlib.h"

#include "console.h"
#include "syscall.h"
#include "util.h"
#include "vbe.h"
#include "keyboard.h"


#define MAX_CHAR_PER_LINE 70
#define ENTRY_CACHE_SIZE 10

uint32_t ypos = 0;

extern ModeInfoBlock_t mib;

void eraseFirst(char* string)
{
    strcpy(string, string+1);
}

void insertVBE(char* string, char c)
{
    for (int i = strlen(string)-1; i >= 0; i--)
    {
        string[i+1] = string[i];
    }
    *string = c;
}

static unsigned int cursorPos;
static unsigned int entryLength;

char RenderBufferVBE[81]; // mib.XResolution / 8

void drawEntryVBE(const char* entry)
{
	
    memset(RenderBufferVBE, 0, 81);
    // sprintf(RenderBufferVBE, "$> %s", entry);
    if(!(cursorPos == entryLength && entryLength < MAX_CHAR_PER_LINE))
    {
        insertVBE(RenderBufferVBE+3+cursorPos, 'v'); insertVBE(RenderBufferVBE+3+cursorPos, '%'); // inserting %v (it looks confusing ;) )
    }
    strcat(RenderBufferVBE, "  ");
    // printLine(RenderBuffer, 40, 0x0B);
	// vbe_drawString(RenderBufferVBE, cursorPos, 0);
	// strcat((char*)entry, "\n");
	vbe_drawString(entry, cursorPos, ypos);
	// ypos += 16;
}

int startVBEShell()
{
    setScrollField(0, 39);
    char entry[MAX_CHAR_PER_LINE+1];
    char entryCache[ENTRY_CACHE_SIZE][MAX_CHAR_PER_LINE+1];
    int curEntry = -1;
    bool insertMode = false;
    unsigned char input;

    //Init Cache
    for (unsigned int i = 0; i < ENTRY_CACHE_SIZE+1; i++)
    {
        memset(entryCache, 0, MAX_CHAR_PER_LINE+1);
    }

    while (true)
    {
        // textColor(0x0F);
        entryLength = 0;
		cursorPos = 0;
        memset(entry, 0, MAX_CHAR_PER_LINE+1);
        drawEntryVBE(entry);

        while (true)
        {
            // showInfo(1); // the train goes on...

            // input = getchar();
			input = keyboard_getChar();
			
            switch (input) {
                case 8: // Backspace
                    if (cursorPos > 0)
                    {
                        if (curEntry != -1)
                        {
                            strcpy(entry, entryCache[curEntry]);
                            curEntry = -1;
                        }
                        eraseFirst(entry+cursorPos-1);
                        --entryLength;
                        --cursorPos;
                        drawEntryVBE(entry);
                    }
                    break;
                case 10: // Enter
                    if(*(curEntry == -1 ? entry : entryCache[curEntry]) == 0)
                    {
                        break; // entry is empty
                    }
                    cursorPos = entryLength+1;
                    entry[entryLength]='\0';
                    if (curEntry == -1)
                    {
                        //Insert entry
                        for (int i = ENTRY_CACHE_SIZE-2; i >= 0; i--)
                        {
                            strncpy(entryCache[i+1], entryCache[i], MAX_CHAR_PER_LINE);
                        }
                        strcpy(entryCache[0], entry);
                    }
                    else
                    {
                        //Move entry to front
                        strcpy(entry, entryCache[curEntry]);
                        for (int i = curEntry-1; i >= 0; i--)
                        {
                            strcpy(entryCache[i+1], entryCache[i]);
                        }
                        strcpy(entryCache[0], entry);
                        curEntry = -1;
                    }
                    // textColor(0x0B);
                    // printf("\n$> %s <--\n", entry);
					// sprintf(entry, "\n$> %s <--\n"); 
					vbe_drawString(entry, cursorPos, ypos);
					ypos += 16;
                    // textColor(0x0F);
                    // printLine("$>                                                                              ", 40, 0x0B);
                    goto EVALUATION;
                case 144: // Insert
                    insertMode = !insertMode;
                    break;
                case 145: // Delete
                    if (cursorPos < entryLength)
                    {
                        if (curEntry != -1)
                        {
                            strcpy(entry, entryCache[curEntry]);
                            curEntry = -1;
                        }
                        eraseFirst(entry+cursorPos);
                        --entryLength;
                        drawEntryVBE(entry);
                    }
                    break;
                case 146: // Pos 1
                    cursorPos = 0;
                    drawEntryVBE((curEntry == -1 ? entry : entryCache[curEntry]));
                    break;
                case 147: // END
                    cursorPos = entryLength;
                    drawEntryVBE((curEntry == -1 ? entry : entryCache[curEntry]));
                    break;
                case 150: // Left Arrow
                    if (cursorPos > 0)
                    {
                        cursorPos--;
                        drawEntryVBE((curEntry == -1 ? entry : entryCache[curEntry]));
                    }
                    break;
                case 151: // Up Arrow
                    if (curEntry < ENTRY_CACHE_SIZE-1 && *entryCache[curEntry+1] != 0)
                    {
                        for (; entryLength > 0; entryLength--)
                        {
                           // putchar('\b'); //Clear row
                        }
                        ++curEntry;
                        entryLength = strlen(entryCache[curEntry]);
                        cursorPos = entryLength;
                        drawEntryVBE(entryCache[curEntry]);
                    }
                    break;
                case 152: // Down Arrow
                    if (curEntry >= 0)
                    {
                        for (; entryLength > 0; entryLength--)
                        {
                            // putchar('\b'); //Clear row
                        }
                        --curEntry;
                        if (curEntry == -1)
                        {
                            entryLength = strlen(entry);
                            cursorPos = entryLength;
                        }
                        else
                        {
                            entryLength = strlen(entryCache[curEntry]);
                            cursorPos = entryLength;
                        }
                    }
                    if (curEntry == -1)
                    {
                        for (; entryLength > 0; entryLength--)
                        {
                            // putchar('\b'); //Clear row
							vbe_drawString("\b", cursorPos, ypos);
							ypos += 16;
                        }
                        memset(entry, 0, MAX_CHAR_PER_LINE);
                        cursorPos = 0;
                    }
                    drawEntryVBE((curEntry == -1 ? entry : entryCache[curEntry]));
                    break;
                case 153: // Right Arrow
                    if (cursorPos < entryLength)
                    {
                        cursorPos++;
                        drawEntryVBE((curEntry == -1 ? entry : entryCache[curEntry]));
                    }
                    break;
                default:
                    if (input >= 0x20 && (entryLength<MAX_CHAR_PER_LINE || (insertMode && entryLength <= MAX_CHAR_PER_LINE && cursorPos != entryLength)))
                    {
                        if (curEntry != -1)
                        {
                            strcpy(entry, entryCache[curEntry]);
                            curEntry = -1;
                        }
                        if (insertMode)
                        {
                            entry[cursorPos]=input;
                            if (cursorPos == entryLength)
                            {
                                entryLength++;
                            }
                        }
                        else
                        {
                            insertVBE(entry+cursorPos, input);
                            ++entryLength;
                        }
                        ++cursorPos;
                        drawEntryVBE(entry);
                    }
                    break;
            } //switch
            drawEntryVBE((curEntry == -1 ? entry : entryCache[curEntry]));
        } //while

EVALUATION: // evaluation of entry
        // textColor(0x02);
        if((strcmp(entry, "help") == 0) || (strcmp(entry, "?") == 0))
        {
            // puts("Implemented Instructions: hi, help, ?, fdir, fformat and reboot\n");
			vbe_drawString("Implemented Instructions: hi, help, ?, fdir, fformat, exit and reboot\n", cursorPos, ypos);
			ypos += 16;
        }
        else if(strcmp(entry, "hi") == 0)
        {
            // puts("I am PrettyOS. Always at your service!\n");
			vbe_drawString("I am PrettyOS. Always at your service!\n", cursorPos, ypos);
			ypos += 16;
        }
        else if(strcmp(entry, "fdir") == 0)
        {
            // floppy_dir();
			ypos += 16;
        }
        else if(strcmp(entry, "fformat") == 0)
        {
            // floppy_format("PrettyOS");
			ypos += 16;
        }
        else if(strcmp(entry, "reboot") == 0)
        {
            systemControl(REBOOT);
			ypos += 16;
        }
        else if(strcmp(entry, "standby") == 0)
        {
            systemControl(STANDBY);
			ypos += 16;
        }
        else if(strcmp(entry, "shutdown") == 0)
        {
            systemControl(SHUTDOWN);
			ypos += 16;
        }
		else if(strcmp(entry, "exit") == 0)
        {
            // return 1;
			break;
        }
        else
        {
            // puts("file is being searched...");
			vbe_drawString("file is being searched...", cursorPos, ypos);

            // Adding ending .elf
            if(entry[strlen(entry)-4] != '.') // No ending, append ".elf"
            {
                strcat(entry, ".elf");
            }

            FS_ERROR error = 0; //execute(entry);
            switch(error)
            {
                case CE_GOOD:
                    // puts("Successfull.\n");
					vbe_drawString("Successfull.\n", cursorPos, ypos);
					ypos += 16;
                    break;
                case CE_INVALID_FILENAME:
                    // puts("The path was not formatted well.\n");
					vbe_drawString("The path was not formatted well.\n", cursorPos, ypos);
					ypos += 16;
                    break;
                case CE_FILE_NOT_FOUND:
                    // puts("The file was not found. ");
					vbe_drawString("The file was not found. ", cursorPos, ypos);
					ypos += 16;
                    
					char newPath[40];
                    strcpy(newPath,"1:/");
                    strcat(newPath, entry);
                    // printf("Trying now %s.\n", newPath);
					vbe_drawString("Trying now %s.\n", cursorPos, ypos);
                    // if(execute(newPath) != CE_GOOD)
                        // puts("Not found on 1:/.\n");
                    break;
                default:
                    // printf("File load was not successful. Error Code: %u\n", error);
					vbe_drawString("File load was not successful. Error Code: %u\n", cursorPos, ypos);
					ypos += 16;
                    break;
            }
        }
    } //while
    return 0;
}

/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
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