/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "icmp.h"
#include "types.h"

// Compute Internet Checksum for "count" bytes beginning at location "addr".
/*
void internetChecksum()
{
    // register 
	uint32_t sum = 0;
	uint32_t count, checksum;
	uint16_t addr;
    while( count > 1 )  
    {
        // This is the inner loop 
        sum += * (uint16_t) addr++;
        count -= 2;
    }
    // Add left-over byte, if any 
    if( count > 0 )
    sum += * (uint8_t*) addr;
    // Fold 32-bit sum to 16 bits 
    while (sum>>16)
    sum = (sum & 0xffff) + (sum >> 16);
    checksum = ~sum;
}
*/

/*
void internetChecksum()
{
	register long sum = 0;

	while( count > 1 )  {
	   //  This is the inner loop 
		   sum += * (unsigned short) addr++;
		   count -= 2;
	}

	   // Add left-over byte, if any 
	if( count > 0 )
		   sum += * (unsigned char *) addr;

	   //  Fold 32-bit sum to 16 bits 
	while (sum>>16)
	   sum = (sum & 0xffff) + (sum >> 16);

	checksum = ~sum;
}
*/

/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/