#include "syscall.h"
#include "task.h"
#include "fat12.h"
#include "file.h"
#include "sys_speaker.h"

// some functions declared extern in os.h
// rest of functions must be declared here:
// ...

DEFN_SYSCALL1( puts,                       0, const char*              )
DEFN_SYSCALL1( putch,                      1, char                     )
DEFN_SYSCALL2( settextcolor,               2, uint8_t, uint8_t         )
DEFN_SYSCALL0( getpid,                     3                           )
DEFN_SYSCALL0( nop,                        4                           )
DEFN_SYSCALL0( switch_context,             5                           )
DEFN_SYSCALL0( checkKQ_and_return_char,    6                           )
DEFN_SYSCALL0( flpydsk_read_directory,     7                           )
DEFN_SYSCALL3( printf,                     8, char*, uint32_t, uint8_t )
DEFN_SYSCALL0( getCurrentSeconds,          9                           )
DEFN_SYSCALL0( getCurrentMilliseconds,    10                           )
DEFN_SYSCALL1( flpydsk_format,            11, char*                    )
DEFN_SYSCALL2( flpydsk_load,              12, const char*, const char* )
DEFN_SYSCALL0( exit,                      13                           )
DEFN_SYSCALL1( settaskflag,               14, int32_t                  )
DEFN_SYSCALL2( beep,                      15, uint32_t, uint32_t       )
DEFN_SYSCALL0( getUserTaskNumber,         16                           )
DEFN_SYSCALL0( testch,                    17                           )
DEFN_SYSCALL1( clear_userscreen,          18, uint8_t                  )
DEFN_SYSCALL2( set_cursor,                19, uint8_t, uint8_t         )
DEFN_SYSCALL1( grow_heap,                 20, uint32_t                 )

static void* syscalls[] =
{
    &puts,
    &putch,
    &settextcolor,
    &getpid,
    &nop,
    &switch_context,
    &checkKQ_and_return_char,
    &flpydsk_read_directory,
    &printf,
    &getCurrentSeconds,
    &getCurrentMilliseconds,
    &flpydsk_format,
    &flpydsk_load,
    &exit,
    &settaskflag,
    &beep,
    &getUserTaskNumber,
    &testch,
    &clear_userscreen,
    &set_cursor,
    &task_grow_userheap
};

void syscall_handler(struct regs* r)
{
    // Firstly, check if the requested syscall number is valid. The syscall number is found in EAX.
    if( r->eax >= sizeof(syscalls)/sizeof(*syscalls) )
        return;

    void* addr = syscalls[r->eax]; // Get the required syscall location.

    // We don't know how many parameters the function wants, so we just push them all onto the stack in the correct order.
    // The function will use all the parameters it wants, and we can pop them all back off afterwards.
    int32_t ret;
    __asm__ volatile (" \
      push %1; \
      push %2; \
      push %3; \
      push %4; \
      push %5; \
      call *%6; \
      add $20, %%esp; \
    " : "=a" (ret) : "r" (r->edi), "r" (r->esi), "r" (r->edx), "r" (r->ecx), "r" (r->ebx), "r" (addr));
    r->eax = ret;
}


void syscall_install()
{
    irq_install_handler( 127, syscall_handler );
}
