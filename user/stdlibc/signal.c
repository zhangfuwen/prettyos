#include "signal.h"
#include "stdlib.h"

static void dummy(int i) {}

void (*SIG_DFL)(int) = (void*)1; // HACK, but maybe best way to solve the problem of having different standard functions for different signals
void (*SIG_IGN)(int) = &dummy;

void (*signals[_SIGNUM])(int) = {&exit, &dummy, &dummy, &dummy, &dummy, &exit};
void (*signal_defaults[_SIGNUM])(int) = {&exit, &dummy, &dummy, &dummy, &dummy, &exit};


void (*signal(int sig, void (*func)(int)))(int)
{
    if(func == SIG_DFL)
        signals[sig] = signal_defaults[sig];
    else
        signals[sig] = func;
    return(signals[sig]);
}

int raise(int sig)
{
    if(sig < _SIGNUM) {
        signals[sig](sig);
        return(0);
    }
    return(-1);
}
