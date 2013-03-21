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

int query_clear(struct query *q);
void query_value(const struct query *q, const uint32_t **value, size_t *n);
inline size_t query_size(const struct query *q) { return q->n; }
/* returns non-zero if query was non-empty */
int query_delete(struct query *q);
/* returns non-zero if offset was decremented */
int query_offset_decr(struct query *q);
/* returns non-zero if offset was incremented */
int query_offset_incr(struct query *q);

/* only public for use in foreach macro */
void query_prepare(struct query *q);
int query_next(struct query *q, uint8_t **data, size_t *n, int *raw, int *id);
int query_finalize(struct query *q);

/* this is macro is not thread safe. also dont 'return' out of it. */
static int query_foreach_i;
#define QUERY_FOREACH(query_p, data_pp, n_p, raw_p, id_p) \
	for(	query_foreach_i = 0; query_prepare(query_p); \
			i < 1; \
			query_finalize(query_p) || i++) \
		for(/* nothing */; \
			query_next(query_p, data_pp, n_p, raw_p, id_p) == 0; \
			/* nothing */; \
		)

int query_first_result(struct query *q, uint8_t **data, 
		size_t *n, int *raw, int *id);

#endif /* AUG_DB_QUERY_H */
