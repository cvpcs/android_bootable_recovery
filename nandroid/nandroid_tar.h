#ifndef RECOVERY_NANDROID_TAR_H_
#define RECOVERY_NANDROID_TAR_H_

int nandroid_backup_path_tar(char* path, char* backup_path);
int nandroid_restore_path_tar(char* path, char* backup_path);

int nandroid_backup_path_tar_gz(char* path, char* backup_path);
int nandroid_restore_path_tar_gz(char* path, char* backup_path);

int nandroid_backup_path_tar_bz2(char* path, char* backup_path);
int nandroid_restore_path_tar_bz2(char* path, char* backup_path);

int nandroid_backup_path_tar_lzma(char* path, char* backup_path);
int nandroid_restore_path_tar_lzma(char* path, char* backup_path);

#endif//RECOVERY_NANDROID_TAR_H_
