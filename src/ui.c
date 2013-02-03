#include "ui.h"

#include "api_calls.h"
#include "err.h"
#include "util.h"
#include "window.h"

#include <pthread.h>
#include <errno.h>

/* this module represents the user interface thread and the functions
 * that other threads can call to interface with the 
 * user interface thread.
 * ui_t_* functions are expected to only be called by
 * the ui thread */

static struct {
	pthread_t tid;
	pthread_mutex_t mtx;
	pthread_cond_t wakeup;
	int shutdown;
	int waiting;
	int error;
	int sig_cmd_key;
} g;

static void *ui_t_run(void *);

/* only cleanup essential stuff here, ui_free will
 * be called later
 */
static void ui_err_cleanup(int error) {
	(void)(error);

	g.error = 1;
}

#define ERR_DIE(...) \
	err_die(__VA_ARGS__)

#define ERR_UNLOCK_DIE(...) \
	do { \
		pthread_mutex_unlock(&g.mtx); \
		err_die(__VA_ARGS__); \
	} while(0)
		
int ui_init() {
	g.shutdown = 0;
	g.error = 0;
	g.waiting = 0;
	g.sig_cmd_key = 0;
	if(pthread_mutex_init(&g.mtx, NULL) != 0)
		return -1;
	if(pthread_cond_init(&g.wakeup, NULL) != 0)
		goto cleanup_mtx;
	if(window_init() != 0)
		goto cleanup_cond;

	if(pthread_create(&g.tid, NULL, ui_t_run, NULL) != 0)
		goto cleanup_window;
	while(g.waiting == 0)
		util_usleep(0, 10000);

	return 0;

cleanup_window:
	window_free();
cleanup_cond:
	pthread_cond_destroy(&g.wakeup);
cleanup_mtx:
	pthread_mutex_destroy(&g.mtx);

	return -1;
}

/* this should not be called by ui thread */
int ui_free() {
	aug_log("ui free\n");
	if(g.error == 0) {
		aug_log("kill ui thread\n");
		if(ui_lock() != 0)
			return -1;
		g.shutdown = 1;
		if(g.waiting != 0)
			if(pthread_cond_signal(&g.wakeup) != 0) {
				ui_unlock();
				return -1;
			}
		/* else if ui is running, it should see the g.shutdown flag set */
		if(ui_unlock() != 0)
			return -1;
	}

	aug_log("join ui thread\n");
	if( pthread_join(g.tid, NULL) != 0)
		return -1;

	aug_log("ui thread dead\n");
#ifdef AUG_DB_DEBUG
	int status;
	if(window_free() != 0)
		err_panic(0, "failed to free window");
	if( (status = pthread_cond_destroy(&g.wakeup)) != 0)
		err_panic(status, "failed to destroy ui condition");
	if( (status = pthread_mutex_destroy(&g.mtx)) != 0)
		err_panic(status, "failed to destroy ui mutex");
#else
	/* an error here isnt fatal */
	window_free();
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

int ui_on_cmd_key() { 
	int status;

	aug_log("ui: on_cmd_key\n");
	if( (status = ui_lock()) != 0)
		return status;
	if(g.waiting != 0) {
		g.waiting = 0;
		if( (status = pthread_cond_signal(&g.wakeup)) != 0) {
			ui_unlock();
			return status;
		}
	}
	else
		g.sig_cmd_key = 1;
		
	if( (status = ui_unlock()) != 0)
		return status;

	return 0;
}

static void ui_t_lock() {
	int status;

	if( (status = pthread_mutex_lock(&g.mtx) ) != 0)
		ERR_DIE(status, "the ui thread encountered error while trying to lock");
}

static void ui_t_unlock() {
	int status;

	if( (status = pthread_mutex_unlock(&g.mtx) ) != 0)
		ERR_DIE(status, "the ui thread encountered error while trying to unlock");
}

static void interact() {
	aug_log("interact: begin\n");
	window_start();
	util_usleep(1, 0);
	window_end();
	aug_log("interact: end\n");
}

static void *ui_t_run(void *user) {
	int status;

	(void)(user);

	err_set_cleanup_fn(ui_err_cleanup);
	ui_t_lock(); /* what if this fails? aug_unload before init finishes? */
	g.waiting = 1;

	while(1) {
		status = pthread_cond_wait(&g.wakeup, &g.mtx);
		if(status != 0)
			ERR_UNLOCK_DIE(status, "error in condition wait");

		if(g.shutdown != 0) {
			ui_t_unlock();
			break;
		}
		else if(g.waiting == 0) {
			interact();
			g.waiting = 1;
		}
	}

	return 0;
}

