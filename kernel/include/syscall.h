#ifndef SYSCALL_H
#define SYSCALL_H

#include "os.h"

void syscall_install();
void syscall_handler(struct regs* regs);

#define DECL_SYSCALL0(fn)                int syscall_##fn();
#define DECL_SYSCALL1(fn,p1)             int syscall_##fn(p1);
#define DECL_SYSCALL2(fn,p1,p2)          int syscall_##fn(p1,p2);
#define DECL_SYSCALL3(fn,p1,p2,p3)       int syscall_##fn(p1,p2,p3);
#define DECL_SYSCALL4(fn,p1,p2,p3,p4)    int syscall_##fn(p1,p2,p3,p4);
#define DECL_SYSCALL5(fn,p1,p2,p3,p4,p5) int syscall_##fn(p1,p2,p3,p4,p5);

#define DEFN_SYSCALL0(fn, num) \
int syscall_##fn() \
{ \
  int a; \
  __asm__ volatile("int $0x7F" : "=a" (a) : "0" (num)); \
  return a; \
}

#define DEFN_SYSCALL1(fn, num, P1) \
int syscall_##fn(P1 p1) \
{ \
  int a; \
  __asm__ volatile("int $0x7F" : "=a" (a) : "0" (num), "b" ((int)p1)); \
  return a; \
}

#define DEFN_SYSCALL2(fn, num, P1, P2) \
int syscall_##fn(P1 p1, P2 p2) \
{ \
  int a; \
  __asm__ volatile("int $0x7F" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2)); \
  return a; \
}

#define DEFN_SYSCALL3(fn, num, P1, P2, P3) \
int syscall_##fn(P1 p1, P2 p2, P3 p3) \
{ \
  int a; \
  __asm__ volatile("int $0x7F" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d"((int)p3)); \
  return a; \
}

#define DEFN_SYSCALL4(fn, num, P1, P2, P3, P4) \
int syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4) \
{ \
  int a; \
  __asm__ volatile("int $0x7F" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4)); \
  return a; \
}

#define DEFN_SYSCALL5(fn, num) \
int syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) \
{ \
  int a; \
  __asm__ volatile("int $0x7F" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4), "D" ((int)p5)); \
  return a; \
}

/******* declaration of syscalls in syscall.h **************/

DECL_SYSCALL1( puts,  char*                               )
DECL_SYSCALL1( putch, char                                )
DECL_SYSCALL2( settextcolor, uint8_t, uint8_t             )
DECL_SYSCALL0( getpid                                     )
DECL_SYSCALL0( nop                                        )
DECL_SYSCALL0( switch_context                             )
DECL_SYSCALL0( checkKQ_and_return_char                    )
DECL_SYSCALL0( flpydsk_read_directory                     )
DECL_SYSCALL3( printf, char*, uint32_t, uint8_t           )
DECL_SYSCALL0( getCurrentSeconds                          )
DECL_SYSCALL0( getCurrentMilliseconds                     )
DECL_SYSCALL1( flpydsk_format, char*                      )
DECL_SYSCALL2( flpydsk_load, char*, char*                 )
DECL_SYSCALL0( exit                                       )
DECL_SYSCALL1( settaskflag, int                           )
DECL_SYSCALL2( beep, uint32_t, uint32_t                   )
/***********************************************************/

#endif
