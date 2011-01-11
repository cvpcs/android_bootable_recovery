#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/reboot.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>

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

int count_strlist_items(char** list) {
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
    if(item) {
        free(item->title);

        item->id = 0;
        item->title = NULL;

        free(item);
        item = NULL;
    }
}

recovery_menu* create_menu(char** headers, recovery_menu_item** items, void* data,
        menu_create_callback on_create, menu_create_items_callback on_create_items,
        menu_select_callback on_select, menu_destroy_callback on_destroy) {
    recovery_menu* menu = (recovery_menu*)malloc(sizeof(recovery_menu));

    // good old iterator
    int i;

ui_print("D:counting headers: ");
    int ch = count_strlist_items(headers);
ui_print("found %d\n", ch);
ui_print("D:creating headers\n");
    menu->headers = (char**)calloc(ch + 1, sizeof(char*));
ui_print("D:copying headers\n");
    for(i = 0; i < ch; ++i) { menu->headers[i] = strdup(headers[i]); }
ui_print("D:null-terminating headers\n");
    menu->headers[i] = NULL;

    menu->items = items;
    menu->data = data;
    menu->on_create = on_create;
    menu->on_create_items = on_create_items;
    menu->on_select = on_select;
    menu->on_destroy = on_destroy;

    return menu;
}

void destroy_menu(recovery_menu* menu) {
    if(menu) {
        // good old iterator
        int i;

        // destroy the headers
        int num_headers = count_strlist_items(menu->headers);
        for(i = 0; i < num_headers; ++i) { free(menu->headers[i]); }
        free(menu->headers);
        menu->headers = NULL;

        // destroy the items if they exists
        if(menu->items) {
            int num_items = count_menu_items(menu->items);
            for(i = 0; i < num_items; ++i) { destroy_menu_item(menu->items[i]); }
            free(menu->items);
            menu->items = NULL;
        }

        // nullify other stuff
        menu->data = NULL;
        menu->on_create = NULL;
        menu->on_create_items = NULL;
        menu->on_select = NULL;
        menu->on_destroy = NULL;

        // finally destroy the menu
        free(menu);
        menu = NULL;
    }
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
        menu_items = NULL;

        // call our "on_select" method, which should always exist
        if(menu->on_select) {
            chosen_item = (*(menu->on_select))(item_id, menu->data);
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

typedef struct {
    int id;
    char* name;
    char* path;
    int is_dir;
} file_select_menu_item;

typedef struct {
    char** exts; // the valid extension list
    char* base_path; // the base path we are allowed to access (going "back" after this will consititue leaving the menu)
    char* cur_path; // the current "base path" we are viewing
    file_select_menu_item** dirs; // list of current directories
    file_select_menu_item** files; // list of current files
    file_select_callback callback; // the callback
} file_select_menu;

#define FILE_SELECT_MENU_ITEM_DIR_ID_BASE 1000
#define FILE_SELECT_MENU_ITEM_FILE_ID_BASE 2000

file_select_menu_item* create_file_select_menu_item(int id, char* name, char* base_path, int is_dir) {
    file_select_menu_item* item = (file_select_menu_item*)malloc(sizeof(file_select_menu_item));
    item->id = id;
    item->path = (char*)malloc(strlen(base_path) + strlen(name) + 2);
    strcpy(item->path, base_path);
    strcat(item->path, "/");
    strcat(item->path, name);
    item->is_dir = is_dir;
    if(item->is_dir) {
        // if item is a directory, append a "/" to its name
        item->name = (char*)malloc(strlen(name) + 2);
        strcpy(item->name, name);
        strcat(item->name, "/");
    } else {
        item->name = strdup(name);
    }
    return item;
}

void destroy_file_select_menu_item(file_select_menu_item* item) {
    if(item) {
        free(item->name);
        free(item->path);
        free(item);
    }
}

int count_file_select_menu_items(file_select_menu_item** list) {
    // count how many items we have
    int count = 0;
    file_select_menu_item** p;
    for (p = list; *p; ++p, ++count);
    return count;
}

void destroy_file_select_menu_item_list(file_select_menu_item** list) {
    if(list) {
        int count = count_file_select_menu_items(list);
        int i;
        for(i = 0; i < count; ++i) {
            destroy_file_select_menu_item(list[i]);
        }
        free(list);
    }
}

recovery_menu_item** file_select_menu_create_items(void* data) {
    file_select_menu* fsm = (file_select_menu*)data;

    // open the current directory
    DIR* dir = opendir(fsm->cur_path);
    if (dir == NULL) {
        LOGE("Couldn't open %s (%s)", fsm->cur_path, strerror(errno));
	    return NULL;
    }

    int fcount = 0;
    int dcount = 0;
    int i;

    // get a count of our extensions
    int ext_count = count_strlist_items(fsm->exts);

    // first we count our files/directories
    struct dirent *de;
    while ((de=readdir(dir)) != NULL) {
        // ignore . and .. and hidden files
        if (de->d_name[0] != '.') {
            if(de->d_type == DT_DIR) {
                // directory types are always valid
                ++dcount;
            } else if(de->d_type == DT_REG) {
                // regular file, check extensions
                if(ext_count > 0) {
                    int dname_len = strlen(de->d_name);
                    for(i = 0; i < ext_count; ++i) {
                        int ext_len = strlen(fsm->exts[i]);
                        if(dname_len > ext_len && strcmp(de->d_name + (dname_len - ext_len), fsm->exts[i]) == 0) {
                            ++fcount;
                            break;
                        }
                    }
                } else {
                    ++fcount;
                }
            }
        }
    }

    file_select_menu_item** file_items = (file_select_menu_item**)calloc(fcount + 1, sizeof(file_select_menu_item*));
    file_select_menu_item** dir_items = (file_select_menu_item**)calloc(dcount + 1, sizeof(file_select_menu_item*));
    file_items[fcount] = NULL;
    dir_items[dcount] = NULL;

	rewinddir(dir);

    int j = 0;
    int k = 0;
	while ((de = readdir(dir)) != NULL) {
        if (de->d_name[0] != '.') {
            int valid = 0;

            if(de->d_type == DT_DIR) {
                valid = 1;
            } else if(de->d_type == DT_REG) {
                // regular file, check extensions
                if(ext_count > 0) {
                    int dname_len = strlen(de->d_name);
                    for(i = 0; i < ext_count; ++i) {
                        int ext_len = strlen(fsm->exts[i]);
                        if(dname_len > ext_len && strcmp(de->d_name + (dname_len - ext_len), fsm->exts[i]) == 0) {
                            valid = 1;
                            break;
                        }
                    }
                } else {
                    valid = 1;
                }
            }

            if(valid) {
                if(de->d_type == DT_DIR && j < dcount) {
                    dir_items[j] = create_file_select_menu_item(FILE_SELECT_MENU_ITEM_DIR_ID_BASE + j, de->d_name, fsm->cur_path, 1);
                    ++j;
                } else if(k < fcount) {
                    file_items[k] = create_file_select_menu_item(FILE_SELECT_MENU_ITEM_FILE_ID_BASE + k, de->d_name, fsm->cur_path, 0);
                    ++k;
                }
            }
        }
    }

	if (closedir(dir) < 0) {
        // clear our menu items
        destroy_file_select_menu_item_list(file_items);
        destroy_file_select_menu_item_list(dir_items);

        // report the failure
        LOGE("Failure closing directory %s (%s)", fsm->cur_path, strerror(errno));
	    return NULL;
	}

    // reset our items in the data structure
    destroy_file_select_menu_item_list(fsm->files);
    destroy_file_select_menu_item_list(fsm->dirs);
    fsm->dirs = dir_items;
    fsm->files = file_items;

    // create our menu items
    recovery_menu_item** items = (recovery_menu_item**)calloc(dcount + fcount + 1, sizeof(recovery_menu_item*));
    i = 0;
    for(j = 0; j < dcount; ++i, ++j) {
        items[i] = create_menu_item(fsm->dirs[j]->id, fsm->dirs[j]->name);
    }
    for(k = 0; k < fcount; ++i, ++j) {
        items[i] = create_menu_item(fsm->files[k]->id, fsm->files[k]->name);
    }
    items[i] = NULL;

    return items;
}

int file_select_menu_select(int chosen_item, void* data) {
    // get our menu data
    file_select_menu* fsm = (file_select_menu*)data;

    // are we going back?
    if(chosen_item == ITEM_BACK) {
        // only report back if cur_path == base_path
        if(strcmp(fsm->base_path, fsm->cur_path) != 0) {
            // we need to reset our current path
            char* new_path = strdup(dirname(fsm->cur_path));
            free(fsm->cur_path);
            fsm->cur_path = new_path;
            return NO_ACTION;
        } else {
            return ITEM_BACK;
        }
    }

    // if we're here, we need to process a possible click on a directory/file

    // get our counts
    int dcount = count_file_select_menu_items(fsm->dirs);
    int fcount = count_file_select_menu_items(fsm->files);

    // did we choose a dir?
    int i;
    for(i = 0; i < dcount; i++) {
        if(chosen_item == fsm->dirs[i]->id) {
            // we selected a directory, so reset the current path and return
            free(fsm->cur_path);
            fsm->cur_path = strdup(fsm->dirs[i]->path);
            return NO_ACTION;
        }
    }

    // did we choose a file?
    for(i = 0; i < fcount; i++) {
        if(chosen_item == fsm->files[i]->id) {
            // we selected a file! woohoo! time to call our callback and get the hell out of dodge
            (*(fsm->callback))(fsm->files[i]->path);
            return ITEM_BACK;
        }
    }

    // no idea what the hell they clicked on so just pass through
    return chosen_item;
}

void display_file_select_menu(char* base_path, char** exts, file_select_callback on_select) {
    // make sure our path is mounted
    if (ensure_path_mounted(base_path) != 0) {
        LOGE("Can't mount %s (%s)", base_path, strerror(errno));
        return;
    }

    // create our headers
    char* headers[] = { "Choose a file/directory",
                        "", NULL };

    // create our data structure
    file_select_menu* fsm = (file_select_menu*)malloc(sizeof(file_select_menu));
    fsm->exts = exts;
    fsm->base_path = base_path;
    fsm->cur_path = strdup(base_path);
    fsm->dirs = NULL;
    fsm->files = NULL;
    fsm->callback = on_select;

    recovery_menu* menu = create_menu(
            headers,
            /* no items */ NULL,
            (void*)fsm,
            /* no on_create */ NULL,
            &file_select_menu_create_items,
            &file_select_menu_select,
            /* no on_destroy */ NULL);
    display_menu(menu);
    destroy_menu(menu);

    // destroy our data structure
    destroy_file_select_menu_item_list(fsm->dirs);
    destroy_file_select_menu_item_list(fsm->files);
    free(fsm->cur_path);
    free(fsm);
}
