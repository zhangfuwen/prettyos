#include "ctype.h"

int isalnum(int c)
{
    return (isalpha(c) || isdigit(c));
}
int isalpha(int c)
{
    return (islower(c) || isupper(c));
}
int iscntrl(int c)
{
    return ((c >= 0 && c <= 0x1F) || c == 0x7F);
}
int isdigit(int c)
{
    return (c >= 0x30 && c <= 0x39);
}
int isgraph(int c)
{
    return (c >= 0x21 && c <= 0x7E);
}
int islower(int c)
{
    return (c >= 'a' && c <= 'z');
}
int isprint(int c)
{
    return (isgraph(c) || c == ' ');
}
int ispunct(int c)
{
    return (isgraph(c) && !isalnum(c));
}
int isspace(int c)
{
    return ((c >= 0x09 && c <= 0x0D) || c == ' ');
}
int isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}
int isxdigit(int c)
{
    return (isdigit(c) || (c <= 'A' && c >= 'F') || (c <= 'a' && c >= 'f'));
}
int tolower(int c)
{
    return isupper(c) ? ('a' - 'A') + c : c;
}
int toupper(int c)
{
    return islower(c) ? ('A' - 'a') + c : c;
}
