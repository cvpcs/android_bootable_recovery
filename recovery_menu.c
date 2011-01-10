#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/reboot.h>

#include "common.h"
#include "install.h"
#include "recovery_lib.h"
#include "recovery_ui.h"
#include "minui/minui.h"
#include "roots.h"

#include "recovery_menu.h"
#include "nandroid_menu.h"
#include "mount_menu.h"
#include "install_menu.h"
#include "wipe_menu.h"


void prompt_and_wait()
{

    char* menu_headers[] = { "Modded by raidzero",
			     "",
			     NULL };

    char** headers = prepend_title(menu_headers);

    char* items[] = { "Reboot into Android",
		      "Reboot into Recovery",
                      "Shutdown system",
		      "Wipe partitions",
		      "Mount options",
		      "Backup/restore",
		      "Install",
		      "Help",
		      NULL };


    char* argv[]={"/sbin/test_menu.sh",NULL};
    char* envp[]={NULL};


#define ITEM_REBOOT          0
#define ITEM_RECOVERY	     1
#define ITEM_POWEROFF        2
#define ITEM_WIPE_PARTS      3
#define ITEM_MOUNT_MENU      4
#define ITEM_NANDROID_MENU   5
#define ITEM_INSTALL         6
#define ITEM_HELP            7

    int chosen_item = -1;
    for (;;) {
        ui_reset_progress();
	if (chosen_item==9999) chosen_item=0;

        chosen_item = get_menu_selection(headers, items, 0, chosen_item<0?0:chosen_item);

        // device-specific code may take some action here.  It may
        // return one of the core actions handled in the switch
        // statement below.
        chosen_item = device_perform_action(chosen_item);

        switch (chosen_item) {
	case ITEM_REBOOT:
		ui_print("\n\n\n\n\n\n\n\n\n\n\n\n\nRebooting Android...");
		finish_recovery(NULL);
	    return;
	case ITEM_RECOVERY:
		ui_print("\n\n\n\n\n\n\n\n\n\n\n\n\nRebooting Recovery...");
		reboot(RB_AUTOBOOT);
	    return;
	case ITEM_POWEROFF:
	    ui_print("\n\n\n\n\n\n\n\n\n\n\n\n\nShutting down...");
		reboot(RB_POWER_OFF);
	    return;
	case ITEM_WIPE_PARTS:
	    show_wipe_menu();
	    break;
	case ITEM_MOUNT_MENU:
	    show_mount_menu();
	    break;
	case ITEM_NANDROID_MENU:
	    show_nandroid_menu();
	    break;
	case ITEM_INSTALL:
	    show_install_menu();
	    break;
	case ITEM_HELP:
		ui_print("\n\n\nHELP/FEATURES:\n");
		ui_print("1.1ghz kernel, wipe menu,\n");
		ui_print("Battery charging,\n");
		ui_print("arbitrary unsigned update.zip install,\n");
		ui_print("arbitrary kernel/recovery img install\n");
		ui_print("rom.tar/tgz scripted install support\n");
		ui_print("\nRecovery images must be in /sdcard/recovery &\n");
		ui_print("end in either -rec.img, _rec.img, .rec.img\n");
		ui_print("\nKernel images must be in /sdcard/kernels &\n");
		ui_print("end in -kernel-boot.img, boot.img, kernel.img\n");
		ui_print("\nWipe Menu: wipes any partition on device\n");
		ui_print("also includes battery statistics wipe\n");
	    break;
        }
    }
}

int count_menu_items(recovery_menu_item** list) {
    // count how many items we have
    int count = 0;
    recovery_menu_item** p;
    for (p = list; *p; ++p, ++count);
    return count;
}

int count_menu_headers(char** list) {
    // count how many items we have
    int count = 0;
    char** p;
    for (p = list; *p; ++p, ++count);
    return count;
}

recovery_menu_item* create_menu_item(int id, const char* title) {
    recovery_menu_item* item = (recovery_menu_item*)malloc(sizeof(recovery_menu_item));
    item->id = id;
    item->title = strdup(title);
    return item;
}

recovery_menu_item* duplicate_menu_item(recovery_menu_item* item) {
    return create_menu_item(item->id, item->title);
}

void destroy_menu_item(recovery_menu_item* item) {
    free(item->title);
    free(item);
}

recovery_menu* create_menu(char** headers, recovery_menu_item* items, void* data,
        menu_create_callback on_create, menu_create_items_callback on_create_items,
        menu_select_callback on_select, menu_destroy_callback on_destroy) {
    recovery_menu* menu = (recovery_menu*)malloc(sizeof(recovery_menu));

    // good old iterator
    int i;

    // copy the headers
    int num_headers = count_menu_headers(headers);
    menu->headers = (char**)calloc(num_headers + 1, sizeof(char*));
    for(i = 0; i < num_headers; ++i) { menu->headers[i] = strdup(headers[i]); }
    menu->headers[num_headers] = NULL;

    // copy the items if we have any (might be NULL if useing on_create_items
    if(items) {
        int num_items = count_menu_items(&items);
        menu->items = (recovery_menu_item**)calloc(num_items + 1, sizeof(recovery_menu_item*));
        for(i = 0; i < num_items; ++i) { menu->items[i] = duplicate_menu_item(&items[i]); }
        menu->items[num_items] = NULL;
    } else {
        menu->items = NULL;
    }

    menu->data = data;
    menu->on_create = on_create;
    menu->on_create_items = on_create_items;
    menu->on_select = on_select;
    menu->on_destroy = on_destroy;

    return menu;
}

void destroy_menu(recovery_menu* menu) {
    // good old iterator
    int i;

    // destroy the headers
    int num_headers = count_menu_headers(menu->headers);
    for(i = 0; i < num_headers; ++i) { free(menu->headers[i]); }
    free(menu->headers);

    // destroy the items if they exists
    if(menu->items) {
        int num_items = count_menu_items(menu->items);
        for(i = 0; i < num_items; ++i) { destroy_menu_item(menu->items[i]); }
        free(menu->items);
    }

    // finally destroy the menu
    free(menu);
}

void display_menu(recovery_menu* menu)
{
    char** headers = prepend_title(menu->headers);

    // if we were provided with an "on_create" method, call it for this menu
    if(menu->on_create) {
        (*(menu->on_create))(menu->data);
    }

    int chosen_item = -1;
    while(chosen_item != ITEM_BACK) {
        recovery_menu_item** menu_items = menu->items;        

        // if we are dynamically creating items, go for it here
        if(menu->on_create_items) {
            menu_items = (*(menu->on_create_items))(menu->data);
        }

        // if we weren't given any menu items (menu->items == NULL and menu->on_create_items == NULL),
        // then we need to bail out
        if(!menu_items) {
            break;
        }

        // count how many items we have
        int count = count_menu_items(menu_items);

        // create a char array for use in get_menu_selection from our list of menu items
        char** items = (char**)calloc(count + 1, sizeof(char*));
        char** ip = items;
        recovery_menu_item** p;
        for (p = menu_items; *p; ++p, ++ip) *ip = (*p)->title;
        *ip = NULL;

        chosen_item = get_menu_selection(headers, items, 1,
                (chosen_item < 0 ? 0 : chosen_item));

        int item_id = menu_items[chosen_item]->id;

        free(items);

        // if we created our items, we must now destroy them
        if(menu->on_create_items) {
            int i;
            for(i = 0; i < count; ++i) { destroy_menu_item(menu_items[i]); }
            free(menu_items);
        }

        // call our "on_select" method, which should always exist
        if(menu->on_select) {
            chosen_item = (*(menu->on_select))(menu_items[chosen_item]->id, menu->data);
        } else {
            // "on_select" doesn't exist so this menu is pointless, exit it
            break;
        }
	}

    // cleanup code
    if(menu->on_destroy) {
        (*(menu->on_destroy))(menu->data);
    }
}

