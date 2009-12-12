/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/


#include "cmos.h"
#include "time.h"

tm_t currentTime;

// info from http://lowlevel.brainsware.org/wiki/index.php/CMOS
tm_t* cmosTime(tm_t* ptm)
{
    ptm->second     = PackedBCD2Decimal(cmos_read(0x00));
    ptm->minute     = PackedBCD2Decimal(cmos_read(0x02));
    ptm->hour       = PackedBCD2Decimal(cmos_read(0x04));
    ptm->weekday    = cmos_read(0x06) & 0xF             ;
    ptm->dayofmonth = PackedBCD2Decimal(cmos_read(0x07));
    ptm->month      = PackedBCD2Decimal(cmos_read(0x08));
    ptm->year       = PackedBCD2Decimal(cmos_read(0x09));
    ptm->century    = PackedBCD2Decimal(cmos_read(0x32));
    return ptm;
}

char* getCurrentDateAndTime(char* pStr)
{
    // sourcecode of "Cuervo" and "ehenkes", PrettyOS team
    tm_t* pct = cmosTime(&currentTime);
    char buf[40];

    // weekday
    switch (pct->weekday)
    {
        case 1:
            strcpy(pStr, "Sunday, ");
        break;
        case 2:
            strcpy(pStr, "Monday, ");
        break;
        case 3:
            strcpy(pStr, "Tuesday, ");
        break;
        case 4:
            strcpy(pStr, "Wednesday, ");
        break;
        case 5:
            strcpy(pStr, "Thursday, ");
        break;
        case 6:
            strcpy(pStr, "Friday, ");
        break;
        case 7:
            strcpy(pStr, "Saturday, ");
        break;
    }

    // month
    switch (pct->month)
    {
        case 1:
            strcat(pStr, "January ");
        break;
        case 2:
            strcat(pStr, "February ");
        break;
        case 3:
            strcat(pStr, "March ");
        break;
        case 4:
            strcat(pStr, "April ");
        break;
        case 5:
            strcat(pStr, "May ");
        break;
        case 6:
            strcat(pStr, "June ");
        break;
        case 7:
            strcat(pStr, "July ");
        break;
        case 8:
            strcat(pStr, "August ");
        break;
        case 9:
            strcat(pStr, "September ");
        break;
        case 10:
            strcat(pStr, "October ");
        break;
        case 11:
            strcat(pStr, "November ");
        break;
        case 12:
            strcat(pStr, "December ");
        break;
    }

    // day
    if(pct->dayofmonth<10)
    {
        strcat(pStr,"0");
        itoa(pct->dayofmonth, buf);
        strcat(pStr, buf);
    }
    else
    {
        itoa(pct->dayofmonth, buf);
        strcat(pStr, buf);
    }

    strcat(pStr,", ");

    // century
    itoa(pct->century, buf);
    strcat(pStr, buf);

    // year
    if(pct->year<10)
    {
        strcat(pStr,"0");
        itoa(pct->year, buf);
        strcat(pStr, buf);
    }
    else
    {
        itoa(pct->year, buf);
        strcat(pStr, buf);
    }

    strcat(pStr,", ");

    // time
    if(pct->hour<10)
    {
        strcat(pStr,"0");
        itoa(pct->hour, buf);
        strcat(pStr, buf);
    }
    else
    {
        itoa(pct->hour, buf);
        strcat(pStr, buf);
    }

    strcat(pStr,":");

    if(pct->minute<10)
    {
        strcat(pStr,"0");
        itoa(pct->minute, buf);
        strcat(pStr, buf);
    }
    else
    {
        itoa(pct->minute, buf);
        strcat(pStr, buf);
    }

    strcat(pStr,":");

    if(pct->second<10)
    {
        strcat(pStr,"0");
        itoa(pct->second, buf);
        strcat(pStr, buf);
    }
    else
    {
        itoa(pct->second, buf);
        strcat(pStr, buf);
    }

    strcat(pStr, ""); // add '\0'

    return pStr;
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
