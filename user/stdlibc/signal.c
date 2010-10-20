#include "signal.h"

static void dummy(int i) {}

void (*SIG_DFL)(int) = (void*)1; // HACK, but maybe best way to solve the problem of having different standard functions for different signals
void (*SIG_IGN)(int) = &dummy;


//void (*signal(int sig, void (*func)(int)))(int);

//int raise(signal sig); /// TODO
