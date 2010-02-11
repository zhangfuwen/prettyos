#include "userlib.h"

int main()
{
    char s[1000];
    unsigned long start; // Beginn der Berechnung bei 'start'
    unsigned long end;   // Ende der Berechnung bei 'end'
    unsigned long zahl;  // Element der Folge
    unsigned long i,j;   // Zählvariable für die Folgenlänge

    settextcolor(11,0);
    puts("--------------------------------------------------------------------\n");
    puts("Bitte geben Sie ihren Namen ein:\n");
    gets(s);
    puts("This piece of software was loaded by PrettyOS from the floppy disk of ");
    puts(s);
    putch('\n');
    puts("--------------------------------------------------------------------\n");
    puts("---                     Collatz-Folge 3n+1                       ---\n");
    puts("--------------------------------------------------------------------\n");

    /**************************** Eingabebereich ****************************/
    puts("Erste Zahl:   ");
    gets(s);
    start = atoi(s);
    puts("Letzte Zahl:  ");
    gets(s);
    end   = atoi(s);
    /**************************** Eingabebereich ****************************/

    for(j=start; j<end+1; j++)
    {
        zahl=j;
        i=1;
        putch('\n');
        itoa(j,s); // begin value
        puts(s);
        putch(' ');

        while(zahl != 1) // test on cycle 4-2-1
        {
            if( zahl % 2 == 0 )  zahl /=  2 ;           // Collatz
            else                 zahl = 3 * zahl + 1 ;  // Collatz

            itoa(zahl,s);
            puts(s); // element of the series
            putch(' ');
            i++ ;
        }

        if(zahl == 1)
        {
            puts("\tH(n): ");
            itoa(i,s);
            puts(s); // H(n) length of the series
            putch('\n');
        }
    }
    settextcolor(15,0);
    return 0;
}
