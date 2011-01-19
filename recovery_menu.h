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
recovery_menu_item* create_menu_item_checkbox(int id, const char* title, int checked);
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
 * \param headers A null-terminated list of headers
 * \param items A null-terminated list of items
 * \param data An optional data object to be passed throughout the callback sequence
 * \param on_create onCreate callback
 * \param on_create_items onCreateItems callback
 * \param on_select onSelect callback
 * \param on_destroy onDestroy callback
 *
 * \return A pointer to the recovery_menu structure.
 *
 * \note headers will be serially copied subsequently free()'d in
 *       any later call to destroy_menu(), data will be accessed directly and should
 *       be managed yourself, items will be accessed directly and consequently destroyed
 *       when calling destroy_menu(), so do not free them yourself
 */
recovery_menu* create_menu(char** headers, recovery_menu_item** items, void* data,
        menu_create_callback on_create, menu_create_items_callback on_create_items,
        menu_select_callback on_select, menu_destroy_callback on_destroy);

void destroy_menu(recovery_menu* menu);

void display_menu(recovery_menu* menu);

/**
 * Display a basic item selection menu
 *
 * shows a list of items, and returns the id of the selected one
 *
 * \param items the list of items to display
 * \param active the id of the "active" item, or an invalid id if none are active
 *
 * \return the id of the selected item
 *
 * \note the items you pass in will be automatically free()'d by this function, so you
 *       should not do so yourself
 */
int display_item_select_menu(recovery_menu_item** items, int active);

/**
 * Display file selection menu
 *
 * shows a file selection menu, and returns the selected file
 *
 * \param base_path path to start at
 * \param ext array of valid extensions to show
 *
 * \return NULL if no file selected, or a pointer to a string containing the full path
 *         of the selected file, which may be free()'d when you are done with it
 */
char* display_file_select_menu(char* base_path, char** exts);

#endif//RECOVERY_RECOVERY_MENU_H_
