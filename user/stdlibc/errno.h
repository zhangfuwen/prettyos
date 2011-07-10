#ifndef ERRNO_H
#define ERRNO_H

#define ERRDOM 1
#define ERRRANGE 2

int* _errno();
#define errno (*_errno())

#endif
