#ifndef RECOVERY_RECOVERY_LIB_H_
#define RECOVERY_RECOVERY_LIB_H_

// returns true if this device has a volume identified by the provided path
int has_volume(const char* path);

// processes volumes on startup immediately after the volume table has been loaded
void process_volumes();

// wrapper for the linux reboot() command, used to ensure recovery is finalized prior to reboot
int recovery_reboot(int cmd);

#endif//RECOVERY_RECOVERY_LIB_H_
