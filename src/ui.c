#include "ui.h"

#include "api_calls.h"
#include "err.h"
#include "util.h"
#include "window.h"
#include "fifo.h"

#include <pthread.h>
#include <errno.h>
#include <ccan/array_size/array_size.h>

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
	int input_buf[1024];
	struct fifo input_pipe;
	pthread_mutex_t pipe_mtx;
} g;

static void *ui_t_run(void *);

/* only cleanup essential stuff here, ui_free will
 * be called later
 */
static void ui_err_cleanup(int error) {
	(void)(error);

	g.error = 1;
}

static void set_sig_cmd_key() {
	g.sig_cmd_key = 1;
}
static void clr_sig_cmd_key() {
	g.sig_cmd_key = 0;
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

	fifo_init(&g.input_pipe, g.input_buf, sizeof(int), ARRAY_SIZE(g.input_buf));

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
			aug_log("signal ui thread\n");
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

static int wakeup_ui_thread(void (*callback)()) {
	int status;

	if( (status = ui_lock()) != 0)
		return status;
	
	if(g.waiting != 0) {
		g.waiting = 0;
		if(callback != NULL)
			(*callback)();
		if( (status = pthread_cond_signal(&g.wakeup)) != 0) 
			goto unlock;
	}
	else {
		aug_log("expected ui thread to be waiting\n");
		status = -1;
		goto unlock;
	}

	if( (status = ui_unlock()) != 0)
		return status;

	return 0;
unlock:
	ui_unlock();
	return status;
}

int ui_on_cmd_key() { 
	aug_log("ui: on_cmd_key\n");
	if(wakeup_ui_thread(set_sig_cmd_key) != 0)
		return -1;
	
	return 0;
}

int ui_on_input(const int *ch) {
	/*aug_log("ui_on_input\n");*/

	if(window_off() != 0)
		return 0;

	if( (pthread_mutex_lock(&g.pipe_mtx)) != 0) {
		return -1;
	}
	
	if(fifo_avail(&g.input_pipe) < 1)
		goto unlock;
	fifo_push(&g.input_pipe, ch);

	if(wakeup_ui_thread(NULL) != 0) 
		goto unlock;
	
	if( (pthread_mutex_unlock(&g.pipe_mtx)) != 0) {
		return -1;
	}			

	return 1;
unlock:
	pthread_mutex_unlock(&g.pipe_mtx);
	return -1;
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

static void ui_t_refresh() {
	aug_lock_screen();
	aug_screen_panel_update();
	aug_screen_doupdate();
	aug_unlock_screen();
	aug_log("refreshed window\n");
}

/* mtx is locked upon entry to this function.
 * this function should return with mtx locked.
 */
static void interact() {
	int status, ch;
	size_t key_amt, i;
	WINDOW *win;

	aug_log("interact: begin\n");
	if(window_start() != 0)
		ERR_UNLOCK_DIE(0, "failed to start window");

	ui_t_refresh();
	g.waiting = 0;
	
	while(1) {
		if(g.sig_cmd_key != 0 || g.shutdown != 0) {
			clr_sig_cmd_key();
			/* leave mtx locked */
			break;
		}
		else if(g.waiting == 0) {
			if( (key_amt = fifo_amt(&g.input_pipe)) > 0) {
				/*aug_log("consume %d chars of input\n", key_amt);*/
				aug_lock_screen();
				window_ncwin(&win);
				if(win == NULL) {
					aug_unlock_screen();
					window_end();
					ERR_UNLOCK_DIE(0, "window was null");
				}

				for(i = 0; i < key_amt; i++) {
					fifo_pop(&g.input_pipe, &ch);
					waddch(win, ch);
				}

				wsyncup(win);
				wcursyncup(win);
				aug_screen_panel_update();
				aug_screen_doupdate();

				aug_unlock_screen();
			}
		} /* else if(g.waiting==0) */
		/* else "spurious wakeup", do nothing */

		g.waiting = 1;
		status = pthread_cond_wait(&g.wakeup, &g.mtx);
		if(status != 0) {
			window_end();
			ERR_UNLOCK_DIE(status, "error in condition wait");
		}
	} /* while(1) */

	window_end();
	ui_t_refresh();
	aug_log("interact: end\n");
}

static void *ui_t_run(void *user) {
	int status;

	(void)(user);

	err_set_cleanup_fn(ui_err_cleanup);
	ui_t_lock(); /* what if this fails? aug_unload before init finishes? */

	while(1) {
		g.waiting = 1;
		status = pthread_cond_wait(&g.wakeup, &g.mtx);
		if(status != 0)
			ERR_UNLOCK_DIE(status, "error in condition wait");

		clr_sig_cmd_key();
		if(g.shutdown != 0) {
			ui_t_unlock();
			break;
		}
		else if(g.waiting == 0) {
			interact();
			/* shut down might be signaled during interaction */
			if(g.shutdown != 0) {
				ui_t_unlock();
				break;	
			}
		}
		/* else "spurious wakeup". do nothing */
	}

	return 0;
}

