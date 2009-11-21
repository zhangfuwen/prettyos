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
    // sourcecode of "Cuervo", PrettyOS team

    uint8_t hour       = (cmos_read(0x04));
    uint8_t minutes    = (cmos_read(0x02));
    uint8_t seconds    = (cmos_read(0x00));
    uint8_t dayofmonth = (cmos_read(0x07));
    uint8_t weekday    = (cmos_read(0x06));
    uint8_t month      = (cmos_read(0x08));
    uint8_t year       = (cmos_read(0x09));

    // day
    switch (weekday&0xF)
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
            printformat("Unknown Day: %d!",weekday&0xF);
    }

    // month
    switch (month&0xF)
    {
        case 1:
            if ((month>>4) == 0)
            {
                printformat("January ");
            }
            else
            {
                printformat("November ");
            }
        break;
        case 2:
            if ((month>>4) == 0)
            {
                printformat("February ");
            }
            else
            {
                printformat("December ");
            }
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
        default:
            printformat("Unknown month: %d %d",month>>4,month&0xF);
    }

    //day
    printformat("%d%d, ",dayofmonth>>4,dayofmonth&0xF);

    // year
    if((year>>4)>6)
    {
        printformat("19%d%d",year>>4,year&0xF);
    }
    else
    {
        printformat("20%d%d ",year>>4,year&0xF);
    }

    // time
    printformat("%d%d:",hour>>4,hour&0xF);
    printformat("%d%d:",minutes>>4,minutes&0xF);
    printformat("%d%d ",seconds>>4,seconds&0xF);
}
