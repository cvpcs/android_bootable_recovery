#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

#include "common.h"
#include "roots.h"
#include "recovery_lib.h"
#include "recovery_config.h"

#include "nandroid/nandroid.h"
#include "nandroid/nandroid_scan.h"

// include nandroid types
#include "nandroid/nandroid_raw.h"
#include "nandroid/nandroid_tar.h"
#include "nandroid/nandroid_yaffs.h"

#define NANDROID_MD5SUM_FILE "nandroid.md5"

int nandroid_backup_path(char* path, char* backup_dir) {
    Volume* v = volume_for_path(path);
    if(v == NULL) {
        ui_print("Skipping backup of %s ... (volume not found)\n", path);
        return -1;
    }

    char* backup_file = get_nandroid_file_for_path(backup_dir, path);
    if(backup_file == NULL) {
        ui_print("Skipping backup of %s ... (backup file could not be generated)\n", path);
        return -1;
    }

    int ret;
    int type = get_nandroid_type_for_new(path);

    ui_print("Backing up %s ... ", path);
    switch(type) {
    case NANDROID_TYPE_RAW:
        ret = nandroid_backup_path_raw(path, backup_file);
        break;
    case NANDROID_TYPE_TAR:
        ret = nandroid_backup_path_tar(path, backup_file);
        break;
    case NANDROID_TYPE_TAR_GZ:
        ret = nandroid_backup_path_tar_gz(path, backup_file);
        break;
    case NANDROID_TYPE_TAR_BZ2:
        ret = nandroid_backup_path_tar_bz2(path, backup_file);
        break;
    case NANDROID_TYPE_YAFFS:
        ret = nandroid_backup_path_yaffs(path, backup_file);
    default:
        ret = -1;
        break;
    }
    if(ret == 0) {
        ui_print("done\n");
    } else {
        ui_print("error (%d)\n", ret);
    }

    free(backup_file);
    return ret;
}

int nandroid_restore_path(char* path, char* backup_dir) {
    Volume* v = volume_for_path(path);

    if(v == NULL) {
        ui_print("Skipping restore of %s ... (volume not found)\n", path);
        return -1;
    }

    int type = get_nandroid_type_for_existing(backup_dir, path);
    if(type == NANDROID_TYPE_NULL) {
        ui_print("Skipping restore of %s ... (no backup found)\n", path);
        return -1;
    }

    char* backup_file = get_nandroid_file_for_path(backup_dir, path);
    if(backup_file == NULL) {
        ui_print("Skipping restore of %s ... (backup file not found)\n", path);
        return -1;
    }

    int ret;
    struct stat file_info;
    if (0 != (ret = statfs(backup_file, &file_info))) {
        ui_print("Skipping restore of %s ... (%s not found)\n", path, backup_file);
        return ret;
    }

    ui_print("Erasing %s before restore ... ", path);
    if (0 != (ret = format_volume(v->mount_point))) {
        ui_print("error (%d)\n", ret);
        free(backup_file);
        return ret;
    } else {
        ui_print("done\n");
    }

    int was_p_mounted;

    ui_print("Restoring %s ... ", path);
    switch(type) {
    case NANDROID_TYPE_RAW:
        ret = nandroid_restore_path_raw(path, backup_file);
        break;
    case NANDROID_TYPE_TAR:
        ret = nandroid_restore_path_tar(path, backup_file);
        break;
    case NANDROID_TYPE_TAR_GZ:
        ret = nandroid_restore_path_tar_gz(path, backup_file);
        break;
    case NANDROID_TYPE_TAR_BZ2:
        ret = nandroid_restore_path_tar_bz2(path, backup_file);
        break;
    case NANDROID_TYPE_YAFFS:
        ret = nandroid_restore_path_yaffs(path, backup_file);
    default:
        ret = -1;
        break;
    }
    if(ret == 0) {
        ui_print("done\n");
    } else {
        ui_print("error (%d)\n", ret);
    }

    free(backup_file);
    return ret;
}

int nandroid_md5_create(char* backup_dir) {
    const char* create_md5_format = "cd %s && md5sum %s > %s";
    const char* append_md5_format = "cd %s && md5sum %s >> %s";

    int ret = 0;
    int count = 0;

    ui_print("Creating MD5 sums ... ");

    // open the directory
    DIR* dir = opendir(backup_dir);
    if (dir != NULL) {
        struct dirent* de;
    	while ((de = readdir(dir)) != NULL) {
            if(de->d_name[0] != '.' && // don't do '.' files
                    de->d_type == DT_REG && // make sure it's a regular file
                    strcmp(de->d_name, NANDROID_MD5SUM_FILE) != 0) { // dont md5 the md5 file if overwriting
                if(count++ == 0) {
                    int cmdlen = strlen(create_md5_format) + strlen(backup_dir) + strlen(de->d_name) + strlen(NANDROID_MD5SUM_FILE);
                    char* cmd = (char*)calloc(cmdlen, sizeof(char));
                    snprintf(cmd, cmdlen, create_md5_format, backup_dir, de->d_name, NANDROID_MD5SUM_FILE);
                    ret |= __system(cmd);
                    free(cmd);
                } else {
                    int cmdlen = strlen(append_md5_format) + strlen(backup_dir) + strlen(de->d_name) + strlen(NANDROID_MD5SUM_FILE);
                    char* cmd = (char*)calloc(cmdlen, sizeof(char));
                    snprintf(cmd, cmdlen, append_md5_format, backup_dir, de->d_name, NANDROID_MD5SUM_FILE);
                    ret |= __system(cmd);
                    free(cmd);
                }
            }
        }

        closedir(dir);
    } else {
        ret = -1;
    }

    if (0 != ret)
        ui_print("error (%d)\n", ret);
    else
        ui_print("done\n");

    return ret;
}

int nandroid_md5_check(char* backup_dir) {
    const char* check_md5_format = "cd %s && md5sum -c %s";

    int cmdlen = strlen(check_md5_format) + strlen(backup_dir) + strlen(NANDROID_MD5SUM_FILE);
    char* cmd = (char*)calloc(cmdlen, sizeof(char));
    snprintf(cmd, cmdlen, check_md5_format, backup_dir, NANDROID_MD5SUM_FILE);

    int ret;

    ui_print("Checking MD5 sums ... ");
    if (0 != (ret = __system(cmd)))
        ui_print("error (%d)\n", ret);
    else
        ui_print("done\n");

    free(cmd);
    return ret;
}

int nandroid_ensure_dir(char* backup_dir) {
    ensure_path_mounted(backup_dir);

    const char* remove_dir_format = "rm -f %s/*";
    const char* ensure_dir_format = "mkdir -p %s";

    int cmdlen = strlen(remove_dir_format) + strlen(backup_dir);
    char* cmd = (char*)calloc(cmdlen, sizeof(char));
    snprintf(cmd, cmdlen, remove_dir_format, backup_dir);
    int ret = __system(cmd);
    free(cmd);

    cmdlen = strlen(ensure_dir_format) + strlen(backup_dir);
    cmd = (char*)calloc(cmdlen, sizeof(char));
    snprintf(cmd, cmdlen, ensure_dir_format, backup_dir);
    ret |= __system(cmd);
    free(cmd);

    return ret;
}

void nandroid_backup(char* backup_dir, int* partitions) {
    int ret;

    if(0 != (ret = nandroid_ensure_dir(backup_dir))) {
        ui_print("Error creating backup directory (%d)\n", ret);
        return;
    }

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
            if(i >= 0 && i < device_partition_num && // make sure our iterator is valid
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
    recovery_config* rconfig = get_config();
    if(rconfig && rconfig->nandroid_do_md5_verification) {
        if(0 != nandroid_md5_check(backup_dir)) {
            return;
        }
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
            if(i >= 0 && i < device_partition_num && // make sure our iterator is valid
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

