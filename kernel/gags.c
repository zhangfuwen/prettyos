#include "os.h"
#include "gags.h"

// Zug als Beispiel übernommen aus
// http://de.wikipedia.org/wiki/ASCII-Art#Allgemeine_Beispiele
// TODO: eigene ASCII-Art erzeugen

char* line1 = "  _______                _______      <>_<>                                     "    ;
char* line2 = " (_______) |_|_|_|_|_|_|| [] [] | .---|'\"`|---.                                 "   ;
char* line3 = "`-oo---oo-'`-oo-----oo-'`-o---o-'`o\"O-OO-OO-O\"o'                                "  ;

void showTrain(int8_t val)
{
    int i;
    char temp1,temp2,temp3;
    switch(val)
    {
        case 1:
            temp1 = line1[79];
            temp2 = line2[79];
            temp3 = line3[79];

            for(i=79;i>0;--i)
            {
                line1[i] = line1[i-1];
                line2[i] = line2[i-1];
                line3[i] = line3[i-1];
            }
            line1[0] = temp1;
            line2[0] = temp2;
            line3[0] = temp3;
            k_printf(line1,46,0xE);
            k_printf(line2,47,0xE);
            k_printf(line3,48,0xE);
        break;

        default:
        break;
    }
}


