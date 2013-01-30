#include "ui.h"

#include "err.h"
#include "util.h"

#include <pthread.h>

/* this module represents the user interface thread and the functions
 * that other threads can call to interface with the 
 * user interface thread.
 * ui_t_* functions are expected to only be called by
 * the ui thread */

extern void aug_plugin_err_cleanup(int);

static struct {
	pthread_t tid;
	pthread_mutex_t mtx;
	int shutdown;
	ERR_MEMBERS;
} g;

static void *ui_t_run(void *);

static void ui_err_cleanup(int error) {
	/* only cleanup essential stuff here, which is nothing right now */
	aug_plugin_err_cleanup(error);
}

int ui_init() {
	err_init(&g);
	err_set_cleanup_fn(&g, ui_err_cleanup);

	g.shutdown = 0;
	if(pthread_mutex_init(&g.mtx, NULL) != 0)
		return -1;
	if(pthread_create(&g.tid, NULL, ui_t_run, NULL) != 0)
		return -1;

	return 0;
}

/* this should not be called by ui thread */
int ui_free() {

	if(ui_lock() != 0)
		return -1;
	if(g.shutdown != 0)
		goto unlock;
	g.shutdown = 1;
unlock:
	if(ui_unlock() != 0)
		return -1;
	if(pthread_join(g.tid, NULL) != 0)
		return -1;

#ifdef AUG_DB_DEBUG
	int status;
	if( (status = pthread_mutex_destroy(&g.mtx)) != 0)
		err_panic(status, "failed to destroy ui mutex");
#else
	/* an error here isnt fatal */
	pthread_mutex_destroy(&g.mtx);
#endif

	return 0;
}

int ui_lock() {
	return pthread_mutex_lock(&g.mtx);
}

int ui_unlock() {
	return pthread_mutex_unlock(&g.mtx);
}

static void ut_t_lock() {
	int status;

	if( (status = pthread_mutex_lock(&g.mtx) ) != 0)
		err_die(&g, status, 
			"the ui thread encountered error while trying to lock");
}

static void ut_t_unlock() {
	int status;

	if( (status = pthread_mutex_unlock(&g.mtx) ) != 0)
		err_die(&g, status, 
			"the ui thread encountered error while trying to unlock");
}

static void *ui_t_run(void *user) {
	(void)(user);

	while(1) {
		ut_t_lock(); {
			if(g.shutdown != 0) {
				ut_t_unlock();
				break;
			}
		} ut_t_unlock();

		util_usleep(0, 500000);
	}

	return 0;
}

