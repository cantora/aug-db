#ifndef AUG_DB_DB_H
#define AUG_DB_DB_H

#include <string.h>
#include <sqlite3.h>

struct db_query {
	sqlite3_stmt *stmt;
};

int db_init(const char *fpath);
void db_free();
void db_query_prepare(struct db_query *query, const char **queries, size_t nqueries,
		const char **tags, size_t ntags);
int db_query_step(struct db_query *query);

#endif

