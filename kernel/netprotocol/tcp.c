/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/

#include "tcp.h"
#include "video/console.h"


void tcpDebug(tcpPacket_t* tcp)
{
  printf("\n");
  printf("TCP Header information:\n");
  textColor(0x0E);
  printf("+--------------+----------------+\n");
  printf("|      %u    |      %u          (source port, destination port)\n", ntohs(tcp->sourcePort), ntohs(tcp->destinationPort));
  printf("+-------------------------------+\n");
  printf("|      %X                  (sequence number)\n", ntohl(tcp->sequenceNumber));
  printf("+-------------------------------+\n");
  printf("|              %u                  (acknowledgmentnumber)\n", ntohl(tcp->acknowledgmentNumber));
  printf("+-------------------------------+\n");
  printf("| |%u%u%u%u%u%u|        %u           (flags: URG ACK PUSH RESET SYN FIN", /*tcp->CWR, tcp->ECN,*/ tcp->URG, tcp->ACK, tcp->PSH, tcp->RST, tcp->SYN, tcp->FIN, ntohs(tcp->window)); printf(", window)\n");
  printf("+--------------+----------------+\n");
  printf("|    %x     |      %u           (checksum, urgent pointer)\n", ntohs(tcp->checksum), ntohs(tcp->urgentPointer));
  printf("+-------------------------------+\n");
}

/*
* Copyright (c) 2010-2011 The PrettyOS Project. All rights reserved.
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
