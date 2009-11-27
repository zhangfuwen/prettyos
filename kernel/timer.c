#include "os.h"

//TASKSWITCH
#include "task.h"
extern uint32_t read_eip();
extern page_directory_t* current_directory;
extern task_t* current_task;
extern tss_entry_t tss_entry;

uint16_t systemfrequency = 100; // system frequency
uint32_t timer_ticks = 0;
uint32_t eticks;

uint32_t getCurrentSeconds()
{
    return timer_ticks/systemfrequency;
}

void timer_handler(struct regs* r)
{
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

    // Send the command byte
    outportb(0x43, 0x36);

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

