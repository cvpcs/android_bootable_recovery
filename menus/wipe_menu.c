#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "recovery_lib.h"
#include "roots.h"
#include "recovery_ui.h"
#include "recovery_menu.h"

void wipe_partition(int which, int confirm) {
    if(which >= 0) {
        if(which >= device_partition_num || // make sure it's a partition
                device_partitions[which].id != which || // make sure our "which" is pointing correctly
                (device_partitions[which].flags & PARTITION_FLAG_WIPEABLE) == 0 || // make sure we're wipeable
                !has_volume(device_partitions[which].path)) { // make sure we exist
            // not valid
            LOGE("Invalid device specified in wipe");
            return;
        }
    }

ui_print("Planning on wiping %s", device_partitions[which].path);

    // we should be good to go now

    if (confirm) {
        char** title_headers = NULL;

        char* confirm;
        char* confirm_select;

        // if we have a specific one, then use it
        const char* confirm_format = "Confirm wipe of %s?";
        const char* confirm_select_format = " Yes -- wipe %s";
        if(which >= 0) {
            int clen  = strlen(device_partitions[which].name) + strlen(confirm_format);
            int cslen = strlen(device_partitions[which].name) + strlen(confirm_select_format);
            confirm        = (char*)calloc(clen,  sizeof(char));
            confirm_select = (char*)calloc(cslen, sizeof(char));
            snprintf(confirm,        clen,  confirm_format,        device_partitions[which].name);
            snprintf(confirm_select, cslen, confirm_select_format, device_partitions[which].name);
        } else {
            // we are wiping all
            confirm = strdup("Confirm wipe of EVERYTHING?");
            confirm_select = strdup(" Yes -- wipe EVERYTHING");
        }

        char* headers[] = { confirm,
                            "THIS CAN NOT BE UNDONE.",
                            "", NULL };
        title_headers = prepend_title(headers);

        

        char* items[] = { " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          confirm_select,   // [7]
                          " No",
                          " No",
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        free(confirm);
        free(confirm_select);
        if (chosen_item != 7) {
            return;
        }
    }

    const char* wiping_format = "---- Wiping %s ----\n";
    const char* wiping_complete = "     complete\n";

    if(which >= 0) {
        ui_print(wiping_format, device_partitions[which].name);
        erase_volume(device_partitions[which].path);
        ui_print("     complete\n");
        if(device_partitions[which].id == PARTITION_DATA && // if on data
                device_partitions[PARTITION_DATADATA].id == PARTITION_DATADATA && // sanity check
                (device_partitions[PARTITION_DATADATA].flags & PARTITION_FLAG_WIPEABLE) > 0 && // make sure it's wipeable
                has_volume(device_partitions[PARTITION_DATADATA].path)) { // datadata exists
            ui_print(wiping_format, device_partitions[PARTITION_DATADATA].name);
            erase_volume(device_partitions[PARTITION_DATADATA].path);
            ui_print(wiping_complete);
        }
    } else {
        int i;
        for(i = 0; i < device_partition_num; ++i) {
            if((device_partitions[i].flags & PARTITION_FLAG_WIPEABLE) > 0 &&
                    has_volume(device_partitions[i].path)) {
            ui_print(wiping_format, device_partitions[i].name);
            erase_volume(device_partitions[i].path);
            ui_print("     complete\n");
            } 
        }
    }
}

void wipe_batts(int confirm) {
    if (confirm) {
        char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "Confirm wipe of battery stats?",
                                "THIS CAN NOT BE UNDONE.",
                                "",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          " Yes -- Wipe battery stats",   // [7]
                          " No",
                          " No",
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 7) {
            return;
        }
    }

    ui_print("\n-- Wiping battery stats...\n");
    ensure_path_mounted("/data");
    remove("/data/system/batterystats.bin");
    ui_print("\n Battery Statistics cleared.\n");
}

#define ITEM_WIPE_ALL  (PARTITION_LAST + 1)
#define ITEM_WIPE_BATT (PARTITION_LAST + 2)

int wipe_menu_select(int chosen_item, void* data) {
    if(chosen_item == ITEM_WIPE_ALL) {
        wipe_partition(-1, ui_text_visible());
    } else if(chosen_item == ITEM_WIPE_BATT) {
        wipe_batts(ui_text_visible());
    } else if(chosen_item < device_partition_num) {
        // we try to wipe our chosen item, which should be a partition id
        wipe_partition(chosen_item, ui_text_visible());
    }

    return chosen_item;
}

void show_wipe_menu()
{
    int i;
    int count = 0;
    for(i = 0; i < device_partition_num; ++i) {
        // silently ignore datadata, it will get wiped with DATA if it exists
        if(device_partitions[i].id == PARTITION_DATADATA)
            continue;

        if((device_partitions[i].flags & PARTITION_FLAG_WIPEABLE) > 0 &&
                has_volume(device_partitions[i].path)) {
            count++;
        }
    }

    recovery_menu_item** items = (recovery_menu_item**)calloc(count + 3, sizeof(recovery_menu_item*));

    // add Wipe All
    int j = 0;
    items[j++] = create_menu_item(ITEM_WIPE_ALL, "Wipe All");

    // add the valid devices
    const char* wipe_menu_format = "Wipe %s";
    for(i = 0; i < device_partition_num; ++i) {
        // silently ignore datadata, it will get wiped with DATA if it exists
        if(device_partitions[i].id == PARTITION_DATADATA)
            continue;

        if((device_partitions[i].flags & PARTITION_FLAG_WIPEABLE) > 0 &&
                has_volume(device_partitions[i].path)) {
            int buflen = strlen(device_partitions[i].name) + strlen(wipe_menu_format);
            char* buf = (char*)calloc(buflen, sizeof(char));
            snprintf(buf, buflen, wipe_menu_format, device_partitions[i].name);
            items[j++] = create_menu_item(device_partitions[i].id, buf);
            free(buf);
        }
    }
    items[j++] = create_menu_item(ITEM_WIPE_BATT, "Wipe battery stats");
    items[j] = NULL;

    char* headers[] = { "Choose an item to wipe",
                        "or press DEL or POWER to return",
                        "USE CAUTION:",
                        "These operations *CANNOT BE UNDONE*",
                        "", NULL };

    recovery_menu* menu = create_menu(
            headers,
            items,
            /* no data */ NULL,
            /* no on_create */ NULL,
            /* no on_create_items */ NULL,
            &wipe_menu_select,
            /* no on_destroy */ NULL);
    display_menu(menu);
    destroy_menu(menu);
}
