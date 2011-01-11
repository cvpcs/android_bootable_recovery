#ifndef RECOVERY_RECOVERY_MENU_H_
#define RECOVERY_RECOVERY_MENU_H_

void prompt_and_wait();
int erase_volume(const char*);

int get_menu_selection(char** headers, char** items, int menu_only, int initial_selection);

/**
 * This structure represents a single menu item.  When selected its ID will be returned
 * in the on_select callback.
 */
typedef struct {
    int id;
    char* title;
} recovery_menu_item;

recovery_menu_item* create_menu_item(int id, const char* title);
recovery_menu_item* duplicate_menu_item(recovery_menu_item* item);
void destroy_menu_item(recovery_menu_item* item);

/**
 * Menu creation callback
 *
 * Called when a menu is initially created, provides a void pointer to the data object supplied
 * when the menu was created.
 */
typedef void(*menu_create_callback)(void*);

/**
 * Menu item creation callback
 *
 * Called whenever the menu loop is continuing, so that dynamic menus can change their options.
 * Provides a void pointer to the data object supplied when the menu was created, and is expected
 * to return a valid array of pointers to recovery_menu_item(s)
 *
 * NOTE: MENU ITEMS ARE EXPECTED TO BE CREATED USING THE recovery_menu_item_create() FUNCTION, AS THEY
 *       WILL BE AUTOMATICALLY DESTROYED USING THE recovery_menu_item_destroy() FUNCTION
 */
typedef recovery_menu_item**(*menu_create_items_callback)(void*);

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

/**
 * This structure represents an entire menu.
 */
typedef struct {
    // text headers for the menu to be displayed (NULL terminated)
    char** headers;
    // text items for the menu to be displayed (NULL terminated)
    recovery_menu_item** items;

    // void pointer to menu-specific data that will be passed around
    void* data;

    // callbacks for menu creation/selection/destruction
    menu_create_callback on_create;
    menu_create_items_callback on_create_items;
    menu_select_callback on_select;
    menu_destroy_callback on_destroy;
} recovery_menu;

/**
 * Creates a recovery menu object
 *
 * NOTE: ALL DATA WILL BE SERIALLY COPIED AND SUBSEQUENTLY DESTROYED WHEN destroy_menu() IS
 *       CALLED, WITH THE EXCEPTION OF THE data POINTER, WHICH IS LEFT UP TO YOU TO FREE AS
 *       IS APPROPRIATE
 * NOTE: headers AND items SHOULD BE NULL-TERMINATED LISTS
 */
recovery_menu* create_menu(char** headers, recovery_menu_item* items, void* data,
        menu_create_callback on_create, menu_create_items_callback on_create_items,
        menu_select_callback on_select, menu_destroy_callback on_destroy);

void destroy_menu(recovery_menu* menu);

void display_menu(recovery_menu* menu);

typedef void(*file_select_callback)(char*);

/**
 * Display file selection menu
 *
 * shows a file selection menu, returns true or false as to whether you should keep displaying this
 * menu (file not selected) or break out (file selected)
 *
 * \param base_path path to start at
 * \param ext array of valid extensions to show
 * \param on_select method to call when a file is selected
 */
void display_file_select_menu(char* base_path, char** exts, file_select_callback on_select);

#endif//RECOVERY_RECOVERY_MENU_H_
