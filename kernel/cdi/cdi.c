/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "cdi.h"
#include "list.h"
#include "cdi/pci.h"
#include "cdi/storage.h"
#include "cdi/fs.h"
#include "video/console.h"

static cdi_list_t drivers = 0;

extern struct cdi_driver* _cdi_start; // Declared in kernel.ld
extern struct cdi_driver* _cdi_end;   // Declared in kernel.ld

// Initialisiert alle PCI-Geraete
static void cdi_tyndur_init_pci_devices(void)
{
    struct cdi_driver* driver;
    struct cdi_device* device;
    struct cdi_pci_device* pci;
    int i, j;

    // Liste der PCI-Geraete holen
    cdi_list_t pci_devices = cdi_list_create();
    cdi_pci_get_all_devices(pci_devices);

    // Fuer jedes Geraet einen Treiber suchen
    for (i = 0; (pci = cdi_list_get(pci_devices, i)); i++) {

        device = 0;
        for (j = 0; (driver = cdi_list_get(drivers, j)); j++)
        {
            if (driver->bus == CDI_PCI && driver->init_device)
            {
                device = driver->init_device(&pci->bus_data);
                break;
            }
        }

        if (device != 0)
        {
            cdi_list_push(driver->devices, device);
            printf("cdi: %x.%x.%x: Benutze Treiber %s\n",
                pci->bus, pci->dev, pci->function, driver->name);
        }
        else
        {
            cdi_pci_device_destroy(pci);
        }
    }

    cdi_list_destroy(pci_devices);
}

/* Diese Funktion wird von Treibern aufgerufen, nachdem ein neuer Treiber
   hinzugefuegt worden ist.

   Sie registriert typischerweise die neu hinzugefuegten Treiber und/oder
   Geraete beim Betriebssystem und startet damit ihre Ausfuehrung.

   Nach dem Aufruf dieser Funktion duerfen vom Treiber keine weiteren Befehle
   ausgefuehrt werden, da nicht definiert ist, ob und wann die Funktion
   zurueckkehrt. */
static void cdi_tyndur_run_drivers()
{
    // PCI-Geraete suchen
    cdi_tyndur_init_pci_devices();

    // Geraete initialisieren
    struct cdi_driver* driver;
    struct cdi_device* device;
    int i, j;
    for (i = 0; (driver = cdi_list_get(drivers, i)); i++)
    {
        for (j = 0; (device = cdi_list_get(driver->devices, j)); j++)
        {
            device->driver = driver;
        }

        if (driver->type != CDI_NETWORK)
        {
            ///init_service_register((char*) driver->name);
        }
    }

    // Warten auf Ereignisse
    ///while (1)
    ///{
    ///    wait_for_rpc();
    ///}
}

void cdi_init()
{
    struct cdi_driver** pdrv;
    struct cdi_driver* drv;

    // Interne Strukturen initialisieren
    drivers = cdi_list_create();
    ///atexit(cdi_destroy);

    ///lostio_init();
    ///lostio_type_directory_use();
    ///timer_sync_caches();

    // Alle in dieser Binary verfuegbaren Treiber aufsammeln
    pdrv = &_cdi_start;
    while (pdrv < &_cdi_end)
    {
        drv = *pdrv;
        if (drv->init != 0)
        {
            // FIXME Der Service muss registriert sein, wenn die Karte bei
            // tcpip registriert wird (fuer den Namen) und das passiert im
            // Moment in drv->init()
            if (drv->type == CDI_NETWORK)
            {
                ///init_service_register((char*) drv->name);
            }

            drv->init();
            ///cdi_driver_register(drv);
        }
        pdrv++;
    }

    // Treiber starten
    cdi_tyndur_run_drivers();
}

void cdi_driver_init(struct cdi_driver* driver)
{
    driver->devices = cdi_list_create();
}

void cdi_driver_destroy(struct cdi_driver* driver)
{
    cdi_list_destroy(driver->devices);
}

void cdi_driver_register(struct cdi_driver* driver)
{
    cdi_list_push(drivers, driver);

    switch (driver->type)
    {
        case CDI_STORAGE:
            //cdi_storage_driver_register((struct cdi_storage_driver*) driver);
            break;
        case CDI_FILESYSTEM:
            cdi_fs_driver_register((struct cdi_fs_driver*) driver);
            break;
        default:
            break;
    }
}

/* Wenn main nicht von einem Treiber ueberschrieben wird, ist hier der
   Einsprungspunkt. Die Standardfunktion ruft nur cdi_init() auf. Treiber, die
   die Funktion ueberschreiben, koennen argc und argv auswerten und muessen als
   letztes ebenfalls cdi_init aufrufen. */
int __attribute__((weak)) main(void)
{
    cdi_init();
    return (0);
}

/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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
