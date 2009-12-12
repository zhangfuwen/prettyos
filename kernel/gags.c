/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

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
            printf(line1,46,0xE);
            printf(line2,47,0xE);
            printf(line3,48,0xE);
        break;

        default:
        break;
    }
}

/*
* Copyright (c) 2009 The PrettyOS Project. All rights reserved.
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

