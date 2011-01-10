#ifndef RECOVERY_INSTALL_MENU_H_
#define RECOVERY_INSTALL_MENU_H_

void show_install_menu();

int install_rom_from_tar(char* filename);
int install_rom_from_zip(char* filename);

#endif//RECOVERY_INSTALL_MENU_H_
