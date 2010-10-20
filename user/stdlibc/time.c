#include "time.h"

char* asctime(const struct tm* timeptr); /// TODO
clock_t clock(); /// TODO
char* ctime(const time_t* time); /// TODO
double difftime(time_t t1, time_t t2); /// TODO
struct tm* gmtime(const time_t* timer); /// TODO
struct tm* localtime(const time_t* timer); /// TODO
time_t mktime(struct tm* timeptr); /// TODO
size_t strftime(char* ptr, size_t maxsize, const char* format, const struct tm* timeptr); /// TODO
time_t time(time_t* timer); /// TODO
