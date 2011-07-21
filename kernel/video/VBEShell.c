/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "console.h"
#include "util.h"
#include "videoutils.h"
#include "kheap.h"
#include "keyboard.h"
#include "events.h"
#include "filesystem/fat12.h"
#include "executable.h"

#define MAX_CHAR_PER_LINE 75
#define ENTRY_CACHE_SIZE 10

uint32_t ypos = 0;
void eraseFirst(char* string)
{
    strcpy(string, string+1);
}
void insert(char* string, char c)
{
    for (int i = strlen(string)-1; i >= 0; i--)
    {
        string[i+1] = string[i];
    }
    *string = c;
}

static unsigned int cursorPos;
static unsigned int entryLength;

void drawEntryVBE(const char* entry)
{
	char RenderBufferVBE[81];
    snprintf(RenderBufferVBE, 80, "$> %s", entry);
    if(strlen(RenderBufferVBE) < MAX_CHAR_PER_LINE)
        memset(RenderBufferVBE+strlen(RenderBufferVBE), ' ', MAX_CHAR_PER_LINE-strlen(RenderBufferVBE));
    RenderBufferVBE[80] = 0;
    video_drawString(video_currentMode->device, RenderBufferVBE, 0, ypos);
}

char* formatPath(char* opath)
{
    bool insertPartition = false;
    bool insertELF = false;
    size_t length = strlen(opath) + 1;
    if(strchr(opath, ':') == 0)
    {
        length += 3;
        insertPartition = true;
    }
    char* pointpos = strchr(opath, '.');
    if(pointpos == 0 || strcmp(toupper(pointpos+1), "ELF") != 0) /// TODO: Do not use stoupper
    {
        length += 4;
        insertELF = true;
    }

    char* npath = malloc(length, 0, "VBEShell: npath");
	memset(npath, 0, length);
    char* retval = npath;
    if(insertPartition)
    {
        strcpy(npath, "1:|");
        npath += 3;
    }
    strcpy(npath, opath);
    npath += strlen(opath);
    if(insertELF)
    {
        strcpy(npath, ".ELF");
    }

    return(retval);
}


void startVBEShell()
{
    event_enable(true);
    char entry[MAX_CHAR_PER_LINE+1];
    char entryCache[ENTRY_CACHE_SIZE][MAX_CHAR_PER_LINE+1];
    int curEntry = -1;
    bool insertMode = false;

    memset(entryCache, 0, ENTRY_CACHE_SIZE*(MAX_CHAR_PER_LINE+1));

    while (!keyPressed(KEY_ESC))
    {
        entryLength = 0;
        cursorPos = 0;
        memset(entry, 0, MAX_CHAR_PER_LINE+1);
        drawEntryVBE(entry);

		while (!keyPressed(KEY_ESC))
        {
            union {char buffer[4]; KEY_t key;} buffer; // We are only interested in TEXT_ENTERED and KEY_DOWN events. They send 4 bytes at maximum
            waitForEvent(0);
            EVENT_t ev = event_poll(buffer.buffer, 4, EVENT_NONE);

            switch(ev)
            {
                case EVENT_TEXT_ENTERED:
                {
                    if(keyPressed(KEY_ESC) || keyPressed(KEY_LCTRL) || keyPressed(KEY_RCTRL)) // To avoid conflicts with strg/esc shortcuts in kernel
                        break;

                    unsigned char text = buffer.buffer[0];
                    if (text > 0x20 && (entryLength<MAX_CHAR_PER_LINE || (insertMode && entryLength <= MAX_CHAR_PER_LINE && cursorPos < entryLength)))
                    {
                        if (curEntry != -1)
                        {
                            strcpy(entry, entryCache[curEntry]);
                            curEntry = -1;
                        }
                        if (insertMode)
                        {
                            entry[cursorPos]=text;
                            if (cursorPos == entryLength)
                            {
                                entryLength++;
                            }
                        }
                        else
                        {
                            insert(entry+cursorPos, text);
                            ++entryLength;
                        }
                        ++cursorPos;
                    }
                    break;
                }
                case EVENT_KEY_DOWN:
                {
                    switch(buffer.key)
                    {
                        case KEY_BACK:
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
                            }
                            break;
                        case KEY_ENTER:
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
                            textColor(0x03);
                            video_drawString(video_currentMode->device, "\n$> ", 0, ypos);
                            video_drawString(video_currentMode->device, entry, 3, ypos);
                            video_drawString(video_currentMode->device, "<--\n", strlen(entry)+5, ypos);
							ypos += 16;
                            textColor(0x0F);
                            goto EVALUATION;
                            break;
                        case KEY_INS:
                            insertMode = !insertMode;
                            break;
                        case KEY_DEL:
                            if (cursorPos < entryLength)
                            {
                                if (curEntry != -1)
                                {
                                    strcpy(entry, entryCache[curEntry]);
                                    curEntry = -1;
                                }
                                eraseFirst(entry+cursorPos);
                                --entryLength;
                            }
                            break;
                        case KEY_HOME:
                            cursorPos = 0;
                            break;
                        case KEY_END:
                            cursorPos = entryLength;
                            break;
                        case KEY_ARRL:
                            if (cursorPos > 0)
                            {
                                cursorPos--;
                            }
                            break;
                        case KEY_ARRR:
                            if (cursorPos < entryLength)
                            {
                                cursorPos++;
                            }
                            break;
                        case KEY_ARRU:
                            if (curEntry < ENTRY_CACHE_SIZE-1 && *entryCache[curEntry+1] != 0)
                            {
                                ++curEntry;
                                entryLength = strlen(entryCache[curEntry]);
                                cursorPos = entryLength;
                            }
                            break;
                        case KEY_ARRD:
                            if (curEntry >= 0)
                            {
                                --curEntry;
                                if (curEntry == -1)
                                {
                                    entryLength = strlen(entry);
                                }
                                else
                                {
                                    entryLength = strlen(entryCache[curEntry]);
                                }
                                cursorPos = entryLength;
                            }
                            else
                            {
                                memset(entry, 0, MAX_CHAR_PER_LINE);
                                cursorPos = 0;
                                entryLength = 0;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            } //switch
            drawEntryVBE((curEntry == -1 ? entry : entryCache[curEntry]));
        } //while

EVALUATION: // evaluation of entry
        if((strcmp(entry, "help") == 0) || (strcmp(entry, "?") == 0))
        {
            video_drawString(video_currentMode->device, "Implemented Instructions: hi, help, ?, fdir, format and reboot\n", 0, ypos);
			ypos += 16;
        }
        else if(strcmp(entry, "hi") == 0)
        {
            video_drawString(video_currentMode->device, "I am PrettyOS. Always at your service!\n", 0, ypos);
			ypos += 16;
        }
        else if(strcmp(entry, "fdir") == 0)
        {
			flpydsk_read_directory();
        }
        else if(strcmp(entry, "format") == 0)
        {
            video_drawString(video_currentMode->device, "Please enter the partition path (for example: 'A:0:'): ", 0, ypos);
			ypos += 16;
            char part[20];
            gets(part);
            video_drawString(video_currentMode->device, "Please enter the filesystem type (1: FAT12, 2: FAT16, 3: FAT32): ", 0, ypos);
			ypos += 16;
            char type[20];
            gets(type);
            video_drawString(video_currentMode->device, "Please enter the volume label: ", 0, ypos);
			ypos += 16;
            char label[20];
            gets(label);
			ypos += 16;
			formatPartition(part, atoi(type), label);
        }
        else if(strcmp(entry, "reboot") == 0)
        {
            systemControl(REBOOT);
        }
        else if(strcmp(entry, "standby") == 0)
        {
            systemControl(STANDBY);
        }
        else if(strcmp(entry, "shutdown") == 0)
        {
            systemControl(SHUTDOWN);
        }
        else
        {
            video_drawString(video_currentMode->device, "File is being searched... ", 0, ypos);
			ypos += 16;

            size_t argc = 1;
            bool apostroph = false;
            // Find out argc
            for(size_t i = 0; entry[i] != 0; i++)
            {
                if(entry[i] == '"')
                    apostroph = !apostroph;

                if(entry[i] == ' ' && !apostroph) // argument end
                    argc++;
            }

            char** argv = malloc(sizeof(char*)*argc, 0, "VBEShell: argv");
            char* argstart = entry;
            size_t j = 0;
            for(size_t i = 0; entry[i] != 0; i++)
            {
                if(entry[i] == '"')
                    apostroph = !apostroph;

                if(entry[i] == ' ' && !apostroph) // argument end
                {
                    entry[i] = 0;
                    argv[j] = argstart;
                    argstart = entry+i+1;
                    j++;
                }
            }
            argv[j] = argstart;

			FS_ERROR error = executeFile(argv[0], argc, argv);
            switch(error)
            {
                case CE_GOOD:
                    video_drawString(video_currentMode->device, " Successfull.\n", 0, ypos);
					ypos += 16;
                    break;
                case CE_INVALID_FILENAME:
                    video_drawString(video_currentMode->device, " The path was malformed.\n", 0, ypos);
					ypos += 16;
                    break;
                case CE_FILE_NOT_FOUND:
                    argv[0] = formatPath(argv[0]);
                    error = executeFile(argv[0], argc, argv);
                    if(error != CE_GOOD)
                        video_drawString(video_currentMode->device, " File not found.\n", 0, ypos);
					ypos += 16;
                    break;
                default:
                    video_drawString(video_currentMode->device, " File load was not successful.\n", 0, ypos);
					ypos += 16;
                    break;
            }

            free(argv[0]);
            free(argv);
        }
    } //while
}

/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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