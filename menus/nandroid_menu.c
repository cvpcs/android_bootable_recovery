#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <libgen.h>

#include "common.h"
#include "recovery_menu.h"
#include "recovery_lib.h"
#include "recovery_ui.h"
#include "minui/minui.h"
#include "roots.h"
#include "nandroid_menu.h"

#include "nandroid/nandroid.h"

recovery_menu_item** select_nandroid_menu_create_items(void* data) {
    nandroid** nandroids = nandroid_scan();

    int count = 0;
    nandroid** p = nandroids;
    while(*(p++)) count++;

    recovery_menu_item** items = (recovery_menu_item**)calloc(count + 1, sizeof(recovery_menu_item*));
    int i;
    const char* item_format = "%s (parts: %d)"; // name: (part count)
    for(i = 0; i < count; ++i) {
        char* name = strdup(basename(strdup(nandroids[i]->dir)));
        int pcount = 0;
        int* pp = nandroids[i]->partitions;
        while(*(pp++) != INT_MAX) pcount++;

        int buflen = strlen(item_format) + strlen(name) + 4;
        char* buf = (char*)calloc(buflen, sizeof(char));

        snprintf(buf, buflen, item_format, name, pcount);

        items[i] = create_menu_item(i, buf);

        free(name);
    }
    items[i] = NULL;

    destroy_nandroid_list(nandroids);

    return items;
}

int select_nandroid_menu_select(int chosen_item, void* data) {
    nandroid** nandroids = nandroid_scan();

    int count = 0;
    nandroid** p = nandroids;
    while(*(p++)) count++;

    destroy_nandroid_list(nandroids);

    if(chosen_item < count) {
        // we selected a nandroid, so set it
        (*(int*)data) = chosen_item;
        // and break from the menu
        return ITEM_BACK;
    }

    return chosen_item;
}

// returns the selection of the nandroid
int show_select_nandroid_menu() {
    char* headers[] = { "Choose a detected nandroid",
			            "", NULL };

    int selection = -1;
    recovery_menu* menu = create_menu(
            headers,
            /* no items */ NULL,
            &selection,
            /* no on_create */ NULL,
            &select_nandroid_menu_create_items,
            &select_nandroid_menu_select,
            /* no on_destroy */ NULL);
    display_menu(menu);
    destroy_menu(menu);
    return selection;
}

#define ITEM_NANDROID_ADV_RESTORE_GO        (PARTITION_LAST + 1)
#define ITEM_NANDROID_ADV_RESTORE_SELECT    (PARTITION_LAST + 2)

typedef struct {
    int partition;
    int restore;
} nandroid_adv_restore_menu;

recovery_menu_item** nandroid_adv_restore_menu_create_items(void* data) {
    // pull in our data
    nandroid* n = (nandroid*)data;

    recovery_menu_item** items;
    if(n->dir) {
        // count partitions
        int pcount = 0;
        int* p = n->partitions;
        while(*(p++) != INT_MAX) pcount++;

        // get a few buffers going
        char* buf = (char*)calloc(PATH_MAX, sizeof(char));
        char* buf2;

        // items
        int j = 0;
        items = (recovery_menu_item**)calloc(3 + pcount, sizeof(recovery_menu_item*));

        // obvious first item
        items[j++] = create_menu_item(ITEM_NANDROID_ADV_RESTORE_GO, "Perform restore");

        buf2 = strdup(basename(strdup(n->dir)));
        snprintf(buf, PATH_MAX, "Select backup: %s", buf2);
        free(buf2);
        items[j++] = create_menu_item(ITEM_NANDROID_ADV_RESTORE_SELECT, buf);

        int i;
        for(i = 0; i < pcount; ++i) {
            int part = n->partitions[i];

            // we use the fact that partition numbers shouldn't be less than 0 to our advantage.
            // a partition is selected if it's id is in this list
            // a partition is not selected if -(id + 1) is in this list
            // a partition does not appear if neither are in the list
            if(part < 0) {
                // get the real id number
                part = -(part + 1);
            }

            if(part >= 0 && part < device_partition_num &&
                    device_partitions[part].id == part &&
                    (device_partitions[part].flags & PARTITION_FLAG_RESTOREABLE) > 0 &&
                    has_volume(device_partitions[part].path)) {
                snprintf(buf, PATH_MAX, "(%s) %s", (n->partitions[i] < 0 ? " " : "*"), device_partitions[part].name);
                items[j++] = create_menu_item(part, buf);
            }
        }

        items[j] = NULL;

        free(buf);
    } else {
        items = (recovery_menu_item**)calloc(3, sizeof(recovery_menu_item*));
        items[0] = create_menu_item(ITEM_NANDROID_ADV_RESTORE_GO,       "Perform restore");
        items[1] = create_menu_item(ITEM_NANDROID_ADV_RESTORE_SELECT,   "Select backup: ---- NONE SELECTED ----");
        items[2] = NULL;
    }
    return items;
}

int nandroid_adv_restore_menu_select(int chosen_item, void* data) {
    nandroid* n = (nandroid*)data;

    if(chosen_item == ITEM_NANDROID_ADV_RESTORE_GO) {
        ui_print("THUNDERCATS! GO!\n");
    } else if(chosen_item == ITEM_NANDROID_ADV_RESTORE_SELECT) {
        nandroid** nandroids = nandroid_scan();
        int i = show_select_nandroid_menu();
        if(i >= 0) {
            // clear and copy the new dir info
            if(n->dir)
                free(n->dir);
            n->dir = strdup(nandroids[i]->dir);

            // clear and copy the new partition info
            if(n->partitions)
                free(n->partitions);
            int pcount = 0;
            int* p = nandroids[i]->partitions;
            while(*(p++) != INT_MAX) pcount++;
            n->partitions = (int*)calloc(pcount + 1, sizeof(int));
            int j;
            for(j = 0; j < pcount; ++j) {
                n->partitions[j] = nandroids[i]->partitions[j];
            }
            n->partitions[j] = INT_MAX;

            // set md5 bit
            n->md5 = nandroids[i]->md5;
        }
    } else if(chosen_item < device_partition_num) {
        // cycle through the partition list and toggle whichever one they selected
        int* p = n->partitions;
        while(*p != INT_MAX) {
            if(*p < 0) {
                int val = -(*p + 1);
                if(val == chosen_item) {
                    *p = val;
                    break;
                }
            } else {
                if(*p == chosen_item) {
                    *p = -(*p + 1);
                    break;
                }
            }

            p++;
        }
    }

    return chosen_item;
}

void show_nandroid_adv_restore_menu() {
    char* headers[] = { "Select the nandroid you would like to restore",
                        "as well as which partitions",
                        "", NULL };

    // list of partitions to restore
    nandroid* n = (nandroid*)malloc(sizeof(nandroid));
    n->partitions = NULL;
    n->dir = NULL;
    n->md5 = 0;

    recovery_menu* menu = create_menu(
            headers,
            /* no items */ NULL,
            n,
            /* no on_create */ NULL,
            &nandroid_adv_restore_menu_create_items,
            &nandroid_adv_restore_menu_select,
            /* no on_destroy */ NULL);
    display_menu(menu);
    destroy_menu(menu);

    // free our structure
    if(n->dir)
        free(n->dir);
    if(n->partitions)
        free(n->partitions);
    free(n);
}

#define ITEM_NANDROID_BACKUP      0
#define ITEM_NANDROID_RESTORE     1
#define ITEM_NANDROID_ADV_BACKUP  2
#define ITEM_NANDROID_ADV_RESTORE 3

int nandroid_menu_select(int chosen_item, void* data) {
    int nandroid_id = -1;
    switch(chosen_item) {
    case ITEM_NANDROID_BACKUP:
        break;
    case ITEM_NANDROID_RESTORE:
        nandroid_id = show_select_nandroid_menu();
        if(nandroid_id >= 0) {
            ui_print("restore all from id %d\n", nandroid_id);
        }
        break;
    case ITEM_NANDROID_ADV_BACKUP:
        break;
    case ITEM_NANDROID_ADV_RESTORE:
        show_nandroid_adv_restore_menu();
        break;
    }

    return chosen_item;
}

void show_nandroid_menu()
{
    recovery_menu_item** items = (recovery_menu_item**)calloc(5, sizeof(recovery_menu_item*));
    items[0] = create_menu_item(ITEM_NANDROID_BACKUP,      "Simple nandroid backup");
    items[1] = create_menu_item(ITEM_NANDROID_RESTORE,     "Simple Nandroid restore");
    items[2] = create_menu_item(ITEM_NANDROID_ADV_BACKUP,  "Advanced Nandroid backup");
    items[3] = create_menu_item(ITEM_NANDROID_ADV_RESTORE, "Advanced Nandroid restore");
    items[4] = NULL;

    char* headers[] = { "Select an option or press POWER to return",
                        "", NULL };

    recovery_menu* menu = create_menu(
            headers,
            items,
            /* no data */ NULL,
            /* no on_create */ NULL,
            /* no on_create_items */ NULL,
            &nandroid_menu_select,
            /* no on_destroy */ NULL);
    display_menu(menu);
    destroy_menu(menu);
}
