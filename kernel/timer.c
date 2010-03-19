#include "os.h"

//TASKSWITCH
#include "task.h"
extern uint32_t read_eip();
extern page_directory_t* current_directory;
extern task_t* current_task;
extern tss_entry_t tss_entry;

uint16_t systemfrequency = 1000; // system frequency
uint32_t timer_ticks = 0;
uint32_t eticks;

uint32_t getCurrentSeconds()
{
    // printformat("%d\n",timer_ticks/systemfrequency);
    return timer_ticks/systemfrequency;
}

uint32_t getCurrentMilliseconds()
{
    // printformat("%d\n",1000*timer_ticks/systemfrequency);
    return 1000*getCurrentSeconds();
}

void timer_handler(struct regs* r)
{
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
        printformat(".");
    settextcolor(15,0);
    #endif

    ++timer_ticks;
    if(eticks>0)
        --eticks;
}

void timer_wait (uint32_t ticks)
{
    eticks = ticks;

    // busy wait...
    while (eticks>0)
    {
        update_cursor();
    }
}

void sleepSeconds (uint32_t seconds)
{
    timer_wait((uint32_t)systemfrequency*seconds);
}

void sleepMilliSeconds (uint32_t ms)
{
    timer_wait((uint32_t)(systemfrequency*ms/1000));
}

void systemTimer_setFrequency( uint32_t freq )
{
    systemfrequency = freq;
    uint32_t divisor = 1193180 / systemfrequency; //divisor must fit into 16 bits
                                                  // PIT (programable interrupt timer)
    // Send the command byte
    // outportb(0x43, 0x36); // 0x36 -> Mode 3 : Square Wave Generator
       outportb(0x43, 0x34); // 0x34 -> Mode 2 : Rate Generator // thx to +gjm+

    // Send divisor
    outportb(0x40, (uint8_t)(  divisor     & 0xFF )); // low  byte
    outportb(0x40, (uint8_t)( (divisor>>8) & 0xFF )); // high byte
}

uint16_t systemTimer_getFrequency()
{
    return systemfrequency;
}

void timer_install()
{
    /* Installs 'timer_handler' to IRQ0 */
    irq_install_handler(32+0, timer_handler);
    systemTimer_setFrequency( systemfrequency ); // x Hz, meaning a tick every 1000/x milliseconds
}

void timer_uninstall()
{
    /* Uninstalls IRQ0 */
    irq_uninstall_handler(32+0);
}

///***********************************************************************///

// delay in microseconds independent of timer interrupt but on rdtsc
void delay (uint32_t microsec)
{
    uint64_t timeout = rdtsc() + (uint64_t)(microsec/1000*pODA->CPU_Frequency_kHz);

    while( rdtsc()<timeout )
    {
        //do nothing
    }
}

