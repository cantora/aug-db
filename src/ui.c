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
#include "ui.h"

#include "api_calls.h"
#include "err.h"
#include "util.h"
#include "window.h"
#include "fifo.h"
#include "ui_state.h"
#include "lock.h"
#include "db.h"

#include <pthread.h>
#include <errno.h>
#include <iconv.h>
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
	int sig_cmd_key;
	int sig_dims_changed;
	int input_buf[1024];
	struct fifo input_pipe;
	pthread_mutex_t pipe_mtx;
	iconv_t cd;
} g;

static void *ui_t_run(void *);

#define DEF_SET_SIG_FN(_sig) \
	static void set_sig_ ## _sig () { g.sig_ ## _sig = 1; }
#define DEF_CLR_SIG_FN(_sig) \
	static void clr_sig_ ## _sig () { g.sig_ ## _sig = 0; }

DEF_SET_SIG_FN(cmd_key)
DEF_CLR_SIG_FN(cmd_key)
DEF_SET_SIG_FN(dims_changed)
DEF_CLR_SIG_FN(dims_changed)

#define UI_LOCK(_status) \
	AUG_DB_LOCK(&g.mtx, _status, "failed to lock ui mutex")
#define UI_UNLOCK(_status) \
	AUG_DB_UNLOCK(&g.mtx, _status, "failed to unlock ui mutex")

#define UI_LOCK_PIPE(_status) \
	AUG_DB_LOCK(&g.pipe_mtx, _status, "failed to lock pipe mutex")
#define UI_UNLOCK_PIPE(_status) \
	AUG_DB_UNLOCK(&g.pipe_mtx, _status, "failed to unlock pipe mutex")

int ui_init() {
	int status;

	g.shutdown = 0;
	g.waiting = 0;
	clr_sig_cmd_key();
	clr_sig_dims_changed();
	if(pthread_mutex_init(&g.mtx, NULL) != 0)
		return -1;
	if(pthread_mutex_init(&g.pipe_mtx, NULL) != 0)
		goto cleanup_mtx;
	if(pthread_cond_init(&g.wakeup, NULL) != 0)
		goto cleanup_both_mtx;

	if( (g.cd = iconv_open("WCHAR_T", "UTF8")) == ((iconv_t) -1) )
		goto cleanup_cond;

	if(ui_state_init() != 0)
		goto cleanup_iconv;
	if(window_init() != 0)
		goto cleanup_ui_state;

	fifo_init(&g.input_pipe, g.input_buf, sizeof(uint32_t), ARRAY_SIZE(g.input_buf));

	if(pthread_create(&g.tid, NULL, ui_t_run, NULL) != 0)
		goto cleanup_window;

	while(1) {
		UI_LOCK(status);
		if(g.waiting == 1) {
			UI_UNLOCK(status);
			break;
		}
		UI_UNLOCK(status);
		util_usleep(0, 10000);
	}

	return 0;

cleanup_window:
	window_free();
cleanup_ui_state:
	ui_state_free();
cleanup_iconv:
	if(iconv_close(g.cd) != 0)
		err_warn(errno, "failed to close iconv descriptor");
cleanup_cond:
	pthread_cond_destroy(&g.wakeup);
cleanup_both_mtx:
	pthread_mutex_destroy(&g.pipe_mtx);
cleanup_mtx:
	pthread_mutex_destroy(&g.mtx);

	return -1;
}

/* this should not be called by ui thread */
void ui_free() {
	int status;

	aug_log("ui free\n");
	aug_log("shutdown ui thread\n");
	ui_lock();
	g.shutdown = 1;
	if(g.waiting != 0) {
		aug_log("signal ui thread\n");
		if( (status = pthread_cond_signal(&g.wakeup)) != 0)
			err_panic(status, "failed to signal condition");
	}
	/* else if ui is not waiting, it should see the g.shutdown flag set */
	ui_unlock();

	aug_log("join ui thread\n");
	if( (status = pthread_join(g.tid, NULL)) != 0)
		err_panic(status, "failed to join thread");

	aug_log("ui thread dead\n");

	window_free();
	ui_state_free();
	if(iconv_close(g.cd) != 0)
		err_warn(errno, "failed to close iconv descriptor");
	if( (status = pthread_cond_destroy(&g.wakeup)) != 0)
		err_warn(status, "failed to destroy ui condition");
	if( (status = pthread_mutex_destroy(&g.pipe_mtx)) != 0)
		err_warn(status, "failed to destroy pipe mutex");
	if( (status = pthread_mutex_destroy(&g.mtx)) != 0)
		err_warn(status, "failed to destroy ui mutex");

}

void ui_lock() {
	int status;
	UI_LOCK(status);
}
void ui_unlock() {
	int status;
	UI_UNLOCK(status);
}

static void wakeup_ui_thread(void (*callback)()) {
	int status;

	ui_lock();
	if(callback != NULL)
		(*callback)();
	if(g.waiting != 0)
		if( (status = pthread_cond_signal(&g.wakeup)) != 0) 
			err_panic(status, "failed to signal cond");
	ui_unlock();
}

void ui_on_cmd_key() { 
	aug_log("ui: on_cmd_key\n");
	wakeup_ui_thread(set_sig_cmd_key);
}

int ui_on_input(const uint32_t *ch) {
	int status;
	/*aug_log("ui_on_input: 0x%04x\n", *ch);*/

	if(window_off() != 0) {
		/*aug_log("ui_on_input: window is off. ignore input\n");*/
		return 1;
	}

	UI_LOCK_PIPE(status);
	if(fifo_avail(&g.input_pipe) < 1) {
		aug_log("ui_on_input: no space available in fifo\n");
		UI_UNLOCK_PIPE(status);
		return -1;
	}
	fifo_push(&g.input_pipe, ch);
	UI_UNLOCK_PIPE(status);
	wakeup_ui_thread(NULL);	

	/*aug_log("ui_on_input: successfully pushed input\n");*/
	return 0;
}

void ui_on_dims_change(int rows, int cols) {
	(void)(rows);
	(void)(cols);

	wakeup_ui_thread(set_sig_dims_changed);	
}

#define WINDOW_TOO_SMALL_MSG "window is too small to fit aug-db interface\n"

static int render() {
	int status;

	UI_LOCK(status);
	if(g.sig_dims_changed) {
		clr_sig_dims_changed();
		window_end();
		if(window_start() != 0) {
			aug_log(WINDOW_TOO_SMALL_MSG);
			UI_UNLOCK(status);
			return -1;
		}
	}
	UI_UNLOCK(status);

	window_render();
	return 0;
}

static void write_data_to_term(const uint8_t *data, size_t dsize, int raw, 
		uint32_t run_ch) {
	int written;
	size_t ibl, obl, status;
	uint32_t ch;
	char *dp, *chp;
		
	aug_log("run entry\n");
	if(raw == 0) { /* utf-8 */
		ibl = dsize;
		dp = (char *) data;

		while(ibl > 0) {
			obl = sizeof(ch);
			chp = (char *) &ch;
	
			/*aug_log("%d bytes left to convert\n", ibl);*/
			if( (status = iconv(g.cd, &dp, &ibl, &chp, &obl)) == ((size_t) -1)) {
				if(errno != E2BIG)
					err_panic(errno, "failed to convert data to utf-32");
				else if(obl != 0)
					err_panic(errno, "only %d/%d bytes were written to output", sizeof(ch)-obl, sizeof(ch));
			}

			while( (written = aug_primary_input(&ch, 1)) != 1)
				if(written != 0)
					err_panic(0, "expected written == 0");
		}

	}
	else { /* raw bytes */
		written = 0;
		while(written < (int) dsize) 
			written += aug_primary_input_chars( (char *) data+written, dsize-written);
	}

	if(run_ch != 0) 
		while( (written = aug_primary_input(&run_ch, 1)) != 1)
			if(written != 0)
				err_panic(0, "expected written == 0");

}

static void use_chosen_result() {
	const uint8_t *data;
	size_t dsize;
	uint32_t run_ch;
	int status, raw;

	status = ui_state_query_selected_result(&data, &dsize, &raw, &run_ch);
	if(status == 0) {
		if(dsize > 0) {
			write_data_to_term(data, dsize, raw, run_ch);					
			/* maybe clear pipe here? */
		}
	}
}

static void act_on_state(int *interact_off, int *do_render) {
	int cmd;

	switch(ui_state_current()) {
	case UI_STATE_QUERY:
		aug_log("act on state: query\n");
		cmd = ui_state_query_run_cmd();
		if(cmd == UI_QUERY_CMD_CHOOSE)
			use_chosen_result();
		if(cmd == UI_QUERY_CMD_EXIT_INTERACT || cmd == UI_QUERY_CMD_CHOOSE)
			*interact_off = 1;

		break;
	case UI_STATE_HELP_QUERY:
		aug_log("act on state: help query\n");
		*do_render = 1;
		break;
	default:
		err_warn(0, "unhandled state in act_on_state");
	} /* switch(ui state) */
}

/* mtx is unlocked upon entry to this function.
 * this function should return with mtx unlocked.
 */
static void interact() {
	int status, amt, do_render, brk;
	
	aug_log("interact: begin\n");
	if(window_start() != 0) {
		aug_log(WINDOW_TOO_SMALL_MSG);
		return;
	}

	do_render = 1;
	/*g.waiting = 0; shouldnt need this, ui_t_run already sets this to 0 */
	brk = 0;
	while(1) {
		UI_LOCK(status);
		if(g.sig_cmd_key != 0 || g.shutdown != 0) {
			clr_sig_cmd_key();
			UI_UNLOCK(status);
			break; 
		}

		/* both branches unlock g.mtx */
		if(g.sig_dims_changed != 0) {
			clr_sig_dims_changed();
			UI_UNLOCK(status);

			window_end();
			ui_state_dims_changed();
			if(window_start() != 0) {
				aug_log(WINDOW_TOO_SMALL_MSG);
				/* UI should be unlocked at refresh, so leave lock state as is */
				goto refresh; 
			}
			do_render = 1;
		}
		else {
			UI_UNLOCK(status);
		}

		while(fifo_amt(&g.input_pipe) > 0) {
			UI_LOCK_PIPE(status);
			aug_log("consume input\n");
			/* no aug API calls in this function, so we shouldnt be in danger of 
			 * circular dependencies */
			amt = ui_state_consume(&g.input_pipe);
			UI_UNLOCK_PIPE(status);

			/* we render on amt > 0, so if it is 0 and act_on_state
			 * sets it to 1 we will render. */
			aug_log("act on state\n");
			act_on_state(&brk, &amt);
			if(brk != 0)
				break; /* breaking here leaves g.mtx unlocked */
			if(amt > 0) {
				if(render() != 0) {
					/* jumps to the end of the function leaving g.mtx unlocked */
					goto refresh;  /* window_end() was called by render() so we can skip it */
				}
				do_render = 0;
			}
		} /* while(data in fifo) */
		
		if(do_render) {
			if(render() != 0) {
				/* jumps to the end of the function leaving g.mtx unlocked */
				goto refresh;  /* window_end() was called by render() */
			}
			do_render = 0;
		}

		if(brk != 0)
			break; /* breaking here leaves g.mtx unlocked */

		UI_LOCK_PIPE(status);
		if(fifo_amt(&g.input_pipe) > 0) {
			UI_UNLOCK_PIPE(status);
			continue;
		}
		UI_UNLOCK_PIPE(status);

		/*aug_log("interact: wait\n");*/
		UI_LOCK(status);
		g.waiting = 1;
		if( (status = pthread_cond_wait(&g.wakeup, &g.mtx)) != 0)
			err_panic(status, "error in condition wait");
		g.waiting = 0;
		UI_UNLOCK(status);
		/*aug_log("interact: wokeup\n");*/
	} /* while(1) */
	ui_state_interact_end();
		
	window_end();
refresh:
	window_refresh();
	aug_log("interact: end\n");
} /* interact */

static void *ui_t_run(void *user) {
	int status;

	(void)(user);

	UI_LOCK(status);
	while(1) {
		g.waiting = 1;
		if( (status = pthread_cond_wait(&g.wakeup, &g.mtx) ) != 0)
			err_panic(status, "error in condition wait");
		g.waiting = 0;

		if(g.shutdown != 0)
			break;
		else if(g.sig_cmd_key != 0) {
			clr_sig_cmd_key();
			UI_UNLOCK(status);
			interact();
			UI_LOCK(status);
			/* shut down might be signaled during interaction */
			if(g.shutdown != 0)
				break;	
		}
		/* else "spurious wakeup". do nothing */
	}
	UI_UNLOCK(status);

	return 0;
}

