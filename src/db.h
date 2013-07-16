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

