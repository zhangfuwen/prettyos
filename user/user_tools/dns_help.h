/*
 * Wrapper for DNS functions.
 */

#ifndef DNS_HELP_H
#define DNS_HELP_H

#include "dns.h"
#include "userlib.h"

IP_t getAddrByName(const char* name);
void showDNSQuery(const char* name);

#endif
