#include "mkyaffs2image.h"
#include "unyaffs.h"

#include "roots.h"

int nandroid_backup_path_yaffs(char* path, char* backup_path) {
    int was_mounted = is_path_mounted(path);
    int ret = 0;

    if(0 != (ret = ensure_path_mounted(path))) {
        return ret;
    }

    ret = mkyaffs2image(path, backup_path, 0, NULL);

    if(!was_mounted)
        ensure_path_unmounted(path);

    return ret;
}

int nandroid_restore_path_yaffs(char* path, char* backup_path) {
    int was_mounted = is_path_mounted(path);
    int ret = 0;

    if(0 != (ret = ensure_path_mounted(path))) {
        return ret;
    }

    ret = unyaffs(backup_path, path, NULL);

    if(!was_mounted)
        ensure_path_unmounted(path);

    return ret;
}
