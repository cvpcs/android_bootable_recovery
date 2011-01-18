#include <stdlib.h>

#include "recovery_config.h"
#include "recovery_menu.h"
#include "recovery_ui.h"

#define ITEM_OPTIONS_MD5_VERIFICATION       1
#define ITEM_OPTIONS_SIGNATURE_VERIFICATION 2
#define ITEM_OPTIONS_NANDROID_TYPE          3
#define ITEM_OPTIONS_COLORS                 4

recovery_menu_item** options_menu_create_items(void* data) {
    recovery_config* rconfig = get_config();
    if(!rconfig) {
        ui_print("Error reading recovery configuration.\n");
        // fallback if config errors out for some reason
        recovery_menu_item** items = (recovery_menu_item**)calloc(2, sizeof(recovery_menu_item*));
        items[0] = create_menu_item(ITEM_BACK, "Error reading recovery configuration");
        items[1] = NULL;
        return items;
    }

    int j = 0;
    recovery_menu_item** items = (recovery_menu_item**)calloc(5, sizeof(recovery_menu_item*));
    items[j++] = create_menu_item_checkbox(ITEM_OPTIONS_MD5_VERIFICATION, "Verify Nandroid MD5 Checksums", rconfig->nandroid_do_md5_verification);
    items[j++] = create_menu_item_checkbox(ITEM_OPTIONS_SIGNATURE_VERIFICATION, "Verify Update.zip Signatures", rconfig->install_do_signature_verification);
    items[j++] = NULL;

    return items;
}

int options_menu_select(int chosen_item, void* data) {
    recovery_config* rconfig = get_config();
    if(!rconfig) {
        // whoops, bail out
        ui_print("Error reading recovery configuration.\n");
        return chosen_item;
    }

    if(chosen_item == ITEM_OPTIONS_MD5_VERIFICATION) {
        rconfig->nandroid_do_md5_verification = !rconfig->nandroid_do_md5_verification;
    } else if(chosen_item == ITEM_OPTIONS_SIGNATURE_VERIFICATION) {
        rconfig->install_do_signature_verification = !rconfig->install_do_signature_verification;
    }

    return chosen_item;
}

void show_options_menu()
{
    char* headers[] = { "Select an option to change its value",
                        "", NULL };

    recovery_menu* menu = create_menu(
            headers,
            /* no items */ NULL,
            /* no data */ NULL,
            /* no on_create */ NULL,
            &options_menu_create_items,
            &options_menu_select,
            /* no on_destroy */ NULL);
    display_menu(menu);
    destroy_menu(menu);
}
