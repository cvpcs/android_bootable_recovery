#ifndef RECOVERY_RECOVERY_MENU_H_
#define RECOVERY_RECOVERY_MENU_H_

void prompt_and_wait();

int get_menu_selection(char** headers, char** items, int menu_only, int initial_selection);

#endif//RECOVERY_RECOVERY_MENU_H_
