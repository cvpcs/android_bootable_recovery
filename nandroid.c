#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "nandroid.h"
#include "common.h"
#include "roots.h"
#include "recovery_lib.h"

char* get_backup_path(char* backup_dir, char* path_or_device, const char* ext) {
    if(backup_dir == NULL || path_or_device == NULL || ext == NULL)
        return NULL;

    int bdlen = strlen(backup_dir);
    int podlen = strlen(path_or_device);

    if(bdlen <= 0 || podlen <= 0)
        return NULL;

    char* p = path_or_device;

    if(*p == '/') {
        // using a path, so set our pointer to the next item
        p++;
    }

    const char* backup_path_format = "%s/%s%s";
    int plen = bdlen + podlen + strlen(ext) + strlen(backup_path_format);
    char* path = (char*)calloc(plen, sizeof(char));
    snprintf(path, plen, backup_path_format, backup_dir, p, ext);
    return path;
}

int nandroid_md5_create(char* backup_dir) {
    // TODO: Need to implement this
    return 0;
}

int nandroid_md5_check(char* backup_dir) {
    // TODO: Need to implement this
    return 0;
}

int nandroid_backup_path(char* path, char* backup_dir) {
    Volume* v = volume_for_path(path);

    if(v == NULL) {
        LOGE("Could not find volume for path: %s", path);
        return -1;
    }

    int ret;
    char* backup_path;

    if(strcmp(v->fs_type, "mtd") == 0) {
        // need to do a raw backup
        backup_path = get_backup_path(backup_dir, v->device, ".img");
        if(backup_path == NULL) {
            LOGE("Error generating backup path: %s", path);
            return -1;
        } else {
            ret = nandroid_raw_backup_path(v->device, backup_path);
        }
    } else {
        int was_mounted = is_path_mounted(path);
        ensure_path_mounted(path);

        // TODO: do either a yaffs2 or tgz backup

        // do a tgz backup as default for now
        backup_path = get_backup_path(backup_dir, path, ".tar.gz");
        if(backup_path == NULL) {
            LOGE("Error generating backup path: %s", path);
            return -1;
        } else {
            ret = nandroid_tgz_backup_path(path, backup_path);
        }

        // if the path was unmounted originally, unmount it again
        if(!was_mounted) {
            ensure_path_unmounted(path);
        }
    }

    free(backup_path);
    return ret;
}

int nandroid_restore_path(char* path, char* backup_dir) {
    Volume* v = volume_for_path(path);

    if(v == NULL) {
        LOGE("Could not find volume for path: %s", path);
        return -1;
    }

    int ret;
    char* backup_path;

    if(strcmp(v->fs_type, "mtd") == 0) {
        backup_path = get_backup_path(backup_dir, v->device, ".img");
    } else {
        backup_path = get_backup_path(backup_dir, path, ".tar.gz");
    }

    if(backup_path == NULL) {
        LOGE("Error generating backup path: %s", path);
        return -1;
    }

    struct stat file_info;
    if (0 != (ret = statfs(backup_path, &file_info))) {
        ui_print("%s not found. Skipping restore of %s. (%d)\n", backup_path, path, ret);
        return ret;
    }

    ui_print("Erasing %s before restore ... ", path);
    if (0 != (ret = format_volume(v->mount_point))) {
        ui_print("error (%d)\n", ret);
        free(backup_path);
        return ret;
    } else {
        ui_print("done\n");
    }


    if(strcmp(v->fs_type, "mtd") == 0) {
        ret = nandroid_raw_restore_path(v->device, backup_path);
    } else {
        int was_mounted = is_path_mounted(path);
        ensure_path_mounted(path);

        // TODO: do either a yaffs2 or tgz restore

        // do a tgz restore as default for now
        ret = nandroid_tgz_restore_path(path, backup_path);

        // if the path was unmounted originally, unmount it again
        if(!was_mounted) {
            ensure_path_unmounted(path);
        }
    }

    free(backup_path);
    return ret;
}

int nandroid_raw_backup_path(char* device, char* backup_path) {
    int ret;
    ui_print("Backing up %s image ... ", device);
    if (0 != (ret = backup_raw_partition(device, backup_path))) {
        ui_print("error (%d)\n", ret);
        return ret;
    } else {
        ui_print("done\n");
    }
    return ret;
}

int nandroid_raw_restore_path(char* device, char* backup_path) {
    int ret;
    ui_print("Restoring %s image ... ", device);
    if (0 != (ret = restore_raw_partition(device, backup_path))) {
        ui_print("error (%d)\n", ret);
        return ret;
    } else {
        ui_print("done\n");
    }
    return ret;
}

int nandroid_tgz_backup_path(char* path, char* backup_path) {
    const char* tar_cmd = "tar -zcvf %s -C %s";

    int cmd_len = strlen(tar_cmd) + strlen(path) + strlen(backup_path);
    char* cmd = (char*)calloc(cmd_len, sizeof(char));

    snprintf(cmd, cmd_len, tar_cmd, backup_path, path);
    int ret = __system(cmd);

    free(cmd);

    return ret;
}

int nandroid_tgz_restore_path(char* path, char* backup_path) {
    const char* tar_cmd = "tar -zxvf %s -C %s";

    int cmd_len = strlen(tar_cmd) + strlen(path) + strlen(backup_path);
    char* cmd = (char*)calloc(cmd_len, sizeof(char));

    snprintf(cmd, cmd_len, tar_cmd, backup_path, path);
    int ret = __system(cmd);

    free(cmd);

    return ret;
}

void nandroid_backup(char* backup_dir, int* partitions) {
    if(partitions == NULL) {
        // if null, we will be backing up all of them
        int i;
        for(i = 0; i < device_partition_num; ++i) {
            if((device_partitions[i].flags & PARTITION_FLAG_SAVEABLE) > 0 &&
                    has_volume(device_partitions[i].path)) {
                // we print ui updates on error here, so we don't check errors here
                nandroid_backup_path(device_partitions[i].path, backup_dir);
            }
        }
    } else {
        while(partitions) {
            int i = *partitions;
            if(i < device_partition_num && // make sure our iterator is valid
                    device_partitions[i].id == i && // sanity-check the id
                    (device_partitions[i].flags & PARTITION_FLAG_SAVEABLE) > 0 &&
                    has_volume(device_partitions[i].path)) {
                // we print ui updates on error here, so we don't check errors here
                nandroid_backup_path(device_partitions[i].path, backup_dir);
            }
            partitions++;
        }
    }

    nandroid_md5_create(backup_dir);
}

void nandroid_restore(char* backup_dir, int* partitions) {
    if(0 != nandroid_md5_check(backup_dir)) {
        return;
    }

    if(partitions == NULL) {
        // if null, we will be restoring up all of them
        int i;
        for(i = 0; i < device_partition_num; ++i) {
            if((device_partitions[i].flags & PARTITION_FLAG_RESTOREABLE) > 0 &&
                    has_volume(device_partitions[i].path)) {
                // we print ui updates on error here, so we don't check errors here
                nandroid_restore_path(device_partitions[i].path, backup_dir);
            }
        }
    } else {
        while(partitions) {
            int i = *partitions;
            if(i < device_partition_num && // make sure our iterator is valid
                    device_partitions[i].id == i && // sanity-check the id
                    (device_partitions[i].flags & PARTITION_FLAG_RESTOREABLE) > 0 &&
                    has_volume(device_partitions[i].path)) {
                // we print ui updates on error here, so we don't check errors here
                nandroid_restore_path(device_partitions[i].path, backup_dir);
            }
            partitions++;
        }
    }
}
