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
#ifndef AUG_DB_LOCK_H
#define AUG_DB_LOCK_H

#include <pthread.h>
#include "api_calls.h"

static inline int fprint_tid(const char *src, const char *func, int lineno, 
		pthread_t pt, const char *suffix, ...) {
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

	return aug_log("(0x%s)%s#%s@%d=> %s\n", buf, src, func, lineno, suf_buf);
}

#define AUG_DB_LOCK(mtx_ptr, status, msg) \
	do { \
		fprint_tid(__FILE__, __func__, __LINE__, pthread_self(), \
			":lock " stringify(mtx_ptr) ); \
		if( (status = pthread_mutex_lock(mtx_ptr)) != 0) \
			{ err_panic(status, msg); } \
	} while(0)

#define AUG_DB_UNLOCK(mtx_ptr, status, msg) \
	do { \
		fprint_tid(__FILE__, __func__, __LINE__, pthread_self(), \
			":unlock " stringify(mtx_ptr) ); \
		if( (status = pthread_mutex_unlock(mtx_ptr)) != 0) \
			{ err_panic(status, msg); } \
	} while(0)

#endif /* AUG_DB_LOCK_H */
