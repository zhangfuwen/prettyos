#include "userlib.h"
#define MAX_CHAR_PER_LINE 70

static void clearEntry(char* entry, int* j)
{
    for((*j)=0;(*j)<MAX_CHAR_PER_LINE;(*j)++)
    {
        entry[(*j)]=0;
    }
}

int main()
{
    int i, n;
    char entry[MAX_CHAR_PER_LINE+10];
    int retVal;
    char name[9];
    char ext[4];
    int posPoint;
    unsigned char input;
    int numTasks;

    // to steer the train's velocity:
    const unsigned int waitingTime = 25;
    unsigned int start;

    ///TEST
    //puts(">>>STOP<<<\t");
    //for(;;);
    ///TEST

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
        i=0;
        clearEntry(entry,&n);
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

            if( (input >= 0x20) /*&& (input <= 0xFF)*/ ) // test-wise open, cf. ascii
            {
              if(i<MAX_CHAR_PER_LINE) //
              {
                  putch(input);
                  entry[i]=input;
                  ++i;
              }
            }
            if( input==8 )                      // Backspace
            {
              if(i>0)
              {
                  putch('\b');
                  entry[i-1]='\0';
                  --i;
              }
            }

            if( input==10 )                     // Line Feed (ENTER-Key)
            {
              puts(" <--");
              putch('\n');
              entry[i]='\0';
              ++i;
              break;
            }
          }
        }//while

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

              posPoint=8;

              name[8]='\0';
              ext[3] ='\0';

              for(i=0;i<8;i++)
              {
                  if(entry[i]=='.')
                  {
                      posPoint=i; // search point position
                  }
              }

              for(i=0;i<8;i++)
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

              //for(i=posPoint+1;i<12;i++)
              for(i=posPoint+1;i<posPoint+4;i++)///TEST
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
