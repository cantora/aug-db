#ifndef AUG_DB_DB_H
#define AUG_DB_DB_H

#include <string.h>
#include <stdint.h>
#include <sqlite3.h>
#include <ccan/talloc/talloc.h>

struct db_query {
	sqlite3_stmt *stmt;
};

int db_init(const char *fpath);
void db_free();

void db_add(const void *data, size_t bytes, int raw, const char **tags, size_t ntags);
void db_trash(int bid);

/* queries and tags are utf-8 encoded strings */
void db_query_prepare(struct db_query *query, unsigned int offset, const uint8_t **queries, 
		size_t nqueries, const uint8_t **tags, size_t ntags);

/* returns 0 on data, non-zero otherwise */
int db_query_step(struct db_query *query);

/* value will be set to a talloc'd buffer of size *size */
void db_query_value(struct db_query *query, uint8_t **value, size_t *size, 
		int *raw, int *id);
void db_query_reset(struct db_query *query);
void db_query_free(struct db_query *query);

void db_update_chosen_at(int id);

#endif

