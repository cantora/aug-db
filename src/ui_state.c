#include "ui_state.h"

#include "api_calls.h"
#include "err.h"
#include "query.h"
#include "db.h"

#include <ccan/array_size/array_size.h>

static struct {
	ui_state_name current;
	struct {
		/* flag which signals whether the current query
		 * value has been selected by the user. run > 0
		 * means the user selected the first result in 
		 * the current query result. run < 0 means the
		 * user wants to exit interaction with aug-db. */
		int run;
		/* the input character used to select the result item.
		 * when the result is written to the terminal it will
		 * be terminated with this character. */
		uint32_t run_ch; 
		struct query q;
	} query_state;
} g;

static int ui_state_consume_query(struct fifo *);

int ui_state_init() {
	g.current = UI_STATE_QUERY;
	ui_state_query_value_clear();

	if(encoding_init() != 0)
		return -1;

	return 0;
}

void ui_state_free() {
	encoding_free();
}

int ui_state_consume(struct fifo *input) {
	int amt;
	switch(g.current) {
	case UI_STATE_QUERY:
		amt = ui_state_consume_query(input);
		break;
	default:
		err_panic(0, "invalid state: %d", g.current);
	}

	return amt;
}

static int ui_state_consume_query(struct fifo *input) {
	size_t i, amt;
	uint32_t ch;
	int brk, nop;
	
	if( (amt = fifo_amt(input)) < 1)
		return 0;

	brk = 0;
	nop = 1;
	for(i = 0; i < amt; i++) {
		fifo_pop(input, &ch);
		switch(ch) {
		case 0x08: /* ^H */
		case 0x7f: /* backspace */
			if(query_delete(&g.query.q) != 0)
				nop = 0;
			break;
		case 0x07: /* ^G */
			if(ui_state_query_value_clear())
				nop = 0;
			break;
		case 0x10: /* ^P */
			if(query_offset_decr(&g.query.q) != 0)
				nop = 0;
			break;
		case 0x0e: /* ^N */
			if(query_offset_incr(&g.query.q) != 0)
				nop = 0;
			break;
		case 0x03: /* ^C */
			g.query.run = -1;
			brk = 1; 
			break;

		default: /* truncate at query size limit for now */
			if(ch >= 0x20 && ch != 0x7f) {
				/*aug_log("added query char: 0x%04x\n", ch);*/
				if(query_add_ch(&g.query.q, ch) != 0)
					nop = 0;
				else
					aug_log("exceeded max query size, query will be truncated\n"); 
			}
			else {
				g.query.run = 1;
				g.query.run_ch = ch;
				brk = 1; 
			}
		} /* switch(ch) */

		if(brk != 0) {
			brk = 0;
			break;
		}
	} /* for i up to amt */

	/*
	if(nop == 0) {
		db_query_free(&g.query.result);
		prepare_query();
	}
	else {
		db_query_reset(&g.query.result);
		g.query.page_size = 0;
	}*/

	return i+1;
}

ui_state_name ui_state_current() {
	return g.current;
}

int ui_state_query_value_clear() {
	g.query.run = 0;
	return query_clear(&g.query.q);
}

int ui_state_query_run(uint8_t **data, size_t *size, 
		int *raw, uint32_t *run_ch) {
	int result, id;
	
	result = g.query.run;

	if(result > 0) {
		*run_ch = g.query.run_ch;
		if(query_first_result(data, size, raw, &id) != 0)
			return 0; /* no results, dont run. */
		db_update_chosen_at(id);
		ui_state_query_value_clear();
	}
	else if(result < 0) {
		g.query.run = 0;
		*size = 0;
	}

	return result;
}
