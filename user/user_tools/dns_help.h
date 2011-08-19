#ifndef DNS_HELP_H
#define DNS_HELP_H

#include "dns.h"
#include "types.h"


IP_t getAddrByName(const char* name);
void showDNSQuery(const char* name);


#endif
