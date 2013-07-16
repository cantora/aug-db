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
#ifndef AUG_DB_UTIL_H
#define AUG_DB_UTIL_H

#include <wordexp.h>
#include <ccan/str_talloc/str_talloc.h>
#include <ccan/talloc/talloc.h>

int util_usleep(int secs, int usecs);
int util_thread_init();
int util_expand_path(const char *path, wordexp_t *exp);
char *util_tal_join(const void *ctx, char **strings, 
		const char *delim);
char *util_tal_multiply(const void *ctx, const char *s, 
		const char *delim, size_t n);


#endif /* AUG_DB_UTIL_H */
