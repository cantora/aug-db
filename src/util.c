#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>

int util_usleep(int secs, int usecs) {
	struct timeval amt;
	amt.tv_sec = secs;
	amt.tv_usec = usecs;
	
	if(select(0, NULL, NULL, NULL, &amt) != 0)
		return -1;

	return 0;
}

int util_thread_init() {
	int status, old;

	if( (status = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old)) != 0) {
		return status;
	}
	if( (status = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old)) != 0) {
		return status;
	}

	return 0;
}
