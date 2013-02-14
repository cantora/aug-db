#ifndef AUG_DB_LOCK_H
#define AUG_DB_LOCK_H

#include <pthread.h>

#define AUG_DB_LOCK(_mtx_ptr, _status, _msg) \
	do { \
		if( (_status = pthread_mutex_lock(_mtx_ptr)) != 0) \
			{ err_panic(_status, _msg); } \
	} while(0)
#define AUG_DB_UNLOCK(_mtx_ptr, _status, _msg) \
	do { \
		if( (_status = pthread_mutex_unlock(_mtx_ptr)) != 0) \
			{ err_panic(_status, _msg); } \
	} while(0)

#endif /* AUG_DB_LOCK_H */
