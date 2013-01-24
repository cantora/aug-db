#include "err.h"

#include <pthread.h>

/* this module represents the user interface thread and the functions
 * that other threads can call to interface with the 
 * user interface thread.
 * ui_t_* functions are expected to only be called by
 * the ui thread */

extern void aug_plugin_err_cleanup(int);

static struct {
	pthread_t thread;
	pthread_mutex_t mtx;
	int shutdown;
	ERR_MEMBERS;
} g;

static void *ui_t_run(void *);

void ui_err_cleanup(int error) {
	/* only cleanup essential stuff here, which is nothing right now */
	aug_plugin_err_cleanup(error);
}

int ui_init() {
	err_init(&g);
	g.shutdown = 0;
	if(pthread_mutex_init(&g.mtx, NULL) != 0)
		return -1;
	if(pthread_create(&g.thread, NULL, ui_t_run, NULL) != 0)
		return -1;

	return 0;
}

static int ui_lock() {
	return pthread_mutex_lock(&g.mtx);
}

void ui_free() {
	
}

static void ut_t_lock() {
	int status;

	if( (status = pthread_mutex_lock(&g.mtx) ) != 0)
		err_die(&g, status, 
			"the ui thread encountered error while trying to lock");
}

static void *ui_t_run(void *user) {
	(void)(user);

	return 0;
}
