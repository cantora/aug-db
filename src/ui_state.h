#ifndef AUG_DB_UI_STATE_H
#define AUG_DB_UI_STATE_H

#include "fifo.h"

typedef enum {
	UI_STATE_QUERY = 0,
	UI_STATE_EDIT
} ui_state_name;

void ui_state_init();
void ui_state_consume(struct fifo *input);
void ui_state_query_value(const int **value, size_t *n);
void ui_state_query_value_reset();
ui_state_name ui_state_current();
int ui_state_query_run(int reset);

#endif /* AUG_DB_UI_STATE_H */