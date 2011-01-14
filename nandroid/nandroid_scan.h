/**
 * \file nandroid_scan.h
 *
 * This file defines functions that scan/detect information about nandroids
 */

#ifndef RECOVERY_NANDROID_SCAN_H_
#define RECOVERY_NANDROID_SCAN_H_

#include "nandroid/nandroid.h"

// scanning info
int get_nandroid_type_for_new(char* path);
int get_nandroid_type_for_existing(char* backup_dir, char* path);
char* get_nandroid_file_for_path(char* backup_dir, char* path);

// scans for available nandroids
nandroid** nandroid_scan();
void destroy_nandroid_list(nandroid** list);

#endif//RECOVERY_NANDROID_SCAN_H_
