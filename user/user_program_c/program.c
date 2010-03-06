#include "userlib.h"
#define MAX_CHAR_PER_LINE 70
#define ENTRY_CACHE_SIZE 10

void* memset(void* dest, char val, unsigned int count)
{
    char* temp = (char*)dest;
    for( ; count != 0; count--) *temp++ = val;
    return dest;
}

int main()
{
    unsigned int j, cursorPos;
    char entry[MAX_CHAR_PER_LINE+1];
    char entryCache[ENTRY_CACHE_SIZE][MAX_CHAR_PER_LINE+1];
    int curEntry = -1;
    int retVal;
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
    for(unsigned int i = 0; i < ENTRY_CACHE_SIZE; i++) {
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
        puts("$> ");
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
                    if(curEntry != -1) {
                        curEntry = -1;
                        strcpy(entry, entryCache[curEntry]);
                    }
                    if(j>0)
                    {
                        putch('\b');
                        entry[j-1]='\0';
                        --j;
                    }
                    break;
                case 10:  // Line Feed (ENTER)
                    puts(" <--\n");
                    entry[j]='\0';
                    ++j;
                    if(curEntry == -1) {
                        //Insert entry
                        for(int i = ENTRY_CACHE_SIZE-2; i >= 0; i--) {
                            strcpy(entryCache[i+1], entryCache[i]);
                        }
                        strcpy(entryCache[0], entry);
                    }
                    else {
                        //Move entry to front
                        strcpy(entry, entryCache[curEntry]);
                        for(int i = curEntry-1; i >= 0; i--) {
                            strcpy(entryCache[i+1], entryCache[i]);
                        }
                        strcpy(entryCache[0], entry);
                        curEntry = -1;
                    }
                    goto EVALUATION;
                case 144: // Insert; To be implemented later
                    break;
                case 145: // Delete; To be implemented later
                    if(curEntry != -1) {
                        curEntry = -1;
                        strcpy(entry, entryCache[curEntry]);
                    }
                    break;
                case 146: // POS1; To be implemented later
                    break;
                case 147: // END; To be implemented later
                    break;
                case 150: // Left Arrow; To be implemented later
                    if(cursorPos > 0) {
                        cursorPos--;
                    }
                    break;
                case 151: // Up Arrow
                    if(curEntry < ENTRY_CACHE_SIZE-1 && *entryCache[curEntry+1] != 0) {
                        for(; j > 0; j--) {
                            putch('\b'); //Clear row
                        }
                        ++curEntry;
                        puts(entryCache[curEntry]);
                        j = strlen(entryCache[curEntry]);
                    }
                    break;
                case 152: // Down Arrow
                    if(curEntry >= 0) {
                        for(; j > 0; j--) {
                            putch('\b'); //Clear row
                        }
                        --curEntry;
                        if(curEntry == -1) {
                            puts(entry);
                            j = strlen(entry);
                        }
                        else {
                            puts(entryCache[curEntry]);
                            j = strlen(entryCache[curEntry]);
                        }
                    }
                    if(curEntry == -1) {
                        for(; j > 0; j--) {
                            putch('\b'); //Clear row
                        }
                        memset(entry, 0, MAX_CHAR_PER_LINE);
                    }
                    break;
                case 153: // Right Arrow; To be implemented later
                    if(cursorPos < j) {
                        cursorPos++;
                    }
                    break;
                default:
                    if(input >= 0x20 && j<MAX_CHAR_PER_LINE /*&& (input <= 0xFF)*/ ) // test-wise open, cf. ascii
                    {
                        if(curEntry != -1) {
                            curEntry = -1;
                            strcpy(entry, entryCache[curEntry]);
                        }
                        putch(input);
                        entry[j]=input;
                        ++j;
                    }
                    break;
            }//switch
          }//if
        }//while

EVALUATION:
        // evaluation of entry
        {
          if( ( strcmp(entry,"help") == 0 ) || ( strcmp(entry,"?") == 0 ) )
          {
              settextcolor(2,0);
              puts("Implemented Instructions: hi  help ?  fdir  fformat\n");
              settextcolor(15,0);
          }
          else if( strcmp(entry,"hi") == 0 )
          {
              settextcolor(2,0);
              puts("I am PrettyOS. Always at your service!\n");
              settextcolor(15,0);
          }
          else if( strcmp(entry,"fdir") == 0 )
          {
              settextcolor(2,0);
              retVal = floppy_dir();
              if(retVal!=0)
              {
                  floppy_dir();
              }
              settextcolor(15,0);
          }
          else if( strcmp(entry,"fformat") == 0 )
          {
              settextcolor(2,0);
              retVal = floppy_format("PrettyOS");
              retVal = floppy_dir();
              if(retVal!=0)
              {
                  floppy_dir();
              }
              settextcolor(15,0);
          }
          else if( strcmp(entry,"") == 0 )
          {
              settextcolor(2,0);
              puts("<-- Sorry, this was a blank entry.\n");
              settextcolor(15,0);
          }
          else
          {
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

              retVal = floppy_load(name,ext);
              if(retVal!=0)
              {
                   puts("<-- Sorry, PrettyOS does not know this command.\n");
              }
              settextcolor(15,0);
          }//else
        }//evaluation
      }//if
    }//while
    return 0;
}
