#ifndef RECOVERY_RECOVERY_CONFIG_H_
#define RECOVERY_RECOVERY_CONFIG_H_

typedef struct {
    char r;
    char g;
    char b;
    char a;
} recovery_config_color;

typedef struct {
    // color configs
    recovery_config_color color_normal_text;
    recovery_config_color color_header_text;
    recovery_config_color color_menu;
    recovery_config_color color_menu_text;
    recovery_config_color color_menu_selected_text;

    // verification configs
    int nandroid_do_md5_verification;
    int install_do_signature_verification;
} recovery_config;

// load and save the config
void load_config();
void save_config();

// get a pointer to the config
recovery_config* get_config();

#endif//RECOVERY_CONFIG_H_
