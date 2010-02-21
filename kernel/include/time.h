#ifndef TIME_H
#define TIME_H

#include "os.h"
#include "cmos.h"

struct tm
{
    int8_t second;      // seconds 0-60 (60, because there can be a leap second
    int8_t minute;      // minutes 0-59
    int8_t hour;        // hours 0-23
    int8_t weekday;     // weekday 1-7 (1 = sunday)
    int8_t dayofmonth;  // day 1-31
    int8_t month;       // month 1-12
    int8_t year;        // year (2 digits)
    int8_t century;     // century (2 digits)
} __attribute__((packed));

typedef struct tm tm_t;

tm_t* cmosTime(tm_t* ptm);
char* getCurrentDateAndTime(char* pStr);

#endif
