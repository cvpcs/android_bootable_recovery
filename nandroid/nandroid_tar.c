#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "roots.h"

#include "nandroid/nandroid.h"

extern int __system(const char* cmd);

int nandroid_backup_path_tar_ext(char* path, char* backup_path, const char* tar_cmd_format) {
    int was_mounted = is_path_mounted(path);
    int ret = 0;

    if(0 != (ret = ensure_path_mounted(path))) {
        return ret;
    }

    int cmd_len = strlen(tar_cmd_format) + strlen(path) + strlen(backup_path);
    char* cmd = (char*)calloc(cmd_len, sizeof(char));

    snprintf(cmd, cmd_len, tar_cmd_format, backup_path, path);
    ret = __system(cmd);

    free(cmd);

    if(!was_mounted)
        ensure_path_unmounted(path);

    return ret;
}

int nandroid_restore_path_tar_ext(char* path, char* backup_path, const char* tar_cmd_format) {
    int was_mounted = is_path_mounted(path);
    int ret = 0;

    if(0 != (ret = ensure_path_mounted(path))) {
        return ret;
    }

    int cmd_len = strlen(tar_cmd_format) + strlen(path) + strlen(backup_path);
    char* cmd = (char*)calloc(cmd_len, sizeof(char));

    snprintf(cmd, cmd_len, tar_cmd_format, backup_path, path);
    ret = __system(cmd);

    free(cmd);

    if(!was_mounted)
        ensure_path_unmounted(path);

    return ret;
}

int nandroid_backup_path_tar(char* path, char* backup_path)  { return nandroid_backup_path_tar_ext(path, backup_path, "tar -cf %s . -C %s");  }
int nandroid_restore_path_tar(char* path, char* backup_path) { return nandroid_restore_path_tar_ext(path, backup_path, "tar -xf %s . -C %s"); }

int nandroid_backup_path_tar_gz(char* path, char* backup_path)  { return nandroid_backup_path_tar_ext(path, backup_path, "tar -zcf %s . -C %s");  }
int nandroid_restore_path_tar_gz(char* path, char* backup_path) { return nandroid_restore_path_tar_ext(path, backup_path, "tar -zxf %s . -C %s"); }

int nandroid_backup_path_tar_bz2(char* path, char* backup_path)  { return nandroid_backup_path_tar_ext(path, backup_path, "tar -jcf %s . -C %s");  }
int nandroid_restore_path_tar_bz2(char* path, char* backup_path) { return nandroid_restore_path_tar_ext(path, backup_path, "tar -jxf %s . -C %s"); }

int nandroid_backup_path_tar_lzma(char* path, char* backup_path) {
    const char* tar_cmd_format = "tar -c . -C %s | lzma -cz - > %s";

    int was_mounted = is_path_mounted(path);
    int ret = 0;

    if(0 != (ret = ensure_path_mounted(path))) {
        return ret;
    }

    int cmd_len = strlen(tar_cmd_format) + strlen(path) + strlen(backup_path);
    char* cmd = (char*)calloc(cmd_len, sizeof(char));

    snprintf(cmd, cmd_len, tar_cmd_format, path, backup_path);
    ret = __system(cmd);

    free(cmd);

    if(!was_mounted)
        ensure_path_unmounted(path);

    return ret;
}

int nandroid_restore_path_tar_lzma(char* path, char* backup_path) {
    const char* tar_cmd_format = "lzma -cd %s | tar -xf - -C %s";

    int was_mounted = is_path_mounted(path);
    int ret = 0;

    if(0 != (ret = ensure_path_mounted(path))) {
        return ret;
    }

    int cmd_len = strlen(tar_cmd_format) + strlen(path) + strlen(backup_path);
    char* cmd = (char*)calloc(cmd_len, sizeof(char));

    snprintf(cmd, cmd_len, tar_cmd_format, backup_path, path);
    ret = __system(cmd);

    free(cmd);

    if(!was_mounted)
        ensure_path_unmounted(path);

    return ret;
}

