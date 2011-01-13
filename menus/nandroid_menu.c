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
#include "nandroid.h"

/*
void nandroid_backup_menu(char* subname, char partitions)
{
    ui_print("Attempting Nandroid backup.\n");
    
    int boot  = partitions&BOOT;
    int cache = partitions&CACHE;
    int data  = partitions&DATA;
    int misc  = partitions&MISC;
    int recovery = partitions&RECOVERY;
    int system = partitions&SYSTEM;
    int progress = partitions&PROGRESS;

    int args = 4;
    if (!recovery) args++;
    if (!boot) args++;
    if (!cache) args++;
    if (!misc) args++;
    if (!system) args++;
    if (!data) args++;
    if (progress) args++;
    if (!strcmp(subname, "")) args+=2;  // if a subname is specified, we need 2 more arguments
  
    char** argv = malloc(args*sizeof(char*));
    argv[0]="/sbin/nandroid-mobile.sh";
    argv[1]="--backup";
    argv[2]="--defaultinput";
    argv[args]=NULL;
  
    int i = 3;
    if(!recovery) {
	argv[i++]="--norecovery";
    }
    if(!boot) {
	argv[i++]="--noboot";
    }
    if(!cache) {
	argv[i++]="--nocache";
    }
    if(!misc) {
	argv[i++]="--nomisc";
    }
    if(!system) {
	argv[i++]="--nosystem";
    }
    if(!data) {
	argv[i++]="--nodata";
    }
    if(progress) {
	argv[i++]="--progress";
    }
    if(strcmp(subname,"")) {
	argv[i++]="--subname";
	argv[i++]=subname;
    }
    argv[i++]=NULL;

    char* envp[] = { NULL };

    int status = runve("/sbin/nandroid-mobile.sh",argv,envp,60);
    if (!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
	ui_printf_int("ERROR: Nandroid exited with status %d\n",WEXITSTATUS(status));
    }
    else {
	ui_print("(done)\n");
    }
    ui_reset_progress();
}

void nandroid_restore_menu(char* subname, char partitions)
{
    int boot  = partitions&BOOT;
    int cache = partitions&CACHE;
    int data  = partitions&DATA;
    int misc  = partitions&MISC;
    int recovery = partitions&RECOVERY;
    int system = partitions&SYSTEM;
    int progress = partitions&PROGRESS;

    int args = 4;
    if (!recovery) args++;
    if (!boot) args++;
    if (!cache) args++;
    if (!misc) args++;
    if (!system) args++;
    if (!data) args++;
    if (progress) args++;
    if (!strcmp(subname, "")) args+=2;  // if a subname is specified, we need 2 more arguments
  
    char** argv = malloc(args*sizeof(char*));
    argv[0]="/sbin/nandroid-mobile.sh";
    argv[1]="--restore";
    argv[2]="--defaultinput";
    argv[args]=NULL;
  
    int i = 3;
    if(!recovery) {
	argv[i++]="--norecovery";
    }
    if(!boot) {
	argv[i++]="--noboot";
    }
    if(!cache) {
	argv[i++]="--nocache";
    }
    if(!misc) {
	argv[i++]="--nomisc";
    }
    if(!system) {
	argv[i++]="--nosystem";
    }
    if(!data) {
	argv[i++]="--nodata";
    }
    if(progress) {
	argv[i++]="--progress";
    }
    if(strcmp(subname,"")) {
	argv[i++]="--subname";
	argv[i++]=subname;
    }
    argv[i++]=NULL;

    char* envp[] = { NULL };

    int status = runve("/sbin/nandroid-mobile.sh",argv,envp,80);
    if (!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
	ui_printf_int("ERROR: Nandroid exited with status %d\n",WEXITSTATUS(status));
    }
    else {
	ui_print("(done)\n");
    }
    ui_reset_progress();
}

static void nandroid_simple_backup()
{
    ui_print("Attempting Nandroid default backup.\n");
    
    char* argv[] = { "/sbin/nandroid-mobile.sh",
		     "--backup",
		     "--nocache",
		     "--nomisc",
		     "--norecovery",
		     "--defaultinput",
		     "--progress",
		     NULL };
    char* envp[] = { NULL };
    int status = runve("/sbin/nandroid-mobile.sh",argv,envp,60);
    if (!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
	ui_printf_int("ERROR: Nandroid exited with status %d\n",WEXITSTATUS(status));
    }
    else {
	ui_print("(done)\n");
    }
    ui_reset_progress();
}

static void nandroid_simple_restore()
{
    ui_print("This should do a default nandroid restore.\n");
    
    ui_print("Attempting Nandroid default restore.\n");
    
    char* argv[] = { "/sbin/nandroid-mobile.sh",
		     "--restore",
		     "--nocache",
		     "--nomisc",
		     "--norecovery",
		     "--defaultinput",
		     "--progress",
		     NULL };
    char* envp[] = { NULL };
    int status = runve("/sbin/nandroid-mobile.sh",argv,envp,80);
    if (!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
	ui_printf_int("ERROR: Nandroid exited with status %d\n",WEXITSTATUS(status));
    }
    else {
	ui_print("(done)\n");
    }
    ui_reset_progress();
}

void nandroid_adv_r_choose_file(char* filename, char* path)
{
    char* headers[] = { "Choose a backup prefix or press",
			       "POWER or DEL to return",
			       "",
			       NULL };

    DIR *dir;
    struct dirent *de;
    int total = 0;
    int i;
    char** files;
    char** list;

    if (ensure_path_mounted(path) != 0) {
	LOGE("Can't mount %s\n", path);
	return;
    }

    dir = opendir(path);
    if (dir == NULL) {
	LOGE("Couldn't open directory %s\n", path);
	return;
    }

    while ((de=readdir(dir)) != NULL) {
	if (de->d_name[0] != '.') {
	    total++;
	}
    }

    if (total==0) {
	LOGE("No nandroid backups found\n");
	if(closedir(dir) < 0) {
	    LOGE("Failed to close directory %s\n", path);
	    goto out; // eww, maybe this can be done better
	}
    }
    else {
	files = (char**) malloc((total+1)*sizeof(char*));
	files[total]= NULL;

	list = (char**) malloc((total+1)*sizeof(char*));
	list[total] = NULL;

	rewinddir(dir);

	i = 0;
	while ((de = readdir(dir)) != NULL) {
	    if (de->d_name[0] != '.') {
		files[i] = (char*) malloc(strlen(path)+strlen(de->d_name)+1);
		strcpy(files[i], path);
		strcat(files[i], de->d_name);

		list[i] = (char *) malloc(strlen(de->d_name)+1);
		strcpy(list[i], de->d_name);

		i++;
	    }
	}

	if (closedir(dir) <0) {
	    LOGE("Failure closing directory %s\n", path);
	    goto out; // again, eww
	}

	int chosen_item = -1;
	while (chosen_item < 0) {
	    chosen_item = get_menu_selection(headers, list, 1, chosen_item<0?0:chosen_item);

	    if(chosen_item >= 0 && chosen_item != ITEM_BACK) {
		strcpy(filename,list[chosen_item]);
	    }
	}
    }
out:
    for (i = 0; i < total; i++) {
	free(files[i]);
	free(list[i]);
    }
    free(files);
    free(list);
}


void get_nandroid_adv_menu_opts(char** list, char p, char* br)
{
    char** tmp = malloc(7*sizeof(char*));
    int i;
    for (i=0; i<6; i++) {
	tmp[i]=malloc((strlen("(*)  RECOVERY")+strlen(br)+1)*sizeof(char));
    }
    
    sprintf(tmp[0],"(%c) %s BOOT",     p&BOOT?    '*':' ', br);
    sprintf(tmp[1],"(%c) %s CACHE",    p&CACHE?   '*':' ', br);
    sprintf(tmp[2],"(%c) %s DATA",     p&DATA?    '*':' ', br);
    sprintf(tmp[3],"(%c) %s MISC",     p&MISC?    '*':' ', br);
    sprintf(tmp[4],"(%c) %s RECOVERY", p&RECOVERY?'*':' ', br);
    sprintf(tmp[5],"(%c) %s SYSTEM",   p&SYSTEM?  '*':' ', br);
    tmp[6]=NULL;

    char** h = list;
    char** j = tmp;

    for(;*j;j++,h++) *h=*j;
    
}

void show_nandroid_adv_r_menu()
{
    char* headers[] = { "Choose an option or press POWER to return",
			"prefix:",
			"",
			"",
			NULL };

    char* items[] = { "Choose backup",
		      "Perform restore",
		      NULL,
		      NULL,
		      NULL,
		      NULL,
		      NULL,
		      NULL,
			  NULL	};
  
#define ITEM_CHSE 0
#define ITEM_PERF 1
#define ITEM_B    2
#define ITEM_C    3
#define ITEM_D    4
#define ITEM_M    5
#define ITEM_R    6
#define ITEM_S    7


    char filename[PATH_MAX];
    filename[0]=NULL;
    char partitions = (char) BSD;
    int chosen_item = -1;

    while(chosen_item!=ITEM_BACK) {
	get_nandroid_adv_menu_opts(items+2,partitions,"restore"); // put the menu options in items[] starting at index 2
	chosen_item=get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);

	switch(chosen_item) {
	case ITEM_CHSE:
	    nandroid_adv_r_choose_file(filename,"SDCARD:/nandroid");
	    headers[2]=filename;
	    break;
	case ITEM_PERF:
	    ui_print("Restoring...\n");
	    nandroid_restore_menu(filename,partitions|PROGRESS);
	    break;
	case ITEM_B:
	    partitions^=BOOT;
	    break;
	case ITEM_C:
	    partitions^=CACHE;
	    break;
	case ITEM_D:
	    partitions^=DATA;
	    break;
	case ITEM_M:
	    partitions^=MISC;
	    break;
	case ITEM_R:
	    partitions^=RECOVERY;
	    break;
	case ITEM_S:
	    partitions^=SYSTEM;
	    break;	
	}
    }
}

void show_nandroid_adv_b_menu()
{
    char* headers[] = { "Choose an option or press POWER to return",
			"prefix:",
			"",
			"",
			NULL };
    char* items[] = { "Set backup name",
		      "Perform backup",
		      NULL,
		      NULL,
		      NULL,
		      NULL,
		      NULL,
		      NULL,
		      NULL };
  
#define ITEM_NAME 0
#define ITEM_PERF 1
#define ITEM_B    2
#define ITEM_C    3
#define ITEM_D    4
#define ITEM_M    5
#define ITEM_R    6
#define ITEM_S    7

    char filename[PATH_MAX];
    filename[0]=NULL;
    int chosen_item = -1;

    char partitions = BSD;

    while(chosen_item!=ITEM_BACK) {
	get_nandroid_adv_menu_opts(items+2,partitions,"backup"); // put the menu options in items[] starting at index 2
	chosen_item=get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);

	switch(chosen_item) {
	case ITEM_NAME:
	    ui_print("Type a prefix to name your backup:\n> ");
	    ui_read_line_n(filename,PATH_MAX);
	    headers[2]=filename;
	    break;
	case ITEM_PERF:
	    nandroid_backup_menu(filename,partitions|PROGRESS);
	    break;
	case ITEM_B:
	    partitions^=BOOT;
	    break;
	case ITEM_C:
	    partitions^=CACHE;
	    break;
	case ITEM_D:
	    partitions^=DATA;
	    break;
	case ITEM_M:
	    partitions^=MISC;
	    break;
	case ITEM_R:
	    partitions^=RECOVERY;
	    break;
	case ITEM_S:
	    partitions^=SYSTEM;
	    break;
	}
    }
}
*/

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
#define ITEM_NANDROID_ADV_RESTORE_CHECKMD5  (PARTITION_LAST + 3)

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
        items = (recovery_menu_item**)calloc(4 + pcount, sizeof(recovery_menu_item*));

        // obvious first item
        items[j++] = create_menu_item(ITEM_NANDROID_ADV_RESTORE_GO, "Perform restore");

        buf2 = strdup(basename(strdup(n->dir)));
        snprintf(buf, PATH_MAX, "Select backup: %s", buf2);
        free(buf2);
        items[j++] = create_menu_item(ITEM_NANDROID_ADV_RESTORE_SELECT, buf);

        snprintf(buf, PATH_MAX, "(%s) Check MD5", (n->md5 ? "*" : " "));
        items[j++] = create_menu_item(ITEM_NANDROID_ADV_RESTORE_CHECKMD5, buf);

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
    } else if(chosen_item == ITEM_NANDROID_ADV_RESTORE_CHECKMD5) {
        // this is never null, so we can toggle it no matter what
        n->md5 = !(n->md5);
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
