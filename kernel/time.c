/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/


#include "cdi_cmos.h"
#include "time.h"

tm_t currentTime;

// info from http://lowlevel.brainsware.org/wiki/index.php/CMOS
/*

/// do not use direct call to the kernel function, but the CDI wrapper
tm_t* cmosTime(tm_t* ptm)
{
    ptm->second     = PackedBCD2Decimal(cmos_read(0x00));
    ptm->minute     = PackedBCD2Decimal(cmos_read(0x02));
    ptm->hour       = PackedBCD2Decimal(cmos_read(0x04));
    ptm->dayofmonth = PackedBCD2Decimal(cmos_read(0x07));
    ptm->month      = PackedBCD2Decimal(cmos_read(0x08));
    ptm->year       = PackedBCD2Decimal(cmos_read(0x09));
    ptm->century    = PackedBCD2Decimal(cmos_read(0x32));
    return ptm;
}
*/

/// CDI wrapper used
tm_t* cmosTime(tm_t* ptm)
{
    ptm->second     = PackedBCD2Decimal(cdi_cmos_read(0x00));
    ptm->minute     = PackedBCD2Decimal(cdi_cmos_read(0x02));
    ptm->hour       = PackedBCD2Decimal(cdi_cmos_read(0x04));
    ptm->dayofmonth = PackedBCD2Decimal(cdi_cmos_read(0x07));
    ptm->month      = PackedBCD2Decimal(cdi_cmos_read(0x08));
    ptm->year       = PackedBCD2Decimal(cdi_cmos_read(0x09));
    ptm->century    = PackedBCD2Decimal(cdi_cmos_read(0x32));
    return ptm;
}

static void appendInt(int val, char* dest, char* buf) {
    if(val<10)
    {
        strcat(dest,"0");
    }
    itoa(val, buf);
	strcat(dest, buf);
}

unsigned int calculateWeekday(unsigned int year, unsigned int month, unsigned int day) {
	day += 6; //1.1.2000 was Saturday
	day += (year-2000)*365.25;
	if(month > 11)
		day+=334;
	else if(month > 10)
		day+=304;
	else if(month > 9)
		day+=273;
	else if(month > 8)
		day+=243;
	else if(month > 7)
		day+=212;
	else if(month > 6)
		day+=181;
	else if(month > 5)
		day+=151;
	else if(month > 4)
		day+=120;
	else if(month > 3)
		day+=90;
	else if(month > 2)
		day+=59;
	else if(month > 1)
		day+=31;

	if(year%4 == 0 && (month < 2 || (month == 2 && day <= 28))) {
		day--;
	}

	return(day%7+1);
}

char* getCurrentDateAndTime(char* pStr)
{
    pStr[0]='\0'; // clear string

    // sourcecode of "Cuervo", "ehenkes" and "Mr X", PrettyOS team
    tm_t* pct = cmosTime(&currentTime);
    char buf[40];

	pct->weekday = calculateWeekday(2000+pct->year, pct->month, pct->dayofmonth);
    switch (pct->weekday)
    {
        case 1: strcpy(pStr, "Sunday, ");    break;
        case 2: strcpy(pStr, "Monday, ");    break;
        case 3: strcpy(pStr, "Tuesday, ");   break;
        case 4: strcpy(pStr, "Wednesday, "); break;
        case 5: strcpy(pStr, "Thursday, ");  break;
        case 6: strcpy(pStr, "Friday, ");    break;
        case 7: strcpy(pStr, "Saturday, ");  break;
    }

    switch (pct->month)
    {
        case 1:  strcat(pStr, "January ");   break;
        case 2:  strcat(pStr, "February ");  break;
        case 3:  strcat(pStr, "March ");     break;
        case 4:  strcat(pStr, "April ");     break;
        case 5:  strcat(pStr, "May ");       break;
        case 6:  strcat(pStr, "June ");      break;
        case 7:  strcat(pStr, "July ");      break;
        case 8:  strcat(pStr, "August ");    break;
        case 9:  strcat(pStr, "September "); break;
        case 10: strcat(pStr, "October ");   break;
        case 11: strcat(pStr, "November ");  break;
        case 12: strcat(pStr, "December ");  break;
    }

	appendInt(pct->dayofmonth, pStr, buf);

    strcat(pStr,", ");

    itoa(pct->century, buf);
    strcat(pStr, buf);

	appendInt(pct->year, pStr, buf);

    strcat(pStr,", ");

	appendInt(pct->hour, pStr, buf);

    strcat(pStr,":");

	appendInt(pct->minute, pStr, buf);

    strcat(pStr,":");

	appendInt(pct->second, pStr, buf);

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
