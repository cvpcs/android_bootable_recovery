#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/limits.h>
#include <linux/input.h>

#include "common.h"
#include "recovery_menu.h"
#include "recovery_lib.h"
#include "minui/minui.h"
#include "recovery_ui.h"
#include "roots.h"
#include "install.h"
#include <string.h>

#include "nandroid_menu.h"

void install_rom_from_tar(char* filename)
{
    if (ui_key_pressed(KEY_SPACE)) {
        ui_print("Backing up before installing...\n");
        nandroid_backup("preinstall",BSD|PROGRESS);
    }

    ui_print("Attempting to install ROM from ");
    ui_print(filename);
    ui_print("...\n");

    char* argv[] = { "/sbin/nandroid-mobile.sh",
                     "--install-rom",
                     filename,
                     "--progress",
                     NULL };

    char* envp[] = { NULL };

    int status = runve("/sbin/nandroid-mobile.sh",argv,envp,1);
    if(!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
        ui_printf_int("ERROR: install exited with status %d\n",WEXITSTATUS(status));
        return WEXITSTATUS(status);
    } else {
        ui_print("(done)\n");
    }
    ui_reset_progress();
    return 0;
}

void install_update_zip(char* filename) {
    ui_print("\n-- Install update.zip from sdcard...\n");
    set_sdcard_update_bootloader_message();
    ui_print("Attempting update from...\n");
    ui_print(filename);
    ui_print("\n");
    int status = install_package(filename);
    if (status != INSTALL_SUCCESS) {
        ui_set_background(BACKGROUND_ICON_ERROR);
        ui_print("Installation aborted.\n");
    } else if (!ui_text_visible()) {
        return;
    } else {
        ui_print("\nInstall from sdcard complete.\n");
        ui_print("\nThanks for using RZrecovery.\n");
    }
    return;
}

void install_kernel_img(char* filename) {
    ui_print("\n-- Install kernel img from...\n");
    ui_print(filename);
    ui_print("\n");
    ensure_path_mounted(filename);

    char* argv[] = { "/sbin/flash_image",
                     "boot",
                     filename,
                     NULL };

    char* envp[] = { NULL };

    int status = runve("/sbin/flash_image",argv,envp,1);

    ui_print("\nKernel flash from sdcard complete.\n");
    ui_print("\nThanks for using RZrecovery.\n");
    return 0;
}

void install_recovery_img(char* filename) {
    ui_print("\n-- Install recovery img from...\n");
    ui_print(filename);
    ui_print("\n");
    ensure_path_mounted(filename);

    char* argv[] = { "/sbin/flash_image",
                     "recovery",
                     filename,
                     NULL };

    char* envp[] = { NULL };

    int status = runve("/sbin/flash_image",argv,envp,1);

    ui_print("\nRecovery flash from sdcard complete.\n");
    ui_print("\nThanks for using RZrecovery.\n");
    return 0;
}

#define INSTALL_ITEM_TAR      0
#define INSTALL_ITEM_ZIP      1
#define INSTALL_ITEM_KERNEL   2
#define INSTALL_ITEM_RECOVERY 3

int install_menu_select(int chosen_item, void* data) {
    static char* zip_exts[]      = { ".zip" };
    static char* tar_exts[]      = { ".rom.tgz", ".rom.tar.gz", ".rom.tar" };
    static char* kernel_exts[]   = { "boot.img" };
    static char* recovery_exts[] = { "-rec.img", "_rec.img", ".rec.img" };
	switch(chosen_item) {
	case INSTALL_ITEM_ZIP:
        display_file_select_menu("/sdcard", zip_exts, &install_update_zip);
	    break;
	case INSTALL_ITEM_TAR:
        display_file_select_menu("/sdcard", tar_exts, &install_rom_from_tar);
	    break;
	case INSTALL_ITEM_KERNEL:
        display_file_select_menu("/sdcard/kernel", kernel_exts, &install_kernel_img);
		break;
	case INSTALL_ITEM_RECOVERY:
		display_file_select_menu("/sdcard/recovery", recovery_exts, &install_recovery_img);
		break;
	}

    return chosen_item;
}

void show_install_menu()
{
    char* headers[] = { "Choose an install option or press",
                        "POWER to exit",
                        "", NULL };
    recovery_menu_item items[] = {
                { INSTALL_ITEM_TAR,      "Install ROM tar from SD card" },
                { INSTALL_ITEM_ZIP,      "Install update.zip from SD card" },
                { INSTALL_ITEM_KERNEL,   "Install kernel img from /sdcard/kernels" },
                { INSTALL_ITEM_RECOVERY, "Install recovery img from /sdcard/recovery" },
                NULL
            };
    recovery_menu* menu = create_menu(
            headers,
            items,
            /* no data */ NULL,
            /* no on_create */ NULL,
            /* no on_create_menu_items */ NULL,
            &install_menu_select,
            /* no on_destroy */ NULL);
    display_menu(menu);
    destroy_menu(menu);
}
