#ifndef STDDEF_H
#define STDDEF_H

typedef unsigned int   size_t;
typedef int            ptrdiff_t;

#ifndef _cplusplus
typedef __WCHAR_TYPE__ wchar_t;
#endif

#define NULL (void*)0

#define offsetof(s,m)  (size_t)&(((s *)0)->m)

#endif
