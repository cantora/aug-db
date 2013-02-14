#ifndef AUG_DB_UI_H
#define AUG_DB_UI_H

int ui_init();
void ui_free();
void ui_lock();
void ui_unlock();
void ui_on_cmd_key();
int ui_on_input(const int *ch);

#endif /* AUG_DB_UI_H */
