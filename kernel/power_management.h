#ifndef POWERMANAGEMENT_H
#define POWERMANAGEMENT_H

#include "os.h"

typedef enum {
	PM_NO, PM_APM, PM_ACPI, _PM_SYSTEMS_END
} PM_SYSTEMS;

typedef enum {
	PM_STANDBY, PM_SOFTOFF, PM_REBOOT, _PM_STATES_END
} PM_STATES;

typedef struct {
	bool supported;            // Indicates wheter this power management system is supported or not.
	bool (*action)(PM_STATES); // Executed when trying to enter to another powermanagemt state

} PM_SYSTEM_t;

void pm_install(); // Detects and configures power management system (chooses best one, that is supported)
void pm_log(); // Prints the supported PM-systems
bool pm_action(PM_STATES state); // Enters to a power management state. Returns true if supported (or a workaround is found) and successful

#endif
