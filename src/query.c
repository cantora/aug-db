#include "query.h"

#include "api_calls.h"
#include "err.h"

#include <ccan/array_size/array_size.h>

int query_clear(struct query *q) {
	int result;

	result = ( q->n != 0 || q->offset != 0 );
	q->n = 0;
	q->offset = 0;
	q->page_size = 0;

	return result;
}

void query_value(const struct query *q, const uint32_t **value, size_t *n) {
	*value = q->value;
	*n = q->n;
}

int query_delete(struct query *q) {
	if(q->n > 0) {
		q->n--;
		q->offset = 0;
		return 1;
	}

	return 0;
}

int query_offset_decr(struct query *q) {
	if(q->offset > 0) {
		q->offset -= 1;
		return 1;
	}

	return 0;
}

int query_offset_incr(struct query *q) {
	if(q->page_size > 1) {
		q->offset += 1;
		return 1;
	}

	return 0;	
}

int query_add_ch(struct query *q, uint32_t ch) {
	if(q->n < ARRAY_SIZE(q->value)) {
		q->value[q->n++] = ch;
		q->offset = 0;
		return 1;
	}

	return 0;
}

int query_first_result(struct query *q, uint8_t **data, 
		size_t *n, int *raw, int *id) {
	
	QUERY_FOREACH(q, data, n, raw, id) {
		break;
	}
}

static void query_prepare_from_value(struct query *q) {
	size_t obl;
	size_t vlen = q->n*8+1;
	/* utf-8 should be able to represent any code point in less than 8 bytes */
	uint8_t value[vlen]; 
	const uint8_t *queries[1];

	obl = encoding_wchar_to_utf8(value, vlen-1, q->value, q->n);
	value[vlen-1-obl] = '\0';
	queries[0] = value;
	db_query_prepare(&q->result, q->offset, queries, 1, NULL, 0);
}

void query_prepare(struct query *q) {

	if(q->n > 0) {
		query_prepare_from_value(q);
	}
	else
		db_query_prepare(&q->result, q->offset, NULL, 0, NULL, 0);

	q->page_size = 0;
}

int query_next(struct query *q, uint8_t **data, size_t *n, int *raw, int *id) {
	if(db_query_step(&q->result) != 0) {
		aug_log("no more results\n");
		return -1;
	}

	db_query_value(&q->result, data, n, raw, id);
	q->page_size += 1;
	return 0;
}

int query_finalize(struct query *q) {
	db_query_free(&q->result);

	/* need to return a value to use this in foreach macro */
	return 0;
}
