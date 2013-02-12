#ifndef AUG_DB_UI_STATE_H
#define AUG_DB_UI_STATE_H

#include "fifo.h"

void ui_state_init();
void ui_state_input(struct fifo *input);

#endif /* AUG_DB_UI_STATE_H */