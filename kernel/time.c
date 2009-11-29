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
            k_strcpy(pStr, "Sunday, ");
        break;
        case 2:
            k_strcpy(pStr, "Monday, ");
        break;
        case 3:
            k_strcpy(pStr, "Tuesday, ");
        break;
        case 4:
            k_strcpy(pStr, "Wednesday, ");
        break;
        case 5:
            k_strcpy(pStr, "Thursday, ");
        break;
        case 6:
            k_strcpy(pStr, "Friday, ");
        break;
        case 7:
            k_strcpy(pStr, "Saturday, ");
        break;
    }

    // month
    switch (pct->month)
    {
        case 1:
            k_strcat(pStr, "January ");
        break;
        case 2:
            k_strcat(pStr, "February ");
        break;
        case 3:
            k_strcat(pStr, "March ");
        break;
        case 4:
            k_strcat(pStr, "April ");
        break;
        case 5:
            k_strcat(pStr, "May ");
        break;
        case 6:
            k_strcat(pStr, "June ");
        break;
        case 7:
            k_strcat(pStr, "July ");
        break;
        case 8:
            k_strcat(pStr, "August ");
        break;
        case 9:
            k_strcat(pStr, "September ");
        break;
        case 10:
            k_strcat(pStr, "October ");
        break;
        case 11:
            k_strcat(pStr, "November ");
        break;
        case 12:
            k_strcat(pStr, "December ");
        break;
    }

    // day
    if(pct->dayofmonth<10)
    {
        k_strcat(pStr,"0");
        k_itoa(pct->dayofmonth, buf);
        k_strcat(pStr, buf);
    }
    else
    {
        k_itoa(pct->dayofmonth, buf);
        k_strcat(pStr, buf);
    }

    k_strcat(pStr,", ");

    // century
    k_itoa(pct->century, buf);
    k_strcat(pStr, buf);

    // year
    if(pct->year<10)
    {
        k_strcat(pStr,"0");
        k_itoa(pct->year, buf);
        k_strcat(pStr, buf);
    }
    else
    {
        k_itoa(pct->year, buf);
        k_strcat(pStr, buf);
    }

    k_strcat(pStr,", ");

    // time
    if(pct->hour<10)
    {
        k_strcat(pStr,"0");
        k_itoa(pct->hour, buf);
        k_strcat(pStr, buf);
    }
    else
    {
        k_itoa(pct->hour, buf);
        k_strcat(pStr, buf);
    }

    k_strcat(pStr,":");

    if(pct->minute<10)
    {
        k_strcat(pStr,"0");
        k_itoa(pct->minute, buf);
        k_strcat(pStr, buf);
    }
    else
    {
        k_itoa(pct->minute, buf);
        k_strcat(pStr, buf);
    }

    k_strcat(pStr,":");

    if(pct->second<10)
    {
        k_strcat(pStr,"0");
        k_itoa(pct->second, buf);
        k_strcat(pStr, buf);
    }
    else
    {
        k_itoa(pct->second, buf);
        k_strcat(pStr, buf);
    }

    k_strcat(pStr, ""); // add '\0'

    return pStr;
}

