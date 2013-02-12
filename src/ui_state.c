#include "ui_state.h"

#include "err.h"
#include <ccan/array_size/array_size.h>

typedef enum {
	UI_STATE_QUERY = 0,
	UI_STATE_EDIT
} ui_state_name;

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
static void ui_state_input_query(struct fifo *);

void ui_state_init() {
	g.current = UI_STATE_QUERY;
	query_reset();
}

static void query_reset() {
	g.query.n = 0;
	g.query.prev_nl = 0;
	g.query.run = 0;
}

void ui_state_input(struct fifo *input) {
	switch(g.current) {
	case UI_STATE_QUERY:
		ui_state_input_query(input);
		break;
	default:
		err_panic(0, "invalid state: %d", g.current);
	}
}

static void ui_state_input_query(struct fifo *input) {
	size_t i, amt;
	int ch;
	
	if( (amt = fifo_amt(input)) < 1)
		return;

	for(i = 0; i < amt; i++) {
		fifo_pop(input, &ch);
		if(ch == '\n' || ch == '\r') {
			g.query.prev_nl = 1;
			g.query.run = 1;
			break;
		}
		else /* truncate at query size limit for now */
			if(g.query.n < ARRAY_SIZE(g.query.value))
				g.query.value[g.query.n++] = ch;
	} /* for i up to amt */

}

