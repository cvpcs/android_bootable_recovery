#ifndef RECOVERY_NANDROID_H_
#define RECOVERY_NANDROID_H_

typedef struct {
    // directory of the nandroid
    char* dir;
    // list of partitions by id
    // end of the list is indicated by INT_MAX
    int* partitions;
    int md5;
} nandroid;

// scans for available nandroids
nandroid** nandroid_scan();
void destroy_nandroid_list(nandroid** list);

#endif//RECOVERY_NANDROID_H_
