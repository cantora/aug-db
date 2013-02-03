#ifndef AUG_DB_ERR_H
#define AUG_DB_ERR_H

#include "err_dispatch.h"

#include <stdlib.h>

/* these routines are meant for a non-primary thread 
 * (i.e. not aug_plugin_init or aug_plugin_free)
 * to call when a fatal error is encountered. the 
 * err_die macro will log the error message, invoke the
 * configured cleanup function, then signal the err_dispatch handler 
 */

#define err_warn(_eno, ...) \
	err_log(__FILE__, __LINE__, _eno, __VA_ARGS__)

#define err_panic(_eno, ...) \
	do { \
		err_log(__FILE__, __LINE__, _eno, __VA_ARGS__); \
		exit(1); \
	} while(0)

#define err_die(_eno, ...) \
	do { \
		err_log(__FILE__, __LINE__, _eno, __VA_ARGS__); \
		err_call_cleanup_fn(_eno); \
		err_dispatch_signal(_eno); \
		pthread_exit((void *)1); \
	} while(0)

/* these should be called by the main thread */
int err_init();
int err_free();

/* meant for non-primary threads */
void err_set_cleanup_fn(void (*cleanup_fn)(int error) );
void err_call_cleanup_fn(int error);
void err_log(const char *file, int lineno, 
					int error, const char *format, ...);

#endif
