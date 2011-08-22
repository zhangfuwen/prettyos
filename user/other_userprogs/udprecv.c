#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"


int main()
{
    setScrollField(7, 46);
    printLine("================================================================================", 0, 0x0B);
    printLine("                     Pretty UDPreceive - Network test program!",                   2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);

    event_enable(true);
    const uint32_t BUFFERSIZE = 4000;
    char buffer[BUFFERSIZE];
    EVENT_t ev = event_poll(buffer, BUFFERSIZE, EVENT_NONE);

    udp_bind(8085);

    iSetCursor(0, 7);

    for (;;)
    {
        switch (ev)
        {
            case EVENT_NONE:
            {
                waitForEvent(0);
                break;
            }
            case EVENT_UDP_RECEIVED:
            {
                static uint32_t count=0;
                count++;
                udpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;
                textColor(0x0F);
                printf("\npacket received from %u.%u.%u.%u. Length = %u", header->srcIP.IP[0], header->srcIP.IP[1], header->srcIP.IP[2], header->srcIP.IP[3], header->length);
                printf("\n%u:\t",count);
                textColor(0x0A);
                printf("%s", data);
                textColor(0x0F);
                break;
            }
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
                if (*key == KEY_ESC)
                {
                    return(0);
                }
                break;
            }
            default:
                break;
        }
        ev = event_poll(buffer, BUFFERSIZE, EVENT_NONE);
    }

    return(0);
}
