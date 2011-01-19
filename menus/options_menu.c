#include <stdlib.h>

#include "recovery_config.h"
#include "recovery_menu.h"
#include "recovery_ui.h"

#include "nandroid/nandroid.h"

#define ITEM_OPTIONS_MD5_VERIFICATION       1
#define ITEM_OPTIONS_SIGNATURE_VERIFICATION 2
#define ITEM_OPTIONS_NANDROID_TYPE          3
#define ITEM_OPTIONS_COLORS                 4

#define ITEM_OPTIONS_COLORS_RED   1
#define ITEM_OPTIONS_COLORS_GREEN 2
#define ITEM_OPTIONS_COLORS_BLUE  3

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
    items[j++] = create_menu_item(ITEM_OPTIONS_NANDROID_TYPE, "Select default nandroid format");
    items[j++] = create_menu_item(ITEM_OPTIONS_COLORS, "Select color theme");
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
    } else if(chosen_item == ITEM_OPTIONS_NANDROID_TYPE) {
        int j = 0;
        recovery_menu_item** items = (recovery_menu_item**)calloc(NANDROID_TYPE_LAST + 1, sizeof(recovery_menu_item*));
        items[j++] = create_menu_item(NANDROID_TYPE_TAR,      "TAR (SPRecovery)");
        items[j++] = create_menu_item(NANDROID_TYPE_TAR_GZ,   "TAR + GZip Compression");
        items[j++] = create_menu_item(NANDROID_TYPE_TAR_BZ2,  "TAR + BZip2 Compression");
        items[j++] = create_menu_item(NANDROID_TYPE_TAR_LZMA, "TAR + LZMA Compression");
        items[j++] = create_menu_item(NANDROID_TYPE_YAFFS,    "YAFFS (ClockworkMod)");
        items[j++] = NULL;

        int selection = display_item_select_menu(items, rconfig->nandroid_type);

        // only use non raw, valid nandroid types
        if(selection > 0 && selection <= NANDROID_TYPE_LAST) {
            rconfig->nandroid_type = selection;
        }
    } else if(chosen_item == ITEM_OPTIONS_COLORS) {
        int j = 0;
        recovery_menu_item** items = (recovery_menu_item**)calloc(4, sizeof(recovery_menu_item*));
        items[j++] = create_menu_item(ITEM_OPTIONS_COLORS_RED,   "Red Theme");
        items[j++] = create_menu_item(ITEM_OPTIONS_COLORS_GREEN, "Green Theme");
        items[j++] = create_menu_item(ITEM_OPTIONS_COLORS_BLUE,  "Blue Theme");
        items[j++] = NULL;

        int selection = display_item_select_menu(items,
                (rconfig->color_menu.r > 128 ?
                        ITEM_OPTIONS_COLORS_RED :
                        (rconfig->color_menu.g > 128 ?
                                ITEM_OPTIONS_COLORS_GREEN :
                                (rconfig->color_menu.b > 128 ?
                                        ITEM_OPTIONS_COLORS_BLUE : 0 ))));

        switch(selection) {
        case ITEM_OPTIONS_COLORS_RED:
            // menu default = red
            rconfig->color_menu.r = 238;
            rconfig->color_menu.g =   0;
            rconfig->color_menu.b =   0;
            rconfig->color_menu.a = 255;
            // menu text default = red
            rconfig->color_menu_text.r = 238;
            rconfig->color_menu_text.g =   0;
            rconfig->color_menu_text.b =   0;
            rconfig->color_menu_text.a = 255;
            break;
        case ITEM_OPTIONS_COLORS_GREEN:
            // menu default = green
            rconfig->color_menu.r =   0;
            rconfig->color_menu.g = 187;
            rconfig->color_menu.b =   0;
            rconfig->color_menu.a = 255;
            // menu text default = green
            rconfig->color_menu_text.r =   0;
            rconfig->color_menu_text.g = 187;
            rconfig->color_menu_text.b =   0;
            rconfig->color_menu_text.a = 255;
            break;
        case ITEM_OPTIONS_COLORS_BLUE:
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
            break;
        }
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
