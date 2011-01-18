#include "recovery_config.h"

#include <stdio.h>
#include <stdlib.h>

#include "roots.h"
#include "nandroid/nandroid.h"

#define RECOVERY_CONFIG_FILE "/data/rc_data"

static recovery_config* rconfig = NULL;

void load_config() {
    // if this already exists, we need to wipe it out so we can reload
    if(rconfig) {
        free(rconfig);
        rconfig = NULL;
    }

    int load_defaults = 1;

    // generate an empty config
    int size = sizeof(recovery_config);
    rconfig = (recovery_config*)malloc(size);

    // ensure our path is mounted
    int was_mounted = is_path_mounted(RECOVERY_CONFIG_FILE);
    if(0 != ensure_path_mounted(RECOVERY_CONFIG_FILE)) {
        LOGW("Error mounting config path.  Config will load defaults.\n");
    } else {
        FILE *file = fopen(RECOVERY_CONFIG_FILE, "rb"); /* read from the file in binary mode */
        if(file != NULL) {
            // if we get a valid size, we don't need to set our data to defaults
            if(1 == fread(rconfig, size, 1, file)) {
                load_defaults = 0;
            } else {
                LOGE("Invalid config data found.  Config will load defaults.\n");
            }

            fclose(file);
        } else {
            LOGW("Error opening config file.  Config will load defaults.\n");
        }

        if(was_mounted) {
            ensure_path_unmounted(RECOVERY_CONFIG_FILE);
        }
    }

    // did something go wrong so we need to load defaults?
    if(load_defaults) {
        // normal text default = white
        rconfig->color_normal_text.r = 255;
        rconfig->color_normal_text.g = 255;
        rconfig->color_normal_text.b = 255;
        rconfig->color_normal_text.a = 255;
        // header text default = white
        rconfig->color_header_text.r = 255;
        rconfig->color_header_text.g = 255;
        rconfig->color_header_text.b = 255;
        rconfig->color_header_text.a = 255;
        // menu default = blue
        rconfig->color_menu.r =  54;
        rconfig->color_menu.g =  74;
        rconfig->color_menu.b = 255;
        rconfig->color_menu.a = 255;
        // menu text default = blue
        rconfig->color_menu_text.r =  54;
        rconfig->color_menu_text.g =  74;
        rconfig->color_menu_text.b = 255;
        rconfig->color_menu_text.a = 255;
        // menu selected text default = white
        rconfig->color_menu_selected_text.r = 255;
        rconfig->color_menu_selected_text.g = 255;
        rconfig->color_menu_selected_text.b = 255;
        rconfig->color_menu_selected_text.a = 255;
        // default to checking MD5's on nandroid
        rconfig->nandroid_do_md5_verification = 1;
        // default to not checking signatures on installation
        rconfig->install_do_signature_verification = 0;
        // tar will be our default nandroid type
        rconfig->nandroid_type = NANDROID_TYPE_TAR;
    }
}

void save_config() {
    // only save if it exists
    if(rconfig) {
        int was_mounted = is_path_mounted(RECOVERY_CONFIG_FILE);

        // ensure root mounted
        if(0 != ensure_path_mounted(RECOVERY_CONFIG_FILE)) {
            LOGE("Error mounting config file path!  Config settings won't be saved.\n");
        } else {
            FILE *file = fopen(RECOVERY_CONFIG_FILE, "wb"); /* write to the file in binary mode */
            if(file != NULL) {
                fwrite(rconfig, sizeof(recovery_config), 1, file);
                fclose(file);
            } else {
                LOGE("Error opening config file for writing!  Config settings won't be saved.\n");
            }

            if(was_mounted) {
                ensure_path_unmounted(RECOVERY_CONFIG_FILE);
            }
        }
    }
}

recovery_config* get_config() {
    return rconfig;
}
