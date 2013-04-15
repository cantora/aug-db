#include "ui_state.h"

#include "api_calls.h"
#include "err.h"
#include "query.h"
#include "db.h"
#include "encoding.h"

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
	
	struct {
		size_t cmd;
		int escape;
	} help_query_state;
} g;

const char *const g_query_cmds[] = {
	"^C", "close the aug-db window.",
	"^G", "clear the value of the search term.",
	"^N", "select the result below the current result.",
	"^P", "select the result above the current result."
};

static int ui_state_consume_query(struct fifo *);
static int ui_state_consume_help_query(struct fifo *);

int ui_state_init() {
	g.current = UI_STATE_QUERY;
	ui_state_query_value_clear();
	ui_state_help_query_reset();
	
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
	case UI_STATE_HELP_QUERY:
		amt = ui_state_consume_help_query(input);
		break;
	default:
		err_panic(0, "invalid state: %d", g.current);
	}

	return amt;
}

static int ui_state_consume_help_query(struct fifo *input) {
	size_t amt;
	uint32_t ch;
	
	if( (amt = fifo_amt(input)) > 0) {
		fifo_pop(input, &ch);
		if(ch != 0x20 || (ui_state_help_query_remain() < 1) )
			g.current = UI_STATE_QUERY;
		/* else the ui will re-render and window_render_help_query
		 * will render the next page of help messages */
	} 

	return amt+1;
}

static int ui_state_consume_query(struct fifo *input) {
	size_t i, amt;
	uint32_t ch;
	int brk;
	
	if( (amt = fifo_amt(input)) < 1)
		return 0;

	brk = 0;
	for(i = 0; i < amt; i++) {
		fifo_pop(input, &ch);
		switch(ch) {
		case 0x08: /* ^H */
		case 0x7f: /* backspace */
			query_delete(&g.query_state.q);
			break;
		case 0x07: /* ^G */
			ui_state_query_value_clear();
			break;
		case 0x10: /* ^P */
			query_offset_decr(&g.query_state.q);
			break;
		case 0x0e: /* ^N */
			query_offset_incr(&g.query_state.q);
			break;
		case 0x03: /* ^C */
			g.query_state.run = -1;
			brk = 1; 
			break;
		case 0x1f: /* ^/ */
			aug_log("transition to help query state\n");
			ui_state_help_query_reset();
			g.current = UI_STATE_HELP_QUERY;
			brk = 1;
			break;
		default: /* truncate at query size limit for now */
			if(ch >= 0x20 && ch != 0x7f) {
				/*aug_log("added query char: 0x%04x\n", ch);*/
				if(query_add_ch(&g.query_state.q, ch) == 0)
					aug_log("exceeded max query size, query will be truncated\n"); 
			}
			else {
				g.query_state.run = 1;
				g.query_state.run_ch = ch;
				brk = 1; 
			}
		} /* switch(ch) */

		if(brk != 0) {
			brk = 0;
			break;
		}
	} /* for i up to amt */

	return i+1;
}

ui_state_name ui_state_current() {
	return g.current;
}

void ui_state_query_value(const uint32_t **value, size_t *n) {
	query_value(&g.query_state.q, value, n);
}

int ui_state_query_value_clear() {
	g.query_state.run = 0;
	return query_clear(&g.query_state.q);
}

int ui_state_query_run(uint8_t **tal_data, size_t *size, 
		int *raw, uint32_t *run_ch) {
	int result, id;
	
	result = g.query_state.run;

	if(result > 0) {
		*run_ch = g.query_state.run_ch;
		if(query_first_result(&g.query_state.q, tal_data, size, raw, &id) != 0)
			return 0; /* no results, dont run. */
		db_update_chosen_at(id);
		ui_state_query_value_clear();
	}
	else if(result < 0) {
		g.query_state.run = 0;
		*size = 0;
	}

	return result;
}

int ui_state_query_foreach_result(
		int (*fn)(uint8_t *data, size_t n, int raw, int id, int i, void *user),
		void *user) {

	return query_foreach_result(&g.query_state.q, fn, user);
}

void ui_state_help_query_reset() {
	g.help_query_state.cmd = 0;
	g.help_query_state.escape = 0;
}

void ui_state_help_query_next() {
	g.help_query_state.cmd++;
}

/* how many query help commands left to display? */
size_t ui_state_help_query_remain() {
	size_t size = ARRAY_SIZE(g_query_cmds) >> 1;
	size_t index = g.help_query_state.cmd;

	if(index >= size)
		return 0;

	/*aug_log("help query remaining %d\n", size - index);*/
	return size - index;
}

/* only call this is *_remain is > 0 */
void ui_state_help_query_value(const char **key, const char **desc) {
	int index = g.help_query_state.cmd*2;

	/*aug_log("help query value at %d\n", index);*/
	*key = g_query_cmds[index];
	*desc = g_query_cmds[index+1];
}

void ui_state_dims_changed() {
	switch(g.current) {
	case UI_STATE_HELP_QUERY:
		ui_state_help_query_reset();
		break;
	default:
		; /*nothing*/
	}
}

void ui_state_interact_end() {
	switch(g.current) {
	case UI_STATE_HELP_QUERY:
		g.current = UI_STATE_QUERY;
		break;
	default:
		; /*nothing*/
	}
}