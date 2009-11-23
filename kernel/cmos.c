#include "cmos.h"

// Read byte from CMOS
uint8_t cmos_read(uint8_t off)
{
     uint8_t tmp = inportb(CMOS_ADDRESS);
     outportb( CMOS_ADDRESS, (tmp & 0x80) | (off & 0x7F) );
     return inportb(CMOS_DATA);
}

// Write byte to CMOS
void cmos_write(uint8_t off,uint8_t val)
{
    uint8_t tmp = inportb(CMOS_ADDRESS);
    outportb(CMOS_ADDRESS, (tmp & 0x80) | (off & 0x7F));
    outportb(CMOS_DATA,val);
}

void cmos_time()
{
    // sourcecode of "Cuervo" and "ehenkes", PrettyOS team

    uint8_t seconds    = PackedBCD2Decimal(cmos_read(0x00));
    uint8_t minutes    = PackedBCD2Decimal(cmos_read(0x02));
    uint8_t hour       = PackedBCD2Decimal(cmos_read(0x04));
    uint8_t weekday    = cmos_read(0x06) & 0xF             ;
    uint8_t dayofmonth = PackedBCD2Decimal(cmos_read(0x07));
    uint8_t month      = PackedBCD2Decimal(cmos_read(0x08));
    uint8_t year       = PackedBCD2Decimal(cmos_read(0x09));

    // weekday
    switch (weekday)
    {
        case 1:
            printformat("Sunday, ");
        break;
        case 2:
            printformat("Monday, ");
        break;
        case 3:
            printformat("Tuesday, ");
        break;
        case 4:
            printformat("Wednesday, ");
        break;
        case 5:
            printformat("Thursday, ");
        break;
        case 6:
            printformat("Friday, ");
        break;
        case 7:
            printformat("Saturday, ");
        break;
        default:
            printformat("Unknown Day: %d!",weekday);
    }

    // month
    switch (month)
    {
        case 1:
            printformat("January ");
        break;
        case 2:
            printformat("February ");
        break;
        case 3:
            printformat("March ");
        break;
        case 4:
            printformat("April ");
        break;
        case 5:
            printformat("May ");
        break;
        case 6:
            printformat("June ");
        break;
        case 7:
            printformat("July ");
        break;
        case 8:
            printformat("August ");
        break;
        case 9:
            printformat("September ");
        break;
        case 10:
            printformat("October ");
        break;
        case 11:
            printformat("November ");
        break;
        case 12:
            printformat("December ");
        break;
        default:
            printformat("Unknown month: %d",month);
    }

    // day
    if(dayofmonth<10)
    {
        printformat("0%d, ",dayofmonth);
    }
    else
    {
        printformat("%d, ",dayofmonth);
    }

    // year
    if(year>69)
    {
        printformat("19%d ",year);
    }
    else
    {
        if(year<10)
        {
            printformat("200%d ",year);
        }
        else
        {
            printformat("20%d ",year);
        }
    }

    // time
    if(hour<10)
    {
        printformat("0%d:",hour);
    }
    else
    {
        printformat("%d:",hour);
    }
    if(minutes<10)
    {
        printformat("0%d:",minutes);
    }
    else
    {
        printformat("%d:",minutes);
    }
    if(seconds<10)
    {
        printformat("0%d ",seconds);
    }
    else
    {
        printformat("%d ",seconds);
    }
}
