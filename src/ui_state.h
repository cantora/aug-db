#ifndef AUG_DB_UI_STATE_H
#define AUG_DB_UI_STATE_H

#include "fifo.h"

#include <stdint.h>
#include <ccan/talloc/talloc.h>

typedef enum {
	UI_STATE_QUERY = 0,
	UI_STATE_HELP_QUERY,
	UI_STATE_EDIT
} ui_state_name;

int ui_state_init();
void ui_state_free();
int ui_state_consume(struct fifo *input);
void ui_state_query_value(const uint32_t **value, size_t *n);
/* returns true if the query was not already reset */
int ui_state_query_value_clear();
int ui_state_query_result_next(uint8_t **data, size_t *n, int *raw, int *id);
void ui_state_query_result_reset();
ui_state_name ui_state_current();

/* talloc_free(*tal_data) when done with it */
int ui_state_query_run(uint8_t **tal_data, size_t *size, int *raw, 
		uint32_t *run_ch);

int ui_state_query_foreach_result(
		int (*fn)(uint8_t *data, size_t n, int raw, int id, int i, void *user),
		void *user);

void ui_state_help_query_reset();
void ui_state_help_query_next();
size_t ui_state_help_query_remain();
void ui_state_help_query_value(const char **key, const char **desc);

void ui_state_dims_changed();
void ui_state_interact_end();

#endif /* AUG_DB_UI_STATE_H */
