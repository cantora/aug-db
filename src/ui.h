#ifndef AUG_DB_UI_H
#define AUG_DB_UI_H

int ui_init();
int ui_free();
int ui_lock();
int ui_unlock();
int ui_on_cmd_key();
int ui_on_input(const int *ch);

#endif /* AUG_DB_UI_H */