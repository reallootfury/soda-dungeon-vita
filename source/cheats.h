#ifndef SODA_CHEATS_H
#define SODA_CHEATS_H

#include <stdint.h>

#define CHEAT_MENU_COUNT 22

void cheats_runtime_init(void);
void cheats_request_debug_menu(void);
void cheats_poll(void);
uint32_t cheats_handle_input(uint32_t pressed, uint32_t held);
int cheats_menu_is_open(void);
int cheats_menu_selection(void);
unsigned int cheats_menu_generation(void);
int cheats_flag_cheap(void);
int cheats_flag_crit(void);
int cheats_flag_auto(void);
int cheats_flag_patrons(void);
int cheats_flag_registration(void);

#endif
