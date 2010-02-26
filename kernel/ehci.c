/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "pci.h"

void analyzeEHCI(uint32_t bar)
{
    uint32_t EHCI_data;

    EHCI_data = *((volatile uint8_t* )(bar + 0x00));
    uint32_t OpRegs = bar + EHCI_data;
    //printformat("\nCAPLENGTH: %y ", EHCI_data); // Capability Register Length

    EHCI_data = *((volatile uint16_t*)(bar + 0x02));
    printformat(  "HCIVERSION: %x ", EHCI_data); // Interface Version Number

    EHCI_data = *((volatile uint32_t*)(bar + 0x04));
    printformat(  "HCSPARAMS: %X ", EHCI_data); // Structural Parameters

    EHCI_data = *((volatile uint32_t*)(bar + 0x08));
    printformat(  "HCCPARAMS: %X ", EHCI_data); // Capability Parameters

    EHCI_data = BYTE2(*((volatile uint32_t*) (bar + 0x08)));
    if(EHCI_data==0) printformat("No ext. capabil. "); // Extended Capabilities Pointer

    printformat("\nOpRegs Address: %X ", OpRegs); // Host Controller Operational Registers
}

/*
* Copyright (c) 2009 The PrettyOS Project. All rights reserved.
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

