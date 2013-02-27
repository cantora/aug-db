#include "ui_state.h"

#include "api_calls.h"
#include "err.h"
#include "db.h"

#include <ccan/array_size/array_size.h>

static struct {
	ui_state_name current;
	struct {
		int prev_nl;
		int value[1024];
		size_t n;
		int run;
		struct db_query	result;
	} query;
} g;

static void query_reset(int);
static int ui_state_consume_query(struct fifo *);

void ui_state_init() {
	g.current = UI_STATE_QUERY;
	query_reset(1);
}

void ui_state_free() {
	db_query_free(&g.query.result);
}

static void query_reset(int init) {
	
	g.query.n = 0;
	g.query.prev_nl = 0;
	g.query.run = 0;
	if(init == 0)
		db_query_free(&g.query.result);
	db_query_prepare(&g.query.result, NULL, 0, NULL, 0);
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
	int ch, brk;
	char value[ARRAY_SIZE(g.query.value)+1];
	const char *queries[1];
	
	if( (amt = fifo_amt(input)) < 1)
		return 0;

	brk = 0;
	for(i = 0; i < amt; i++) {
		fifo_pop(input, &ch);
		switch(ch) {
		case '\n': /* fall through */
		case '\r':
			if(g.query.prev_nl != 0)
				continue;

			g.query.prev_nl = 1;
			g.query.run = 1;
			brk = 1;
			break;
		case 0x0107: /* osx delete */
		case 0x08: /* ^H */
		case 0x7f: /* backspace */
			if(g.query.n > 0)
				g.query.n--;
			g.query.prev_nl = 0;
			break;
		case 0x014a:
		case 0x07: /* ^G */
			g.query.n = 0;
			g.query.prev_nl = 0;
			break;
		default: /* truncate at query size limit for now */
			g.query.prev_nl = 0;
			if(ch >= 0x20 && ch <= 0x7e) {
				/*aug_log("added query char: 0x%04x\n", ch);*/
				if(g.query.n < ARRAY_SIZE(g.query.value))
					g.query.value[g.query.n++] = ch;
				else
					aug_log("exceeded max query size, query will be truncated\n");
			}
			else
				aug_log("dropped unprintable character 0x%04x\n", ch);
		} /* switch(ch) */

		if(brk != 0) {
			brk = 0;
			break;
		}
	} /* for i up to amt */

	db_query_free(&g.query.result);

	if(g.query.n > 0) {	
		for(i = 0; i < g.query.n; i++) 
			value[i] = (char) g.query.value[i];
		value[i] = '\0';
		queries[0] = value;
		db_query_prepare(&g.query.result, queries, 1, NULL, 0);
	}
	else
		db_query_prepare(&g.query.result, NULL, 0, NULL, 0);

	return i+1;
}

ui_state_name ui_state_current() {
	return g.current;
}

void ui_state_query_value(const int **value, size_t *n) {
	*value = g.query.value;
	*n = g.query.n;
}

void ui_state_query_value_reset() {
	g.query.n = 0;
}

int ui_state_query_run(int reset) {
	int result;
	
	result = g.query.run;
	if(reset)
		g.query.run = 0;

	return result;
}

int ui_state_query_result_next(uint8_t **data, size_t *n) {
	if(db_query_step(&g.query.result) != 0) {
		aug_log("ui_state: no more results\n");
		return -1;
	}

	db_query_value(&g.query.result, data, n);
	return 0;
}

void ui_state_query_result_reset() {
	db_query_reset(&g.query.result);
}
