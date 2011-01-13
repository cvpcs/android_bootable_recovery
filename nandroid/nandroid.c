#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

#include "nandroid.h"
#include "common.h"
#include "roots.h"
#include "recovery_lib.h"

// nandroid types
#define NANDROID_TYPE_NULL    -1 // null if shouldn't be nandroided
#define NANDROID_TYPE_RAW      0 // raw MTD image
#define NANDROID_TYPE_TAR      1 // tar file
#define NANDROID_TYPE_TAR_GZ   2 // tar.gz file
#define NANDROID_TYPE_TAR_BZ2  3 // tar.bz2 file 
#define NANDROID_TYPE_TAR_LZMA 4 // tar.lzma file (not supported yet)
#define NANDROID_TYPE_YAFFS    5 // yaffs image (fastboot/cwm)

// extensions for the various nandroid types
#define NANDROID_TYPE_RAW_EXT      ".img"
#define NANDROID_TYPE_TAR_EXT      ".tar"
#define NANDROID_TYPE_TAR_GZ_EXT   ".tar.gz"
#define NANDROID_TYPE_TAR_BZ2_EXT  ".tar.bz2"
#define NANDROID_TYPE_TAR_LZMA_EXT ".tar.lzma"
#define NANDROID_TYPE_YAFFS_EXT    ".img"

#define NANDROID_MD5_EXT ".md5"

extern int __system(const char* command);

// determines the default nandroid type for non-raw partitions
int get_default_nandroid_type() {
    // TODO: determine this from an option

    return NANDROID_TYPE_TAR_GZ;
}

// determines the type of nandroid that should be used on this root path (writing)
int get_nandroid_type_for_path(char* path) {
    Volume* v = volume_for_path(path);
    if(v == NULL)
        return NANDROID_TYPE_NULL;

    if(strcmp(v->fs_type, "mtd") == 0) {
        return NANDROID_TYPE_RAW;
    } else {
        return get_default_nandroid_type();
    }
}

// determines the type of nandoid from a nandroid directory and path (reading)
int get_nandroid_type_for_file(char* backup_dir, char* path) {
    Volume* v = volume_for_path(path);
    if(v == NULL)
        return NANDROID_TYPE_NULL;

    ensure_path_mounted(backup_dir);

    const char* file_format = "%s/%s%s";

    struct stat file_info;
    int ret = NANDROID_TYPE_NULL;
    char* buf = (char*)calloc(PATH_MAX,sizeof(char));

    if(strcmp(v->fs_type, "mtd") == 0) {
        if(snprintf(buf, PATH_MAX, file_format, backup_dir, v->device, NANDROID_TYPE_RAW_EXT) && 0 == statfs(buf, &file_info)) {
            ret = NANDROID_TYPE_RAW;
        }
    } else {
        char* p = path + 1; // (chunk off the / character);
        if(snprintf(buf, PATH_MAX, file_format, backup_dir, p, NANDROID_TYPE_TAR_EXT) && 0 == statfs(buf, &file_info)) {
            ret = NANDROID_TYPE_TAR;
        } else if(snprintf(buf, PATH_MAX, file_format, backup_dir, p, NANDROID_TYPE_TAR_GZ_EXT) && 0 == statfs(buf, &file_info)) {
            ret = NANDROID_TYPE_TAR_GZ;
        } else if(snprintf(buf, PATH_MAX, file_format, backup_dir, p, NANDROID_TYPE_TAR_BZ2_EXT) && 0 == statfs(buf, &file_info)) {
            ret = NANDROID_TYPE_TAR_BZ2;
        } else if(snprintf(buf, PATH_MAX, file_format, backup_dir, p, NANDROID_TYPE_TAR_LZMA_EXT) && 0 == statfs(buf, &file_info)) {
            ret = NANDROID_TYPE_TAR_LZMA;
        } else if(snprintf(buf, PATH_MAX, file_format, backup_dir, p, NANDROID_TYPE_YAFFS_EXT) && 0 == statfs(buf, &file_info)) {
            ret = NANDROID_TYPE_YAFFS;
        }
    }

    free(buf);
    return ret;
}

// will return the name of an existing nandroid within backup_dir for path, or generate an appropriate name if one does not exist
// it is the responsibility of the caller to differentiate between backup and restore calls
char* get_nandroid_path_for_path(char* backup_dir, char* path) {
    Volume* v = volume_for_path(path);
    if(v == NULL)
        return NULL;

    // first see if we're getting an existing path
    int type = get_nandroid_type_for_file(backup_dir, path);

    if(type == NANDROID_TYPE_NULL) {
        type = get_nandroid_type_for_path(path);
    }

    char* ext;
    char* p = path + 1;

    switch(type) {
    case NANDROID_TYPE_RAW:
        ext = NANDROID_TYPE_RAW_EXT;
        p = v->device;
        break;
    case NANDROID_TYPE_TAR:
        ext = NANDROID_TYPE_TAR_EXT;
        break;
    case NANDROID_TYPE_TAR_GZ:
        ext = NANDROID_TYPE_TAR_GZ_EXT;
        break;
    case NANDROID_TYPE_TAR_BZ2:
        ext = NANDROID_TYPE_TAR_BZ2_EXT;
        break;
    case NANDROID_TYPE_TAR_LZMA:
        ext = NANDROID_TYPE_TAR_LZMA_EXT;
        break;
    case NANDROID_TYPE_YAFFS:
        ext = NANDROID_TYPE_YAFFS_EXT;
        break;
    default:
        return NULL;
    }

    const char* backup_path_format = "%s/%s%s";
    int bplen = strlen(backup_dir) + strlen(p) + strlen(ext) + strlen(backup_path_format);
    char* backup_path = (char*)calloc(bplen, sizeof(char));
    snprintf(backup_path, bplen, backup_path_format, backup_dir, p, ext);
    return backup_path;
}

// scan a directory and see if it is a nandroid
nandroid* nandroid_scan_dir(char* dir_path) {
    // only an int array, no need to count
    int* partitions = (int*)calloc(device_partition_num + 1, sizeof(int));

    int i;
    int j = 0;
    for(i = 0; i < device_partition_num; ++i) {
        if((device_partitions[i].flags & PARTITION_FLAG_RESTOREABLE) > 0 && // only bother scanning restorable partitions
                has_volume(device_partitions[i].path)) { // make sure it exists
            if(get_nandroid_type_for_file(dir_path, device_partitions[i].path) != NANDROID_TYPE_NULL) {
                partitions[j++] = device_partitions[i].id;
            }
        }
    }
    partitions[j] = INT_MAX;

    if(j == 0) { // didn't find anything
        free(partitions);
        return NULL;
    }

    nandroid* n = (nandroid*)malloc(sizeof(nandroid));
    n->dir = strdup(dir_path);
    n->partitions = partitions;
    n->md5 = 0; // assume no md5 present

    // check if an md5 file exists
    DIR* dir = opendir(dir_path);
    if (dir != NULL) {
        struct dirent* de;
        while ((de = readdir(dir)) != NULL) {
            int ext_len = strlen(NANDROID_MD5_EXT);
            int dname_len = strlen(de->d_name);
            if(dname_len > ext_len && strcmp(de->d_name + (dname_len - ext_len), NANDROID_MD5_EXT) == 0) {
                n->md5 = 1;
                break;
            }
        }

        closedir(dir);
    }

    return n;
}

// our nandroid directories to scan
static char* nandroid_dirs[] = {
            "/sdcard/nandroid",
            "/sdcard/clockworkmod/backup"
        };
static int nandroid_dir_num = sizeof(nandroid_dirs) / sizeof(nandroid_dirs[0]);

// return a list of all available nandroids
nandroid** nandroid_scan() {
    // count our nandroids
    int count = 0;
    int i;
    for(i = 0; i < nandroid_dir_num; ++i) {
        ensure_path_mounted(nandroid_dirs[i]);

        // open the directory
        DIR* dir = opendir(nandroid_dirs[i]);
        if (dir != NULL) {
            struct dirent* de;
        	while ((de = readdir(dir)) != NULL) {
                // assume directories reference nandroids
                if(de->d_name != '.' && de->d_type == DT_DIR) {
                    ++count;
                }
            }

            closedir(dir);
        }
    }

    // now we create our array
    nandroid** nandroids = (nandroid**)calloc(count + 1, sizeof(nandroid*));
    int j = 0;
    for(i = 0; i < nandroid_dir_num; ++i) {
        ensure_path_mounted(nandroid_dirs[i]);

        // open the directory
        DIR* dir = opendir(nandroid_dirs[i]);
        if (dir != NULL) {
            struct dirent* de;

        	while ((de = readdir(dir)) != NULL) {
                // assume directories reference nandroids
                if(de->d_name[0] != '.' && de->d_type == DT_DIR) {
                    int buflen = strlen(nandroid_dirs[i]) + strlen(de->d_name) + 2;
                    char* buf = (char*)calloc(buflen, sizeof(char));
                    snprintf(buf, buflen, "%s/%s", nandroid_dirs[i], de->d_name);
                    nandroid* n = nandroid_scan_dir(buf);
                    free(buf);
                    if(n != NULL) {
                        nandroids[j++] = n;
                    }
                }
            }

            closedir(dir);
        }
    }

    nandroids[j] = NULL;
    return nandroids;
}

// destroy a list of nandroids retreived through nandroid_scan()
void destroy_nandroid_list(nandroid** list) {
    nandroid** p = list;

    while(*p) {
        nandroid* n = *p;

        free(n->dir);
        free(n->partitions);
        free(n);

        p++;
    }

    free(list);
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
        ui_print("Skipping backup of %s ... (volume not found)\n", path);
        return -1;
    }

    char* backup_path = get_nandroid_path_for_path(backup_dir, path);
    if(backup_path == NULL) {
        ui_print("Skipping backup of %s ... (backup path could not be generated)\n", path);
        return -1;
    }

    int ret;
    int type = get_nandroid_type_for_path(path);

    int was_bp_mounted = is_path_mounted(backup_dir);
    int was_p_mounted;
    ensure_path_mounted(backup_dir);

    switch(type) {
    case NANDROID_TYPE_RAW:
        ret = nandroid_raw_backup_path(v->device, backup_path);
        break;
    default:
        was_p_mounted = is_path_mounted(path);
        ensure_path_mounted(path);

        switch(type) {
        case NANDROID_TYPE_TAR_GZ:
            ret = nandroid_tgz_backup_path(path, backup_path);
            break;
        }

        // if the path was unmounted originally, unmount it again
        if(!was_p_mounted) {
            ensure_path_unmounted(path);
        }
    }

    if(!was_bp_mounted) {
        ensure_path_unmounted(backup_dir);
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
    int type = get_nandroid_type_for_file(backup_dir, path);
    if(type == NANDROID_TYPE_NULL) {
        ui_print("Skipping restore of %s ... (no backup found)\n", path);
        return -1;
    }

    char* backup_path = get_nandroid_path_for_path(backup_dir, path);
    if(backup_path == NULL) {
        ui_print("Skipping restore of %s ... (backup path not found)\n", path);
        return ret;
    }

    struct stat file_info;
    if (0 != (ret = statfs(backup_path, &file_info))) {
        ui_print("Skipping restore of %s ... (%s not found)\n", path, backup_path);
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

