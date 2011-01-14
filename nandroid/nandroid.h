#ifndef RECOVERY_NANDROID_H_
#define RECOVERY_NANDROID_H_

// nandroid types
#define NANDROID_TYPE_NULL    -1 // null if shouldn't be nandroided
#define NANDROID_TYPE_RAW      0 // raw MTD image
#define NANDROID_TYPE_TAR      1 // tar file
#define NANDROID_TYPE_TAR_GZ   2 // tar.gz file
#define NANDROID_TYPE_TAR_BZ2  3 // tar.bz2 file 
#define NANDROID_TYPE_TAR_LZMA 4 // tar.lzma file (not supported yet)
#define NANDROID_TYPE_YAFFS    5 // yaffs image (fastboot/cwm)
#define NANDROID_TYPE_LAST     NANDROID_TYPE_YAFFS // used to determine validity of types

typedef struct {
    // directory of the nandroid
    char* dir;
    // list of partitions by id
    // end of the list is indicated by INT_MAX
    int* partitions;
} nandroid;

void nandroid_backup(char* backup_dir, int* partitions);
void nandroid_restore(char* backup_dir, int* partitions);

#endif//RECOVERY_NANDROID_H_
