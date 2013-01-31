#include "ui.h"

#include "api_calls.h"
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
	pthread_cond_t wakeup;
	int shutdown;
	int ready;
	int error;
	ERR_MEMBERS;
} g;

static struct {
	int mtx_locked;
} t_g;

static void *ui_t_run(void *);

/* only cleanup essential stuff here, ui_free will
 * be called later
 */
static void ui_err_cleanup(int error) {
	int status;
	(void)(error);

	g.error = 1;
	/* cant call ui_t_unlock here or things could
	 * get recursive */
	if(t_g.mtx_locked != 0)
		if( (status = pthread_mutex_unlock(&g.mtx)) != 0)
			err_panic(status, "fatal: cant unlock mutex, thread lock would be imminent");
}

int ui_init() {
	err_init(&g);
	err_set_cleanup_fn(&g, ui_err_cleanup);

	g.shutdown = 0;
	g.error = 0;
	g.ready = 0;
	if(pthread_mutex_init(&g.mtx, NULL) != 0)
		return -1;
	if(pthread_cond_init(&g.wakeup, NULL) != 0)
		goto cleanup_mtx;
	if(pthread_create(&g.tid, NULL, ui_t_run, NULL) != 0)
		goto cleanup_cond;

	while(g.ready == 0)
		util_usleep(0, 10000);

	return 0;

cleanup_cond:
	pthread_cond_destroy(&g.wakeup);
cleanup_mtx:
	pthread_mutex_destroy(&g.mtx);

	return -1;
}

/* this should not be called by ui thread */
int ui_free() {
	int s;
	aug_log("ui free\n");
	if(g.error == 0) {
		aug_log("kill ui thread\n");
		if(ui_lock() != 0)
			return -1;
		g.shutdown = 1;
		if(pthread_cond_signal(&g.wakeup) != 0) {
			ui_unlock();
			return -1;
		}
		if(ui_unlock() != 0)
			return -1;
	}

	aug_log("join ui thread\n");
	if( (s = pthread_join(g.tid, NULL)) != 0) {
		aug_log("join error: %d\n", s);
		return -1;
	}

	aug_log("ui thread dead\n");
#ifdef AUG_DB_DEBUG
	int status;
	if( (status = pthread_cond_destroy(&g.wakeup)) != 0)
		err_panic(status, "failed to destroy ui condition");
	if( (status = pthread_mutex_destroy(&g.mtx)) != 0)
		err_panic(status, "failed to destroy ui mutex");
#else
	/* an error here isnt fatal */
	pthread_cond_destroy(&g.wakeup);
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

static void ui_t_lock() {
	int status;

	if( (status = pthread_mutex_lock(&g.mtx) ) != 0)
		err_die(&g, status, 
			"the ui thread encountered error while trying to lock");
	t_g.mtx_locked = 1;
}

static void ui_t_unlock() {
	int status;

	if( (status = pthread_mutex_unlock(&g.mtx) ) != 0)
		err_die(&g, status, 
			"the ui thread encountered error while trying to unlock");
	t_g.mtx_locked = 0;
}

static void *ui_t_run(void *user) {
	int status;
	(void)(user);

	t_g.mtx_locked = 0;

	ui_t_lock(); 
	g.ready = 1;

	while(1) {
		if( (status = pthread_cond_wait(&g.wakeup, &g.mtx)) != 0)
			err_die(&g, status, "error in condition wait");

		if(g.shutdown != 0) {
			ui_t_unlock();
			break;
		}
	}

	return 0;
}

