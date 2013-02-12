#include "ui_state.h"

#include "api_calls.h"
#include "err.h"
#include <ccan/array_size/array_size.h>

static struct {
	ui_state_name current;
	struct {
		int prev_nl;
		int value[1024];
		size_t n;
		int run;
	} query;
} g;

static void query_reset();
static void ui_state_consume_query(struct fifo *);

void ui_state_init() {
	g.current = UI_STATE_QUERY;
	query_reset();
}

static void query_reset() {
	g.query.n = 0;
	g.query.prev_nl = 0;
	g.query.run = 0;
}

void ui_state_consume(struct fifo *input) {
	switch(g.current) {
	case UI_STATE_QUERY:
		ui_state_consume_query(input);
		break;
	default:
		err_panic(0, "invalid state: %d", g.current);
	}
}

static void ui_state_consume_query(struct fifo *input) {
	size_t i, amt;
	int ch, brk;
	
	if( (amt = fifo_amt(input)) < 1)
		return;

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
		case 0x08: /* ^H fall through */
		case 0x7f: /* backspace */
			if(g.query.n > 0)
				g.query.n--;
			g.query.prev_nl = 0;
			break;
		case 0x07: /* ^G */
			g.query.n = 0;
			g.query.prev_nl = 0;
			break;
		default: /* truncate at query size limit for now */
			g.query.prev_nl = 0;
			if(ch >= 0x20 && ch <= 0x7e) {
				aug_log("added query char: 0x%04x\n", ch);
				if(g.query.n < ARRAY_SIZE(g.query.value))
					g.query.value[g.query.n++] = ch;
			}
			else
				aug_log("dropped unprintable character 0x%04x\n", ch);
		} /* switch(ch) */

		if(brk != 0) {
			brk = 0;
			break;
		}
	} /* for i up to amt */

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
