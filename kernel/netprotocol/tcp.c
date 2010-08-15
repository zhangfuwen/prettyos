/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "tcp.h"
#include "video/console.h"

void tcpDebug(tcpheader_t *tcp)
{
  printf("\n");
  printf("TCP Header information:\n");
  textColor(0x0E);
  printf("+--------------+----------------+\n");
  printf("|      %u       |      %u        | (source port, destination port)\n", tcp->sourcePort, tcp->destinationPort);
  printf("+-------------------------------+\n");
  printf("|              %u                | (sequence number)\n", tcp->sequence_number);
  printf("+-------------------------------+\n");
  printf("|              %u                | (acknowledgmentnumber)\n", tcp->acknowledgment_number);
  printf("+-------------------------------+\n");
  printf("| %u  |    |%u%u%u%u%u|    %u           | (data offset, flags (", tcp->URG, tcp->ACK, tcp->PSH, tcp->RST, tcp->SYN, tcp->FIN, tcp->window); printf("), window)\n");
  printf("+--------------+----------------+\n");
  printf("|     %u        |      %u        | (checksum, urgent pointer)\n", tcp->checksum, tcp->urgent_pointer);
  printf("+-------------------------------+\n");
}

/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
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