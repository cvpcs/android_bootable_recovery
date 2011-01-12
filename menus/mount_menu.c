#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "common.h"
#include "recovery_lib.h"
#include "roots.h"
#include "recovery_ui.h"
#include "recovery_menu.h"

void toggle_mount_partition(int which) {
    if(which < 0 || which >= device_partition_num || // make sure it's a partition
            device_partitions[which].id != which || // make sure our "which" is pointing correctly
            (device_partitions[which].flags & PARTITION_FLAG_MOUNTABLE) == 0 || // make sure we're wipeable
            !has_volume(device_partitions[which].path)) { // make sure we exist
        // not valid
        LOGE("Invalid device specified in (un)mount");
        return;
    }

    const char* mount_format = "Mounted %s\n";
    const char* unmount_format = "Unmounted %s\n";
    if(is_path_mounted(device_partitions[which].path)) {
		ensure_path_unmounted(device_partitions[which].path);
        ui_print(unmount_format, device_partitions[which].name);
	} else {
		ensure_path_mounted(device_partitions[which].path);
        ui_print(mount_format, device_partitions[which].name);
    }
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

#define ITEM_ENABLE_USB (PARTITION_LAST + 1)

recovery_menu_item** mount_menu_create_items(void* data) {
    int i;
    int count = 0;
    for(i = 0; i < device_partition_num; ++i) {
        if((device_partitions[i].flags & PARTITION_FLAG_MOUNTABLE) > 0 &&
                has_volume(device_partitions[i].path)) {
            count++;
        }
    }

    recovery_menu_item** items = (recovery_menu_item**)calloc(count + 2, sizeof(recovery_menu_item*));

    // add Wipe All
    int j = 0;

    // add the valid devices
    const char* mount_menu_format = "Mount %s";
    const char* unmount_menu_format = "Unmount %s";
    for(i = 0; i < device_partition_num; ++i) {
        if((device_partitions[i].flags & PARTITION_FLAG_MOUNTABLE) > 0 &&
                has_volume(device_partitions[i].path)) {
            if(is_path_mounted(device_partitions[i].path)) {
                int buflen = strlen(device_partitions[i].name) + strlen(unmount_menu_format);
                char* buf = (char*)calloc(buflen, sizeof(char));
                snprintf(buf, buflen, unmount_menu_format, device_partitions[i].name);
                items[j++] = create_menu_item(device_partitions[i].id, buf);
                free(buf);
            } else {
                int buflen = strlen(device_partitions[i].name) + strlen(mount_menu_format);
                char* buf = (char*)calloc(buflen, sizeof(char));
                snprintf(buf, buflen, mount_menu_format, device_partitions[i].name);
                items[j++] = create_menu_item(device_partitions[i].id, buf);
                free(buf);
            }
        }
    }
    if(is_usb_storage_enabled()) {
        items[j++] = create_menu_item(ITEM_ENABLE_USB, "Disable USB mass storage");
    } else {
        items[j++] = create_menu_item(ITEM_ENABLE_USB, "Enable USB mass storage");
    }
    items[j] = NULL;

    return items;
}

int mount_menu_select(int chosen_item, void* data) {
    if(chosen_item == ITEM_ENABLE_USB) {
	    if(is_usb_storage_enabled()) {
            disable_usb_mass_storage();
	    } else {
            enable_usb_mass_storage();
	    }
    } else if(chosen_item < device_partition_num) {
        toggle_mount_partition(chosen_item);
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

