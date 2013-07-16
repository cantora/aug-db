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
#ifndef AUG_DB_QUERY_H
#define AUG_DB_QUERY_H

#include "db.h"

struct query {
	uint32_t value[1024];
	/* the size of the data in value */
	size_t n;
	/* result db_query object from db.c */
	struct db_query	result;
	/* current offset into the sql query */
	unsigned int offset;
	/* the number of result items displayed on the page for 
	 * the current query result. */
	int page_size;
};

void query_init(struct query *q);
/* returns non-zero if the query wasnt already cleared */
int query_clear(struct query *q);
void query_value(const struct query *q, const uint32_t **value, size_t *n);
static inline size_t query_size(const struct query *q) { return q->n; }
/* returns non-zero if query was non-empty */
int query_delete(struct query *q);
/* returns non-zero if offset was decremented */
int query_offset_decr(struct query *q);
/* returns non-zero if offset was incremented */
int query_offset_incr(struct query *q);
/* returns non-zero if offset was non-zero */
int query_offset_reset(struct query *q);
/* returns non-zero if char was added */
int query_add_ch(struct query *q, uint32_t ch);


/* only public for use in foreach macro */
void query_prepare(struct query *q);
/* tal_data will point to a talloc'd buffer of size *n */
int query_next(struct query *q, uint8_t **tal_data, size_t *n, 
		int *raw, int *id);
int query_finalize(struct query *q);

/* returns the number of times @fn was called */
int query_foreach_result(struct query *q, 
		int (*fn)(uint8_t *data, size_t n, int raw, int id, int i, void *user),
		void *user);

/* make sure to call talloc_free(*tal_data) when done.
 * returns zero if there is valid data.
 */
int query_first_result(struct query *q, uint8_t **tal_data, 
		size_t *n, int *raw, int *id);


#endif /* AUG_DB_QUERY_H */
