#ifndef RECOVERY_NANDROID_MENU_H_
#define RECOVERY_NANDROID_MENU_H_

#define BOOT     0x01
#define CACHE    0x02
#define DATA     0x04
#define MISC     0x08
#define RECOVERY 0x10
#define SYSTEM   0x20
#define PROGRESS 0x40

#define BSD      0x25

void nandroid_backup(char* subname, char partitions);

void show_nandroid_menu();

#endif//RECOVERY_NANDROID_MENU_H_
