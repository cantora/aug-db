/* 
 * Copyright 2013 anthony cantor
 * This file is part of aug-db.
 *
 * aug-db is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * aug-db is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with aug-db.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "query.h"

#include "aug_api.h"
#include "err.h"
#include "encoding.h"

#include <ccan/array_size/array_size.h>

void query_init(struct query *q) {
	query_clear(q);
}

int query_clear(struct query *q) {
	int result;

	result = ( q->n != 0 || q->offset != 0 );
	q->n = 0;
	q->offset = 0;
	q->page_size = 0;

	return result;
}

void query_value(const struct query *q, const uint32_t **value, size_t *n) {
	*n = q->n;
	if(*n > 0) 
		*value = q->value;
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

int query_offset_reset(struct query *q) {
	if(q->offset != 0) {
		q->offset = 0;
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

/*int query_first_result(struct query *q, uint8_t **data, 
		size_t *n, int *raw, int *id) {
	
	QUERY_FOREACH(q, data, n, raw, id) {
		break;
	}
}*/

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

int query_next(struct query *q, uint8_t **tal_data, size_t *n, 
		int *raw, int *id) {
	if(db_query_step(&q->result) != 0) {
		aug_log("no more results\n");
		return -1;
	}

	db_query_value(&q->result, tal_data, n, raw, id);
	q->page_size += 1;
	return 0;
}

int query_finalize(struct query *q) {
	db_query_free(&q->result);

	/* need to return a value to use this in foreach macro */
	return 0;
}

int query_foreach_result(struct query *q, 
		int (*fn)(uint8_t *data, size_t n, int raw, int id, int i, void *user),
		void *user) {
	uint8_t *data;
	size_t n;
	int raw, id, i, status;

	query_prepare(q);

	i = 0;
	while(query_next(q, &data, &n, &raw, &id) == 0) {
		status = (*fn)(data, n, raw, id, i++, user);
		talloc_free(data);
		if(status != 0)
			break;
	}
	query_finalize(q);

	return i;
}

int query_first_result(struct query *q, uint8_t **tal_data, 
		size_t *n, int *raw, int *id) {
	int result;

	query_prepare(q);
	result = query_next(q, tal_data, n, raw, id);
	query_finalize(q);

	return result;
}
