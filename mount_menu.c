#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "common.h"
#include "recovery_lib.h"
#include "roots.h"
#include "recovery_ui.h"
#include "recovery_menu.h"

typedef struct {
    char* name;
    char* path;
} mount_info;

#define MOUNT_PARTITION_SYSTEM 0
#define MOUNT_PARTITION_DATA   1
#define MOUNT_PARTITION_CACHE  2
#define MOUNT_PARTITION_SDCARD 3

int count_mount_partitions(mount_info* partitions) {
    int count = 0;
    mount_info* p;
    for(p = partitions; p->name; ++p, ++count);
    return count;
}

mount_info* get_mount_partitions() {
    // create our list defaulting to not being mounted
    static mount_info mount_partitions[] = {
        { "System",  "/system" },
        { "Data",    "/data" },
        { "Cache",   "/cache" },
        { "SD Card", "/sdcard" },
        { NULL, NULL }
    };

    return mount_partitions;
}

void toggle_mount_partition(int which) {
    // get our updated partition list
    mount_info* partitions = get_mount_partitions();

    // get a count of how many partitions we have
    int count = count_mount_partitions(partitions);

    // bail out if our selection is invalid
    if(which < 0 || which >= count) {
        return;
    }

    // ok, we have a valid selection, so toggle it
    mount_info mount = partitions[which];
    if(is_path_mounted(mount.path)) {
		ensure_path_unmounted(mount.path);
		ui_print("Unm");
	} else {
		ensure_path_mounted(mount.path);
		ui_print("M");
    }
    ui_print("ounted ");
    ui_print(mount.name);
    ui_print("\n");
}

int is_usb_storage_enabled()
{
    FILE* fp = fopen("/sys/devices/platform/usb_mass_storage/lun0/file","r");
    char* buf = malloc(11*sizeof(char));
    fgets(buf, 11, fp);
    fclose(fp);

    if(strcmp(buf,"/dev/block")==0) {
        return(1);
    } else {
        return(0);
    }
}

void enable_usb_mass_storage()
{
    ensure_path_unmounted("/sdcard");
    FILE* fp = fopen("/sys/devices/platform/usb_mass_storage/lun0/file","w");
    if(fp != NULL) {
        ui_print("Enabling USB mass storage ... ");
        const char* sdcard_partition = "/dev/block/mmcblk0\n";
        fprintf(fp,sdcard_partition);
        fclose(fp);
        ui_print("complete\n");
    } else {
        LOGE("Error enabling USB mass storage (%s)\n", strerror(errno));
    }
}

void disable_usb_mass_storage()
{
    FILE* fp = fopen("/sys/devices/platform/usb_mass_storage/lun0/file","w");
    if(fp != NULL) {
        ui_print("Disabling USB mass storage ... ");
        fprintf(fp,"\n");
        fclose(fp);
        ui_print("complete\n");
    } else {
        LOGE("Error disabling USB mass storage (%s)\n", strerror(errno));
    }
}

// we set our item ids equal to the mount ids, so we can create our menu dynamically
#define ITEM_MOUNT_SYSTEM MOUNT_PARTITION_SYSTEM
#define ITEM_MOUNT_DATA   MOUNT_PARTITION_DATA
#define ITEM_MOUNT_CACHE  MOUNT_PARTITION_CACHE
#define ITEM_MOUNT_SDCARD MOUNT_PARTITION_SDCARD
#define ITEM_ENABLE_USB   1000 // we make this large so we know it won't interfere with the others

recovery_menu_item** mount_menu_create_items(void* data) {
    // get our updated partition list
    mount_info* partitions = get_mount_partitions();

    // get a count of how many partitions we have
    int count = count_mount_partitions(partitions);

    // build our item list (#mounts plus 2 for USB and NULL)
    recovery_menu_item** items = (recovery_menu_item**)calloc(count + 2, sizeof(recovery_menu_item*));

    // fill the beginning of the array with mount options
    int i;
    char* buf;
    for(i = 0; i < count; ++i) {
        buf = (char*)calloc(9 + strlen(partitions[i].name), sizeof(char));
        if(is_path_mounted(partitions[i].path)) {
            strcpy(buf, "Unm");
        } else {
            strcpy(buf, "M");
        }
        strcat(buf, "ount ");
        strcat(buf, partitions[i].name);
        items[i] = create_menu_item(i, buf);
        free(buf);
    }
    // add usb storage
    buf = (char*)calloc(25, sizeof(char));
    if(is_usb_storage_enabled()) {
        strcpy(buf, "Dis");
    } else {
        strcpy(buf, "En");
    }
    strcat(buf, "able USB Mass Storage");
    items[i++] = create_menu_item(ITEM_ENABLE_USB, buf);
    free(buf);
    items[i++] = NULL;

    return items;
}

int mount_menu_select(int chosen_item, void* data) {
    switch (chosen_item) {
    case ITEM_ENABLE_USB:
	    if(is_usb_storage_enabled()) {
            disable_usb_mass_storage();
	    } else {
            enable_usb_mass_storage();
	    }
        break;
    default:
        // we default to mounting something, because even if it's invalid it will just
        // bail out
        toggle_mount_partition(chosen_item);
        break;
    }

    return chosen_item;
}

void show_mount_menu()
{
    char* headers[] = { "Choose a mount or unmount option",
                        "or press DEL or POWER to return",
                        "", NULL };
    recovery_menu* menu = create_menu(
            headers,
            /* no items */ NULL,
            /* no data */ NULL,
            /* no on_create */ NULL,
            &mount_menu_create_items,
            &mount_menu_select,
            /* no on_destroy */ NULL);
    display_menu(menu);
    destroy_menu(menu);
}

