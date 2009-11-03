#include "userlib.h"

#define MAX_CHAR_PER_LINE 70

int main()
{
    for(;;)
    {
      settextcolor(15,0);
      puts("$> ");

      char entry[MAX_CHAR_PER_LINE+10];
      int i = 0;

      for(;;)
      {
        unsigned char input = getch();

        // if (
        //      (input==32)                ||  // space key
        //      (input==45)                ||  // "-"   key
        //      (input==63)                ||  // "?"   key
        //     ((input>=65)&&(input<=90))  ||  // capital letters
        //     ((input>=97)&&(input<=122)) ||  // little letters
        //     ((input>=48)&&(input<=57))      // 0 ... 9
        //     )

        //if( (input >= 0x20) && (input <= 0x7E) )
        if( (input >= 0x20) && (input <= 0xFF) ) // test-wise open, cf. ascii
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

      if( ( strcmp(entry,"help") == 0 ) || ( strcmp(entry,"?") == 0 ) )
      {
          settextcolor(2,0);
          puts("Implemented Instructions: help ? hi fdir\n");
          settextcolor(15,0);
      }

      else if( strcmp(entry,"hi") == 0)
      {
          settextcolor(2,0);
          puts("I am PrettyOS. Always at your service!\n");
          settextcolor(15,0);
      }

      else if( strcmp(entry,"fdir") == 0)
      {
          settextcolor(2,0);
          floppy_dir();
          settextcolor(15,0);
      }

      else
      {
          settextcolor(2,0);
          puts("Sorry, I do not know this command.\n");
          settextcolor(15,0);
      }
  }
  return 0;
}
