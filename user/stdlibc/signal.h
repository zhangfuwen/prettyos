#ifndef SIGNAL_H
#define SIGNAL_H

enum {
    SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM
};

extern void (*SIG_DFL)(int);
extern void (*SIG_IGN)(int);

typedef int sig_atomic_t;

//extern void (*signal(int sig, void (*func)(int)))(int);


#ifdef _cplusplus
extern "C" {
#endif

//int raise(signal sig);

#ifdef _cplusplus
}
#endif

#endif
