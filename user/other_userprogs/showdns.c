/*
 * Shows a DNS query.
 * Written by: cooky451 - 8/15/2011
 * Last change: cooky451 - 8/15/2011
 * Todo: -
 * Comments: -
 */

#include "stdio.h"
#include "dns_help.h"

int main(int argc, char* argv[])
{
    char buf[256];
    char *name = argv[1];

    if (argc < 2)
    {
        printf("Hostname: ");
        name = gets(buf);
    }

    showDNSQuery(name);
    return 0;
}
