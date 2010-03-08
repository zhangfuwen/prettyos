#include "userlib.h"
#define MAX_CHAR_PER_LINE 70
#define ENTRY_CACHE_SIZE 10

void* memset(void* dest, char val, unsigned int count)
{
    char* temp = (char*)dest;
    for( ; count != 0; count--) *temp++ = val;
    return dest;
}

void eraseFirst(char* string)
{
    strcpy(string, string+1);
}
void insert(char* string, char c)
{
    for(int i = strlen(string)-1; i >= 0; i--) {
        string[i+1] = string[i];
    }
    *string = c;
}

static unsigned int cursorPos;
static unsigned int j;

void drawEntry(char* entry)
{
    putch('\r');
    puts("$> ");
    for(int i = 0; i <= j; i++) {
        if(i == cursorPos) {
            settextcolor(0, 15);
            putch(entry[cursorPos]);
            settextcolor(15, 0);
        }
        else {
            putch(entry[i]);
        }
    }
	if(cursorPos == j) {
		settextcolor(0, 15);
	    putch(' ');
		settextcolor(15, 0);
	    putch(' ');
	}
	else {
	    putch(' ');
	}
}

int main()
{
    char entry[MAX_CHAR_PER_LINE+1];
    char entryCache[ENTRY_CACHE_SIZE][MAX_CHAR_PER_LINE+1];
    int curEntry = -1;
	bool insertMode = false;
    unsigned char input;
    int numTasks;

    // to steer the train's velocity:
    const unsigned int waitingTime = 25;
    unsigned int start;

    ///TEST
    //puts(">>>STOP<<<\t");
    //for(;;);
    ///TEST

    //Init Cache
    for(unsigned int i = 0; i < ENTRY_CACHE_SIZE+1; i++) {
        memset(entryCache, 0, MAX_CHAR_PER_LINE+1);
    }

    while(true)
    {
      numTasks = getUserTaskNumber();

      #ifdef _DIAGNOSIS_
      static char s[10];
      settextcolor(2,0);
      itoa(numTasks,s);
      puts(s); putch(' ');
      settextcolor(15,0);
      #endif

      if(numTasks<=0) // no user tasks are running
      {
        settextcolor(15,0);
        j = 0; cursorPos = 0;
        memset(entry, 0, MAX_CHAR_PER_LINE+1);
        drawEntry(entry);
        start = getCurrentMilliseconds();

        while(true)
        {
          /* settextcolor(4,0); puts("."); settextcolor(15,0); */

          // the train goes on ////////////////////////////
          settextcolor(2,0);
          if( getCurrentMilliseconds() >= (start + waitingTime) )
          {
              showInfo(1);
              start = getCurrentMilliseconds();
          }
          settextcolor(15,0);
          /////////////////////////////////////////////////

          if ( testch() )
          {
            input = getch();

            switch(input) {
                case 8:   // Backspace
                    if(cursorPos>0)
                    {
                        if(curEntry != -1)
                        {
                            strcpy(entry, entryCache[curEntry]);
                            curEntry = -1;
                        }
                        eraseFirst(entry+cursorPos-1);
                        --j;
                        --cursorPos;
                        drawEntry(entry);
                    }
                    break;
                case 10:  // Enter
                    cursorPos = j+1;
                    drawEntry(entry);
                    puts("<--\n");
                    entry[j]='\0';
                    if(curEntry == -1) {
                        //Insert entry
                        for(int i = ENTRY_CACHE_SIZE-2; i >= 0; i--)
                        {
                            strncpy(entryCache[i+1], entryCache[i], MAX_CHAR_PER_LINE);
                        }
                        strcpy(entryCache[0], entry);
                    }
                    else {
                        //Move entry to front
                        strcpy(entry, entryCache[curEntry]);
                        for(int i = curEntry-1; i >= 0; i--)
                        {
                            strcpy(entryCache[i+1], entryCache[i]);
                        }
                        strcpy(entryCache[0], entry);
                        curEntry = -1;
                    }
                    goto EVALUATION;
                case 144: // Insert
                    insertMode = !insertMode;
                    break;
                case 145: // Delete
                    if(cursorPos < j) {
                        if(curEntry != -1)
                        {
                            strcpy(entry, entryCache[curEntry]);
                            curEntry = -1;
                        }
                        eraseFirst(entry+cursorPos);
                        --j;
                        drawEntry(entry);
                    }
                    break;
                case 146: // Pos 1
                    cursorPos = 0;
                    drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
                    break;
                case 147: // END
                    cursorPos = j;
                    drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
                    break;
                case 150: // Left Arrow
                    if(cursorPos > 0)
                    {
                        cursorPos--;
                        drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
                    }
                    break;
                case 151: // Up Arrow
                    if(curEntry < ENTRY_CACHE_SIZE-1 && *entryCache[curEntry+1] != 0) {
                        for(; j > 0; j--)
                        {
                            putch('\b'); //Clear row
                        }
                        ++curEntry;
                        puts(entryCache[curEntry]);
                        j = strlen(entryCache[curEntry]);
                        cursorPos = j;
                        drawEntry(entryCache[curEntry]);
                    }
                    break;
                case 152: // Down Arrow
                    if(curEntry >= 0) {
                        for(; j > 0; j--)
                        {
                            putch('\b'); //Clear row
                        }
                        --curEntry;
                        if(curEntry == -1)
                        {
                            puts(entry);
                            j = strlen(entry);
                            cursorPos = j;
                        }
                        else
                        {
                            puts(entryCache[curEntry]);
                            j = strlen(entryCache[curEntry]);
                            cursorPos = j;
                        }
                    }
                    if(curEntry == -1)
                    {
                        for(; j > 0; j--)
                        {
                            putch('\b'); //Clear row
                        }
                        memset(entry, 0, MAX_CHAR_PER_LINE);
                        cursorPos = 0;
                    }
                    drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
                    break;
                case 153: // Right Arrow
                    if(cursorPos < j)
                    {
                        cursorPos++;
                        drawEntry((curEntry == -1 ? entry : entryCache[curEntry]));
                    }
                    break;
                default:
                    if(input >= 0x20 && (j<MAX_CHAR_PER_LINE || (insertMode && j <=MAX_CHAR_PER_LINE && cursorPos != j)) /*&& (input <= 0xFF)*/ ) // test-wise open, cf. ascii
                    {
                        if(curEntry != -1)
                        {
                            strcpy(entry, entryCache[curEntry]);
                            curEntry = -1;
                        }
                        if(insertMode)
                        {
                            entry[cursorPos]=input;
                            if(cursorPos == j) {
                                j++;
                            }
                        }
                        else
                        {
                            insert(entry+cursorPos, input);
                            ++j;
                        }
                        ++cursorPos;
                        drawEntry(entry);
                    }
                    break;
            }//switch
          }//if
        }//while

EVALUATION:
        // evaluation of entry
        {
          settextcolor(2,0);
          if( ( strcmp(entry,"help") == 0 ) || ( strcmp(entry,"?") == 0 ) )
          {
              puts("Implemented Instructions: hi  help ?  fdir  fformat\n");
          }
          else if( strcmp(entry,"hi") == 0 )
          {
              puts("I am PrettyOS. Always at your service!\n");
          }
          else if( strcmp(entry,"fdir") == 0 )
          {
              if(floppy_dir())
              {
                  floppy_dir();
              }
          }
          else if( strcmp(entry,"fformat") == 0 )
          {
              floppy_format("PrettyOS");
              if(floppy_dir())
              {
                  floppy_dir();
              }
          }
          else if( strcmp(entry,"") == 0 )
          {
              puts("<-- Sorry, this was a blank entry.\n");
          }
          else
          {
			  settextcolor(15,0);
              puts("file is being searched.\n");
              settextcolor(2,0);
              toupper(entry);

              int posPoint = 8;
              char name[9];
              char ext[4];
              name[8]='\0';
              ext[3] ='\0';

              for(int i=0;i<8;i++)
              {
                  if(entry[i]=='.')
                  {
                      posPoint=i; // search point position
                  }
              }

              for(int i=0; i<8; i++)
              {
                  if( (i<posPoint) && (isalnum(entry[i])) )
                  {
                      name[i]=entry[i];
                  }
                  else
                  {
                      name[i]=0x20; // space
                  }
              }

              for(int i=posPoint+1; i<posPoint+4; i++)
              {
                  ext[i-posPoint-1]=entry[i];
              }

              if( (ext[0]==0) && (ext[1]==0) && (ext[2]==0) )
              {
                  ext[0] = 'E';
                  ext[1] = 'L';
                  ext[2] = 'F';
              }

              if(floppy_load(name,ext))
              {
                   puts("<-- Sorry, PrettyOS does not know this command.\n");
              }
          }//else
          settextcolor(15,0);
        }//evaluation
      }//if
    }//while
    return 0;
}
