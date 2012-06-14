#pragma once
#define __attribute__((a))
#define restrict

#ifndef __clang__
#	define __GNUC__ 4
#	define __GNUC_MINOR__ 4 // Assume GCC 4.4 crosscompiler
#endif
