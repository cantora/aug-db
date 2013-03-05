#ifndef AUG_DB_UI_H
#define AUG_DB_UI_H

#include <stdint.h>

int ui_init();
void ui_free();
void ui_lock();
void ui_unlock();
void ui_on_cmd_key();
int ui_on_input(const uint32_t *ch);
void ui_on_dims_change(int rows, int cols);

#endif /* AUG_DB_UI_H */
