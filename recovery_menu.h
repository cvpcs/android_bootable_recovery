#ifndef RECOVERY_RECOVERY_MENU_H_
#define RECOVERY_RECOVERY_MENU_H_

void prompt_and_wait();
int erase_volume(const char*);

int get_menu_selection(char** headers, char** items, int menu_only, int initial_selection);

/**
 * Menu creation callback
 *
 * Called when a menu is initially created, provides a void pointer to the data object supplied
 * when the menu was created.
 */
typedef void(*menu_create_callback)(void*);

/**
 * Menu selection callback
 *
 * Called when a menu item is selected, provides the value of the selection as well as a void pointer
 * to the data object supplied when the menu was created.  It is expected to return the id of the
 * selection for cases where this callback may wish to modify that selection.
 */
typedef int(*menu_select_callback)(int, void*);

/**
 * Menu destruction callback
 *
 * Called when a menu item is destroyed, provides a void pointer to the data object supplied
 * when the menu was created.
 */
typedef void(*menu_destroy_callback)(void*);

typedef struct {
    // text headers for the menu to be displayed
    char** headers;
    // text items for the menu to be displayed
    char** items;

    // void pointer to menu-specific data that will be passed around
    void* data;

    // callbacks for menu creation/selection/destruction
    menu_create_callback on_create;
    menu_select_callback on_select;
    menu_destroy_callback on_destroy;
} recovery_menu;

/**
 * Creates a recovery menu object
 *
 * Note: the object will be created but data from headers, items, and data will not
 *       be serially copied over nore destroyed in a corresponding destroy_menu() call
 */
recovery_menu* create_menu(char** headers, char** items, void* data,
        menu_create_callback on_create, menu_select_callback on_select,
        menu_destroy_callback on_destroy);

void destroy_menu(recovery_menu* menu);

void display_menu(recovery_menu* menu);

#endif//RECOVERY_RECOVERY_MENU_H_
