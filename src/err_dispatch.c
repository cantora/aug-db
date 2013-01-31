#include "err_dispatch.h"

#include "util.h"
#include "err.h"

#include <pthread.h>


/* waits for a signal from some thread that encountered 
 * a fatal error, then invokes a callback function meant
 * to clean up, and finally calls pthread_exit. this exists
 * so that the thread that encountered a fatal error isnt
 * the thread which is doing the clean up of all the other
 * threads in the program, thus there is no worry of some
 * thread joining itself or something like that.
 */

static struct {
	pthread_t id;
	pthread_mutex_t mtx;
	pthread_cond_t cond;
	void (*cleanup)(int error);
	int ready;
	int ran;
	int error;
} g;

static void *err_dispatch_run(void *);

int err_dispatch_init(void (*cleanup)(int error) ) {
	int status;

	g.cleanup = cleanup;
	g.ran = 0;
	g.ready = 0;
	g.error = 0;

	if( (status = pthread_mutex_init(&g.mtx, NULL) ) != 0)
		return status;
	if( (status = pthread_cond_init(&g.cond, NULL) ) != 0)
		goto cleanup_mtx;
	if( (status = pthread_create(&g.id, NULL, err_dispatch_run, NULL) ) != 0)
		goto cleanup_cond;
	if( (status = util_thread_init()) != 0) {
		pthread_cancel(g.id);
		goto cleanup_cond;
	}

	/* we want the dispatch
	 * thread to be up and running before any other threads might
	 * signal it. */
	while(g.ready == 0)
		util_usleep(0, 10000);

	return 0;

cleanup_cond:
	pthread_cond_destroy(&g.cond);
cleanup_mtx:
	pthread_mutex_destroy(&g.mtx);

	return status;
}

/* err_dispatch should not be freed before all 
 * other threads are dead (besides dispatch run
 * thread), since one of those threads could 
 * throw an error and try to lock the destroyed
 * mutex by calling err_dispatch_signal */
int err_dispatch_free() {
	int status;

	if( (status = pthread_mutex_lock(&g.mtx) != 0) ) {
		err_warn(status, "failed mutex lock while freeing dispatch");
		return status;
	}
	if(g.ran == 0)
		if( (status = pthread_cancel(g.id) ) != 0 ) {
			err_warn(status, "pthread cancel failed while freeing dispatch");
			return status;
		}
	if( (status = pthread_mutex_unlock(&g.mtx) != 0) ) {
		err_warn(status, "failed mutex unlock while freeing dispatch");
		return status;
	}
	if( (status = pthread_join(g.id, NULL)) != 0) {
		err_warn(status, "pthread join failed while freeing dispatch");
		return status;
	}
	
	pthread_mutex_destroy(&g.mtx);
	pthread_cond_destroy(&g.cond);

	return 0;
}

void err_dispatch_signal(int error) {
	int status;

	if( (status = pthread_mutex_lock(&g.mtx) != 0) ) 
		err_panic(status, "failed mutex lock while signaling dispatch");

	g.error = error;
	if( (status = pthread_cond_signal(&g.cond)) != 0) 
		err_panic(status, "failed to signal dispatch");

	if( (status = pthread_mutex_unlock(&g.mtx) != 0) ) 
		err_panic(status, "failed mutex unlock while signaling dispatch");

}

static void *err_dispatch_run(void *user) {
	int status;
	(void)(user);

	if( (status = pthread_mutex_lock(&g.mtx)) != 0)
		err_panic(status, "fatal: err_dispatch thread failed to lock");

	g.ready = 1;
	/* unlocks g.mtx after starting to wait. relocks g.mtx
	 * just after waking up from a signal. */
	if( (status = pthread_cond_wait(&g.cond, &g.mtx)) != 0)
		err_panic(status, "fatal: error in condition wait");
	/* keep g.mtx locked so this thread wont get canceled by err_dispatch_free */
	g.ran = 1;

	/* call cleanup function */
	(*g.cleanup)(g.error);
	pthread_mutex_unlock(&g.mtx);
	return 0;
}

