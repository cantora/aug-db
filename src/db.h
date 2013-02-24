#ifndef AUG_DB_DB_H
#define AUG_DB_DB_H

#include <string.h>
#include <sqlite3.h>

struct db_query {
	sqlite3_stmt *stmt;
};

int db_init(const char *fpath);
void db_free();

void db_add(const void *data, size_t bytes, const char **tags, size_t ntags);

void db_query_prepare(struct db_query *query, const char **queries, size_t nqueries,
		const char **tags, size_t ntags);
int db_query_step(struct db_query *query);
/* value will be set to a talloc'd buffer of size *size */
void db_query_value(struct db_query *query, char **value, size_t *size);
void db_query_reset(struct db_query *query);
void db_query_free(struct db_query *query);

#endif

