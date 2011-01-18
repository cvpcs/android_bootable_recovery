#include "common.h"
#include "roots.h"

#include "flashutils/flashutils.h"

#include "nandroid/nandroid.h"

int nandroid_backup_path_raw(char* path, char* backup_path) {
    int ret;
    if(0 != (ret = ensure_path_unmounted(path))) {
        return ret;
    }

    Volume* v = volume_for_path(path);
    if(v == NULL) {
        return -1;
    }

    return backup_raw_partition(v->device, backup_path);
}

int nandroid_restore_path_raw(char* path, char* backup_path) {
    int ret;
    if(0 != (ret = ensure_path_unmounted(path))) {
        return ret;
    }

    Volume* v = volume_for_path(path);
    if(v == NULL) {
        return -1;
    }

    return restore_raw_partition(v->device, backup_path);
}
