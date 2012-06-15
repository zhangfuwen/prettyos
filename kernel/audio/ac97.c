/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "ac97.h"
#include "util/util.h"
#include "video/console.h"
#include "paging.h"
#include "timer.h"

void install_AC97(pciDev_t* device)
{
    uint16_t pciCommandRegister = pci_config_read(device->bus, device->device, device->func, PCI_COMMAND, 2);
    // printf("\nCMD REG: %x ==> ",pciCommandRegister);

    pci_config_write_word(device->bus, device->device, device->func, PCI_COMMAND, pciCommandRegister | PCI_CMD_BUSMASTER | PCI_CMD_IO);
    pciCommandRegister = pci_config_read(device->bus, device->device, device->func, PCI_COMMAND, 2);
    // printf("%x\n",pciCommandRegister);

    // first and second address room
    uint32_t nambar  = device->bar[0].baseAddress; // NAM-BAR  // Mixer
    uint32_t nabmbar = device->bar[1].baseAddress; // NABM-BAR // Player
    printf("\nnambar: %X nabmbar: %X  ",nambar, nabmbar);

    // reset
    outportw(nambar  + PORT_NAM_RESET, 42);            // Each value is possible
    outportb(nabmbar + PORT_NABM_GLB_CTRL_STAT, 0x02); // 0x02 is mandatory
    sleepMilliSeconds(100);

    // volume
    uint8_t volume = 0; //Am lautesten!
    outportw(nambar + PORT_NAM_MASTER_VOLUME, (volume<<8) | volume); // General volume left and right
    outportw(nambar + PORT_NAM_MONO_VOLUME,    volume);              // Volume for Mono
    outportw(nambar + PORT_NAM_PC_BEEP,        volume);              // Volume for PC speaker
    outportw(nambar + PORT_NAM_PCM_VOLUME,    (volume<<8) | volume); // Volume for PCM left and right
    sleepMilliSeconds(10);

    // sample rate
    if (!(inportw(nambar + PORT_NAM_EXT_AUDIO_ID) & 1))
    {
        // sample rate is fixed to 48 kHz
    }
    else
    {
        outportw(nambar + PORT_NAM_EXT_AUDIO_STC, inportw(nambar + PORT_NAM_EXT_AUDIO_STC) | 1); // Activate variable rate audio
        sleepMilliSeconds(10);
        outportw(nambar + PORT_NAM_FRONT_SPLRATE, 44100); // General sample rate: 44100 Hz
        outportw(nambar + PORT_NAM_LR_SPLRATE,    44100); // Stereo  sample rate: 44100 Hz
        sleepMilliSeconds(10);
    }

    // Actual sample rate can be read from PORT_NAM_FRONT_SPLRATE or PORT_NAM_LR_SPLRATE
    printf("sample rate: %u Hz\n", inportw(nambar + PORT_NAM_FRONT_SPLRATE));

    // Generate beep of ~23 sec length
    uint16_t buffer[65536];
    bool tick = false;
    for(size_t i = 0; i < 65536; i++) {
        if(i%100 == 0)
            tick = !tick;
        if(tick)
            buffer[i] = 0x7FFF;
        else
            buffer[i] = 0xFFFF;
    }
    struct buf_desc descs[32];
    for(int i = 0; i < 32; i++) {
        descs[i].buf = (void*)paging_getPhysAddr(buffer);
        descs[i].len = 0xFFFE;
        descs[i].ioc = 1;
        descs[i].bup = 0;
    }
    descs[31].bup = 1;
    outportl(nabmbar + PORT_NABM_POBDBAR, paging_getPhysAddr(descs));
    outportb(nabmbar + PORT_NABM_POLVI, 31);
    outportb(nabmbar + PORT_NABM_POCONTROL, 0x15); //Abspielen, und danach auch Interrupt generieren!
}

/*
* Copyright (c) 2009-2012 The PrettyOS Project. All rights reserved.
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
