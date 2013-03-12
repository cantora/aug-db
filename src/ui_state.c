#include "ui_state.h"

#include "api_calls.h"
#include "err.h"
#include "db.h"

#include <iconv.h>
#include <ccan/array_size/array_size.h>

static struct {
	ui_state_name current;
	struct {
		uint32_t value[1024];
		/* the size of the data in value */
		size_t n;
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
		/* result db_query object from db.c */
		struct db_query	result;
		/* current offset into the sql query */
		unsigned int offset;
		/* the number of result items displayed on the page for 
		 * the current query result. */
		int page_size;
	} query;
	iconv_t cd;
} g;

static int ui_state_consume_query(struct fifo *);
static void prepare_query();

int ui_state_init() {
	g.current = UI_STATE_QUERY;
	ui_state_query_value_reset();

	if( (g.cd = iconv_open("UTF8", "WCHAR_T")) == ((iconv_t) -1) )
		return -1;

	prepare_query();

	return 0;
}

void ui_state_free() {
	db_query_free(&g.query.result);

	if(iconv_close(g.cd) != 0)
		err_warn(errno, "failed to close iconv descriptor");
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

static void prepare_query() {
	size_t status, ibl, obl;
	size_t vlen = g.query.n*8+1;
	/* utf-8 should be able to represent any code point in less than 8 bytes */
	uint8_t value[vlen]; 
	const uint8_t *queries[1];
	char *ip, *op;

	if(g.query.n > 0) {
		ip = (char *) g.query.value;
		op = (char *) value;
		ibl = g.query.n*sizeof(uint32_t);
		obl = vlen-1;

		if( (status = iconv(g.cd, &ip, &ibl, &op, &obl)) == ((size_t) -1) )
			err_panic(errno, "failed to convert query value to utf-8");

		value[vlen-1-obl] = '\0';
		queries[0] = value;
		db_query_prepare(&g.query.result, g.query.offset, queries, 1, NULL, 0);
	}
	else
		db_query_prepare(&g.query.result, g.query.offset, NULL, 0, NULL, 0);

	g.query.page_size = 0;
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
			if(g.query.n > 0) {
				g.query.n--;
				g.query.offset = 0;
				nop = 0;
			}
			break;
		case 0x07: /* ^G */
			if(ui_state_query_value_reset())
				nop = 0;
			break;
		case 0x10: /* ^P */
			if(g.query.offset > 0) {
				g.query.offset -= 1;
				nop = 0;
			}
			break;
		case 0x0e: /* ^N */
			if(g.query.page_size > 1) {
				g.query.offset += 1;
				nop = 0;
			}
			break;
		case 0x03: /* ^C */
			g.query.run = -1;
			brk = 1; 
			break;

		default: /* truncate at query size limit for now */
			if(ch >= 0x20 && ch != 0x7f) {
				/*aug_log("added query char: 0x%04x\n", ch);*/
				if(g.query.n < ARRAY_SIZE(g.query.value)) {
					g.query.value[g.query.n++] = ch;
					g.query.offset = 0;
					nop = 0;
				}
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

	if(nop == 0) {
		db_query_free(&g.query.result);
		prepare_query();
	}
	else {
		db_query_reset(&g.query.result);
		g.query.page_size = 0;
	}

	return i+1;
}

ui_state_name ui_state_current() {
	return g.current;
}

void ui_state_query_value(const uint32_t **value, size_t *n) {
	*value = g.query.value;
	*n = g.query.n;
}

int ui_state_query_value_reset() {
	int result;

	result = ( g.query.n != 0 || g.query.offset != 0 );

	g.query.n = 0;
	g.query.offset = 0;
	g.query.run = 0;
	g.query.page_size = 0;

	return result;
}

int ui_state_query_run(uint8_t **data, size_t *size, 
		int *raw, uint32_t *run_ch) {
	int result, id;
	
	result = g.query.run;

	if(result > 0) {
		*run_ch = g.query.run_ch;
		ui_state_query_result_reset();
		if(ui_state_query_result_next(data, size, raw, &id) != 0)
			return 0; /* no results, dont run. */
		db_update_chosen_at(id);
		ui_state_query_value_reset();
		db_query_free(&g.query.result);
		prepare_query();
	}
	else if(result < 0) {
		g.query.run = 0;
		*size = 0;
	}

	return result;
}

int ui_state_query_result_next(uint8_t **data, size_t *n, int *raw, int *id) {
	if(db_query_step(&g.query.result) != 0) {
		aug_log("ui_state: no more results\n");
		return -1;
	}

	db_query_value(&g.query.result, data, n, raw, id);
	g.query.page_size += 1;
	return 0;
}

void ui_state_query_result_reset() {
	db_query_reset(&g.query.result);
}
