#ifndef AUG_DB_LOCK_H
#define AUG_DB_LOCK_H

#include <pthread.h>
#include "api_calls.h"

static inline int fprint_tid(const char *src, const char *func, int lineno, 
		FILE *f, pthread_t pt, const char *suffix, ...) {
	va_list args;
	size_t k;
#define FPRINT_TID_BUFLEN ( sizeof(pthread_t)*2 + 1 )
	unsigned char *ptc;
	char buf[FPRINT_TID_BUFLEN];
	char suf_buf[512];

	ptc = (unsigned char*)(void*)(&pt);
	for(k = 0; k < sizeof(pthread_t); k++) 
		snprintf(buf+k*2, 3, "%02x", (unsigned)(ptc[k]));

	buf[FPRINT_TID_BUFLEN-1] = '\0';
#undef FPRINT_TID_BUFLEN

	va_start(args, suffix);
	vsnprintf(suf_buf, sizeof(suf_buf), suffix, args);
	va_end(args);

	return fprintf(f, "(0x%s)%s#%s@%d=> %s\n", buf, src, func, lineno, suf_buf);
}

#define AUG_DB_LOCK(_mtx_ptr, _status, _msg) \
	do { \
		fprint_tid(__FILE__, __func__, __LINE__, stderr, pthread_self(), \
			":lock " stringify(_mtx_ptr) ); \
		if( (_status = pthread_mutex_lock(_mtx_ptr)) != 0) \
			{ err_panic(_status, _msg); } \
	} while(0)

#define AUG_DB_UNLOCK(_mtx_ptr, _status, _msg) \
	do { \
		fprint_tid(__FILE__, __func__, __LINE__, stderr, pthread_self(), \
			":unlock " stringify(_mtx_ptr) ); \
		if( (_status = pthread_mutex_unlock(_mtx_ptr)) != 0) \
			{ err_panic(_status, _msg); } \
	} while(0)

#endif /* AUG_DB_LOCK_H */
