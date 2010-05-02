#include "userlib.h"
#define MAX_CHAR_PER_LINE 70
#define ENTRY_CACHE_SIZE 10

void* memset(void* dest, int8_t val, size_t count)
{
    int8_t* temp = (int8_t*)dest;
    for (; count != 0; count--) *temp++ = val;
    return dest;
}

void eraseFirst(char* string)
{
    strcpy(string, string+1);
}
void insert(char* string, char c)
{
    for (int i = strlen(string)-1; i >= 0; i--) {
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
    if(!(cursorPos == entryLength && entryLength < MAX_CHAR_PER_LINE)) {
        insert(RenderBuffer+3+cursorPos, 'v'); insert(RenderBuffer+3+cursorPos, '%'); // inserting %v (it looks confusing ;) )
    }
    strcat(RenderBuffer, " ");
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

            input = getch();

            switch (input) {
                case 8:   // Backspace
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
                case 10:  // Enter
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
                    printf("$> %s <--\n", entry);
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
                            putch('\b'); //Clear row
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
                            putch('\b'); //Clear row
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
                            putch('\b'); //Clear row
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
            }//switch
            drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
        }//while

EVALUATION: // evaluation of entry
        textColor(0x02);
        if ((strcmp(entry,"help") == 0) || (strcmp(entry,"?") == 0))
        {
            puts("Implemented Instructions: hi  help ?  fdir  fformat\n");
        }
        else if (strcmp(entry,"hi") == 0)
        {
            puts("I am PrettyOS. Always at your service!\n");
        }
        else if (strcmp(entry,"fdir") == 0)
        {
            floppy_dir();
        }
        else if (strcmp(entry,"fformat") == 0)
        {
            floppy_format("PrettyOS");
        }
        else
        {
            puts("file is being searched...\n");

            toupper(entry);

            uint8_t posPoint = 8;
            for (uint8_t i = 0; i < 8; i++)
            {
                if (entry[i]=='.')
                {
                    posPoint = i; // search point position
                }
            }

            char name[9];
            memset(name, 0x20, 8); name[8] = '\0';
            for (uint8_t i = 0; i < 8; i++)
            {
                if (i < posPoint && isalnum(entry[i]))
                {
                    name[i] = entry[i];
                }
                else
                {
                    name[i] = 0x20; // space
                }
            }

            char ext[4];
            memset(ext, '\0', 4);
            for (uint8_t i = 0; i < 3; i++)
            {
                ext[i] = entry[i + posPoint + 1];
            }

            if (ext[0]==0 && ext[1]==0 && ext[2]==0)
            {
                ext[0] = 'E';
                ext[1] = 'L';
                ext[2] = 'F';
            }

            if (floppy_load(name,ext))
            {
                puts("<-- Sorry, PrettyOS does not know this command.\n");
            }
        }//else
    }//while
    return 0;
}
