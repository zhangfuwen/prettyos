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
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);

    
    iSetCursor(0, 7);

    for(;;)
    {
        switch(ev)
        {
            case EVENT_NONE:
            {
                waitForEvent(0);
                break;
            }
            case EVENT_UDP_RECEIVED:
            {
                udpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;
                printf("\npacket received. Length = %u\n:%s", header->length, data);
                break;
            }
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
                if(*key == KEY_ESC)
                {
                    return(0);
                }
                break;
            }
            default:
                break;
        }
        ev = event_poll(buffer, 4096, EVENT_NONE);
    }
        
    return(0);
}
