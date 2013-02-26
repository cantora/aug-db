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
