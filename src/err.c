#include "err.h"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "api_calls.h"

#include <pthread.h>

pthread_key_t g_err_cleanup_key;

int err_init() {
	if(pthread_key_create(&g_err_cleanup_key, NULL) != 0)
		return -1;

	return 0;
}

int err_free() {
	if(pthread_key_delete(g_err_cleanup_key) != 0)
		return -1;

	return 0;
}

void err_set_cleanup_fn(void (*cleanup_fn)(int error) ) {
	if(pthread_setspecific(g_err_cleanup_key, cleanup_fn) != 0)
		err_panic(0, "failed to set err cleanup function");
}

void err_call_cleanup_fn(int error) {
	void (*cleanup_fn)(int error);

	if( (cleanup_fn = pthread_getspecific(g_err_cleanup_key) ) != NULL)
		(*cleanup_fn)(error);
}

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

	aug_log("(%s:%d) %s%s%s\n", file, lineno, buf, 
		(error != 0)? ": " : "",
		(error != 0)? strerror(error) : ""
	);

	return;
log_fail:
	aug_log("failed to ouput log information\n");
}

