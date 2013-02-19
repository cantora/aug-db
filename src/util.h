#ifndef AUG_DB_UTIL_H
#define AUG_DB_UTIL_H

#include <wordexp.h>

int util_usleep(int secs, int usecs);
int util_thread_init();
int util_expand_path(const char *path, wordexp_t *exp);

#endif /* AUG_DB_UTIL_H */
