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
#include "err.h"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "aug_api.h"

void err_log(const char *file, int lineno, 
					int error, const char *format, ...) {
	va_list args;
	size_t buflen = 1024;
	char buf[buflen];
	int result;

	va_start(args, format);
	result = vsnprintf(buf, buflen, format, args);
	va_end(args);
	if(result <= 0)
		goto log_fail;

	aug_log("aug-db ERROR (%s:%d) %s%s%s\n", file, lineno, buf, 
		(error != 0)? ": " : "",
		(error != 0)? strerror(error) : ""
	);

	return;
log_fail:
	aug_log("failed to ouput log information\n");
}

