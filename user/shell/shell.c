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
    if(strcmp(stoupper(strchr(opath, '.')+1), "ELF") != 0) /// TODO: Do not use stoupper and strrchr instead of strchr
    {
        length += 4;
        insertELF = true;
    }

    char* npath = calloc(length, 1);
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


int main()
{
    setScrollField(0, 39);
    event_enable(true);
    char entry[MAX_CHAR_PER_LINE+1];
    char entryCache[ENTRY_CACHE_SIZE][MAX_CHAR_PER_LINE+1];
    int curEntry = -1;
    bool insertMode = false;

    memset(entryCache, 0, ENTRY_CACHE_SIZE*(MAX_CHAR_PER_LINE+1));

    while (true)
    {
        textColor(0x0F);
        entryLength = 0; cursorPos = 0;
        memset(entry, 0, MAX_CHAR_PER_LINE+1);
        drawEntry(entry);

        while (true)
        {
            char buffer[4]; // We are only interested in TEXT_ENTERED and KEY_DOWN events. They send 4 bytes at maximum
            EVENT_t ev = event_poll(buffer, 4, EVENT_NONE);

            switch(ev)
            {
                case EVENT_TEXT_ENTERED:
                {
                    showInfo(1); // the train goes on...

                    if(keyPressed(KEY_ESC) || keyPressed(KEY_LCTRL) || keyPressed(KEY_RCTRL)) // To avoid conflicts with strg/esc shortcuts in kernel
                        break;

                    unsigned char text = buffer[0];
                    if (text > 0x20 && (entryLength<MAX_CHAR_PER_LINE || (insertMode && entryLength <= MAX_CHAR_PER_LINE && cursorPos != entryLength)))
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
                    showInfo(1); // the train goes on...
                    KEY_t key = *(KEY_t*)buffer;
                    switch(key)
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
                            printf("\n$> %s <--\n", entry);
                            textColor(0x0F);
                            printLine("$>                                                                              ", 40, 0x0B);
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
                                for (; entryLength > 0; entryLength--)
                                {
                                    putchar('\b'); //Clear row
                                }
                                ++curEntry;
                                entryLength = strlen(entryCache[curEntry]);
                                cursorPos = entryLength;
                            }
                            break;
                        case KEY_ARRD:
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
                            break;
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            } //switch
            drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
        } //while

EVALUATION: // evaluation of entry
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
            puts("Please enter the partition path (for example: 'A:0:'): ");
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
            puts("File is being searched... ");

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

            char** argv = malloc(sizeof(char*)*argc);
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

            argv[0] = formatPath(argv[0]);

            FS_ERROR error = execute(argv[0], argc, argv);
            switch(error)
            {
                case CE_GOOD:
                    puts(" Successfull.\n");
                    break;
                case CE_INVALID_FILENAME:
                    puts(" The path was malformed.\n");
                    break;
                case CE_FILE_NOT_FOUND:
                    puts(" File not found.\n");
                    break;
                default:
                    printf(" File load was not successful. Error Code: %u\n", error);
                    break;
            }

            free(argv[0]);
            free(argv);
        }
    } //while
    return 0;
}
