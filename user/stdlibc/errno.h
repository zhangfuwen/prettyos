#ifndef ERRNO_H
#define ERRNO_H

#define EDOM 1
#define ERANGE 2

int* _errno();
#define errno (*_errno())

#endif
