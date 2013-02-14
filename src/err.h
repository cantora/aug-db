#ifndef AUG_DB_ERR_H
#define AUG_DB_ERR_H

#include <stdlib.h>
#include <ccan/str/str.h>

/* these routines are meant for a non-primary thread 
 * (i.e. not aug_plugin_init or aug_plugin_free)
 * to call when a fatal error is encountered. the 
 * err_die macro will log the error message, invoke the
 * configured cleanup function, then signal the err_dispatch handler 
 */

/* change back to this at stable version 
 * #define err_warn(_eno, ...) \
 *	err_log(__FILE__, __LINE__, _eno, __VA_ARGS__)
 */
#define err_warn(_eno, ...) \
	err_panic(_eno, __VA_ARGS__)

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
