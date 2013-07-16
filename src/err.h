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
#ifndef AUG_DB_ERR_H
#define AUG_DB_ERR_H

#include <stdlib.h>
#include <errno.h>
#include <ccan/str/str.h>

/* these routines are meant for a non-primary thread 
 * (i.e. not aug_plugin_init or aug_plugin_free)
 * to call when a fatal error is encountered. the 
 * err_die macro will log the error message, invoke the
 * configured cleanup function, then signal the err_dispatch handler 
 */

/* change back to this at stable version */
#define err_warn(_eno, ...) \
	err_log(__FILE__, __LINE__, _eno, __VA_ARGS__)

/* strict version
 * #define err_warn(_eno, ...) \
 *	err_panic(_eno, __VA_ARGS__)
 */

#define err_panic(_eno, ...) \
	do { \
		err_log(__FILE__, __LINE__, _eno, __VA_ARGS__); \
		exit(1); \
	} while(0)

#define err_assert(_expr) \
	do { \
		if(!(_expr)) { err_panic(0, "failed assert: " stringify(_expr)); } \
	} while(0)

void err_log(const char *file, int lineno, 
					int error, const char *format, ...);

#endif
