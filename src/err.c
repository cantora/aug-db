#include "err.h"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "api_calls.h"

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

