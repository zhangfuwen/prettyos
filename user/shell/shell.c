#include "userlib.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#define MAX_CHAR_PER_LINE 70
#define ENTRY_CACHE_SIZE 10

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

char RenderBuffer[81];
void drawEntry(const char* entry)
{
    memset(RenderBuffer, 0, 81);
    sprintf(RenderBuffer, "$> %s", entry);
    if(!(cursorPos == entryLength && entryLength < MAX_CHAR_PER_LINE))
    {
        insert(RenderBuffer+3+cursorPos, 'v'); insert(RenderBuffer+3+cursorPos, '%'); // inserting %v (it looks confusing ;) )
    }
    strcat(RenderBuffer, "  ");
    printLine(RenderBuffer, 40, 0x0B);
}

int main()
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
        textColor(0x0F);
        entryLength = 0; cursorPos = 0;
        memset(entry, 0, MAX_CHAR_PER_LINE+1);
        drawEntry(entry);

        while (true)
        {
            showInfo(1); // the train goes on...

            input = getchar();

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
                        drawEntry(entry);
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
                    textColor(0x0B);
                    printf("\n$> %s <--\n", entry);
                    textColor(0x0F);
                    printLine("$>                                                                              ", 40, 0x0B);
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
                        drawEntry(entry);
                    }
                    break;
                case 146: // Pos 1
                    cursorPos = 0;
                    drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
                    break;
                case 147: // END
                    cursorPos = entryLength;
                    drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
                    break;
                case 150: // Left Arrow
                    if (cursorPos > 0)
                    {
                        cursorPos--;
                        drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
                    }
                    break;
                case 151: // Up Arrow
                    if (curEntry < ENTRY_CACHE_SIZE-1 && *entryCache[curEntry+1] != 0)
                    {
                        for (; entryLength > 0; entryLength--)
                        {
                            putchar('\b'); //Clear row
                        }
                        ++curEntry;
                        entryLength = strlen(entryCache[curEntry]);
                        cursorPos = entryLength;
                        drawEntry(entryCache[curEntry]);
                    }
                    break;
                case 152: // Down Arrow
                    if (curEntry >= 0)
                    {
                        for (; entryLength > 0; entryLength--)
                        {
                            putchar('\b'); //Clear row
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
                            putchar('\b'); //Clear row
                        }
                        memset(entry, 0, MAX_CHAR_PER_LINE);
                        cursorPos = 0;
                    }
                    drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
                    break;
                case 153: // Right Arrow
                    if (cursorPos < entryLength)
                    {
                        cursorPos++;
                        drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
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
                            insert(entry+cursorPos, input);
                            ++entryLength;
                        }
                        ++cursorPos;
                        drawEntry(entry);
                    }
                    break;
            } //switch
            drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
        } //while

EVALUATION: // evaluation of entry
        textColor(0x02);
        if((strcmp(entry, "help") == 0) || (strcmp(entry, "?") == 0))
        {
            puts("Implemented Instructions: hi, help, ?, fdir, format and reboot\n");
        }
        else if(strcmp(entry, "hi") == 0)
        {
            puts("I am PrettyOS. Always at your service!\n");
        }
        else if(strcmp(entry, "fdir") == 0)
        {
            floppy_dir();
        }
        else if(strcmp(entry, "format") == 0)
        {
            puts("Please enter the partition path: ");
            char part[20];
            gets(part);
            puts("Please enter the filesystem type (1: FAT12, 2: FAT16, 3: FAT32): ");
            char type[20];
            gets(type);
            puts("Please enter the volume label: ");
            char label[20];
            gets(label);
            puts("\n");
            partition_format(part, atoi(type), label);
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
            puts("file is being searched...");

            // Adding ending .elf
            if(entry[strlen(entry)-4] != '.') // No ending, append ".elf"
            {
                strcat(entry, ".elf");
            }

            FS_ERROR error = execute(entry);
            switch(error)
            {
                case CE_GOOD:
                    puts("Successfull.\n");
                    break;
                case CE_INVALID_FILENAME:
                    puts("The path was not formatted well.\n");
                    break;
                case CE_FILE_NOT_FOUND:
                    puts("The file was not found. ");
                    char newPath[40];
                    strcpy(newPath,"1:/");
                    strcat(newPath, entry);
                    printf("Trying now %s.\n", newPath);
                    if(execute(newPath) != CE_GOOD)
                        puts("Not found on 1:/.\n");
                    break;
                default:
                    printf("File load was not successful. Error Code: %u\n", error);
                    break;
            }
        }
    } //while
    return 0;
}
