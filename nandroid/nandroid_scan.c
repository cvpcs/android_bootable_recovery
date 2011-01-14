#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

#include "recovery_config.h"
#include "roots.h"
#include "recovery_lib.h"

#include "nandroid/nandroid.h"

// extensions for the various nandroid types
#define NANDROID_TYPE_RAW_EXT      ".img"
#define NANDROID_TYPE_TAR_EXT      ".tar"
#define NANDROID_TYPE_TAR_GZ_EXT   ".tar.gz"
#define NANDROID_TYPE_TAR_BZ2_EXT  ".tar.bz2"
#define NANDROID_TYPE_TAR_LZMA_EXT ".tar.lzma"
#define NANDROID_TYPE_YAFFS_EXT    ".img"

#define NANDROID_MD5_EXT ".md5"

// our nandroid directories to scan
static char* nandroid_dirs[] = {
            "/sdcard/nandroid",
            "/sdcard/clockworkmod/backup"
        };
static int nandroid_dir_num = sizeof(nandroid_dirs) / sizeof(nandroid_dirs[0]);

/**
 * Returns the default nandroid type as defined by the recovery config
 *
 * \return A valid NANDROID_TYPE between 0 and NANDROID_TYPE_LAST (inclusive) 
 */
int get_default_nandroid_type() {
    recovery_config* rconfig = get_config();

    // ensure our config is valid
    if(rconfig && rconfig->nandroid_type >= 0 && rconfig->nandroid_type <= NANDROID_TYPE_LAST) {
        return rconfig->nandroid_type;
    }

    // default if we have an invalid config
    return NANDROID_TYPE_TAR;
}

/**
 * Returns the appropriate type of nandroid that should be performed on a new
 * path.
 *
 * \param path The root path (e.g. /boot) that you are checking for
 *
 * \returns A valid NANDROID_TYPE between 0 and NANDROID_TYPE_LAST (inclusive) if
 *          path is a valid volume.  Returns NANDROID_TYPE_NULL otherwise.
 */
int get_nandroid_type_for_new(char* path) {
    Volume* v = volume_for_path(path);
    if(v == NULL)
        return NANDROID_TYPE_NULL;

    if(strcmp(v->fs_type, "mtd") == 0) {
        return NANDROID_TYPE_RAW;
    } else {
        return get_default_nandroid_type();
    }
}

/**
 * Returns the type of nandroid that was used to create an existing nandroid
 *
 * \param backup_dir The directory where the nandroid resides
 * \param path The root path (e.g. /boot) that you are checking for
 *
 * \returns A valid NANDROID_TYPE between 0 and NANDROID_TYPE_LAST (inclusive) if
 *          the nandroid backup is found, or NANDROID_TYPE_NULL if the backup doesn't
 *          exist
 */
int get_nandroid_type_for_existing(char* backup_dir, char* path) {
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

/**
 * Returns the appropriate file name for the given backup directory and path.
 * This function will first check for an existing backup, and if it exists, return
 * the appropriate file name based on the detected type. If the backup is not found
 * it will return the appropriate filename for a new backup.
 *
 * \param backup_dir The nandroid backup directory you are referencing
 * \param path The root path (e.g. /boot) that you intend to backup/restore
 *
 * \returns The appropriate backup filename.
 */
char* get_nandroid_file_for_path(char* backup_dir, char* path) {
    Volume* v = volume_for_path(path);
    if(v == NULL)
        return NULL;

    // first see if we're getting an existing path
    int type = get_nandroid_type_for_existing(backup_dir, path);

    if(type == NANDROID_TYPE_NULL) {
        type = get_nandroid_type_for_new(path);
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

/**
 * Scan a given directory and determine if it is a nandroid backup or not.  If it
 * is, then return a nandroid structure with the appropriate information.
 *
 * \param dir_path The directory path to scan for nandroids
 *
 * \return A valid nandroid structure if the directory is a nandroid, or NULL if it
 *         is not.
 */
nandroid* nandroid_scan_dir(char* dir_path) {
    // make an int array that we KNOW will be big enough
    int* partitions = (int*)calloc(device_partition_num + 1, sizeof(int));

    int i;
    int j = 0;
    for(i = 0; i < device_partition_num; ++i) {
        if((device_partitions[i].flags & PARTITION_FLAG_RESTOREABLE) > 0 && // only bother scanning restorable partitions
                has_volume(device_partitions[i].path) && // make sure the path exists on this device
                get_nandroid_type_for_existing(dir_path, device_partitions[i].path) != NANDROID_TYPE_NULL) { // make sure the backup file exists
            partitions[j++] = device_partitions[i].id;
        }
    }
    partitions[j] = INT_MAX; // INT_MAX is our indication of the end of the list

    if(j == 0) { // didn't find anything
        free(partitions);
        return NULL;
    }

    // found something, time to create our nandroid structure
    nandroid* n = (nandroid*)malloc(sizeof(nandroid));
    n->dir = strdup(dir_path);
    n->partitions = partitions;

    // return it
    return n;
}

/**
 * Scan the defined nandroid directories, returning a list of all nandroids that
 * were found.
 *
 * \return A null-terminated list of nandroids that were found.  This should later
 *         be destroyed with a call to destroy_nandroid_list()
 */
nandroid** nandroid_scan() {
    // first make sure our nandroid directories are mounted.  we don't bother
    // exiting of they aren't because the rest of this will just return 0 nandroids
    // anyway
    int i;
    for(i = 0; i < nandroid_dir_num; ++i) {
        ensure_path_mounted(nandroid_dirs[i]);
    }

    // count our nandroids.  here we assume that every subdirectory within our
    // nandroid directories are nandroids, to ensure that our list will be big
    // enough.
    int count = 0;
    for(i = 0; i < nandroid_dir_num; ++i) {
        // open the directory
        DIR* dir = opendir(nandroid_dirs[i]);
        if (dir != NULL) {
            struct dirent* de;
        	while ((de = readdir(dir)) != NULL) {
                // assume directories reference nandroids
                if(de->d_name[0] != '.' && de->d_type == DT_DIR) {
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
        // open the directory
        DIR* dir = opendir(nandroid_dirs[i]);
        if (dir != NULL) {
            struct dirent* de;

        	while ((de = readdir(dir)) != NULL) {
                // assume directories reference nandroids
                if(de->d_name[0] != '.' && de->d_type == DT_DIR) {
                    // create our directory string and scan for a nandroid
                    int buflen = strlen(nandroid_dirs[i]) + strlen(de->d_name) + 2;
                    char* buf = (char*)calloc(buflen, sizeof(char));
                    snprintf(buf, buflen, "%s/%s", nandroid_dirs[i], de->d_name);
                    nandroid* n = nandroid_scan_dir(buf);
                    free(buf);

                    // if it's not null (valid nandroid), then add it to the list
                    if(n != NULL) {
                        nandroids[j++] = n;
                    }
                }
            }

            closedir(dir);
        }
    }
    // end the list and return it
    nandroids[j] = NULL;
    return nandroids;
}

/**
 * Destroy a list of nandroids (free from memory)
 *
 * \param list A list of nandroids retrieved through the use of nandroid_scan()
 */
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
