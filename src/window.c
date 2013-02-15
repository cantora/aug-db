#include "window.h"

#include "api_calls.h"
#include "err.h"
#include "ui_state.h"
#include "lock.h"

static struct {
	PANEL *panel;
	pthread_mutex_t mtx;
	int off;
	WINDOW *win;
} g;

static void window_reset_vars();

int window_init() {
	int status;
	if( (status = pthread_mutex_init(&g.mtx, NULL)) != 0) {
		err_warn(status, "failed to init mutex");
		return -1;
	}
	window_reset_vars();

	return 0;
}

static void window_reset_vars() {
	g.off = 1;
	g.panel = NULL;
	g.win = NULL;
}

void window_free() {
	int status;
	if( (status = pthread_mutex_destroy(&g.mtx)) != 0)
		err_warn(status, "failed to destroy window mutex");
}

#define WINDOW_LOCK(_status) \
	AUG_DB_LOCK(&g.mtx, _status, "failed to lock window")
#define WINDOW_UNLOCK(_status) \
	AUG_DB_UNLOCK(&g.mtx, _status, "failed to unlock window")
	
int window_off() {
	int result, status;

	WINDOW_LOCK(status);
	result = g.off;
	WINDOW_UNLOCK(status);
	
	return result;
}

int window_start() {
	WINDOW *win;
	int status, rows, cols;

	WINDOW_LOCK(status);
	err_assert(g.off != 0);

	aug_screen_panel_alloc(0, 0, 0, 0, &g.panel);
	aug_lock_screen();
	if( (win = panel_window(g.panel)) == NULL) 
		err_panic(0, "could not get window from panel\n");
	
	getmaxyx(win, rows, cols);
	/* need at least 3 rows and 3 columns for box */
	if(rows < 3 || cols < 3) {
		aug_unlock_screen();
		aug_screen_panel_dealloc(g.panel);
		WINDOW_UNLOCK(status);
		return -1;
	}

	box(win, 0, 0);
	g.win = derwin(win, rows-2, cols-2, 1, 1);
	if(keypad(stdscr, 1) == ERR)
		err_panic(0, "failed to enable keypad");
	
	aug_unlock_screen();
	if(g.win == NULL) 
		err_panic(0, "derwin was null\n");

	g.off = 0;
	WINDOW_UNLOCK(status);

	return 0;
}

void window_end() {
	int status;

	WINDOW_LOCK(status);
	err_assert(g.off == 0);

	aug_lock_screen();
	if(keypad(stdscr, 0) == ERR)
		err_panic(0, "failed to disable keypad");

	status = delwin(g.win);
	aug_unlock_screen();
	if(status == ERR)
		err_panic(0, "failed to delete window\n");

	aug_screen_panel_dealloc(g.panel);
	window_reset_vars();
	WINDOW_UNLOCK(status);
}

void window_ncwin(WINDOW **win) {
	*win = g.win;
}

void window_refresh() {
	aug_lock_screen();
	aug_screen_panel_update();
	aug_screen_doupdate();
	aug_unlock_screen();
	aug_log("refreshed window\n");
}

#define WADDCH(_win, _ch) \
	do { \
		if(waddch(_win, _ch) == ERR) { err_warn(0, "failed to write char to window"); } \
	} while(0)
	
#define WPRINTW(_win, ...) \
	do { \
		if(wprintw(_win, __VA_ARGS__) == ERR) { err_warn(0, "failed to printw to window"); } \
	} while(0)

static void window_render_query() {
	const int *query;
	size_t n,i;

	err_assert(g.win != NULL);

	ui_state_query_value(&query, &n);
	aug_lock_screen();
	if(werase(g.win) == ERR)
		err_panic(0, "failed to erase window");
	if(wmove(g.win, 0, 0) == ERR)
		err_panic(0, "failed to move cursor");

	WPRINTW(g.win, "(search)`");
	if(n > 0) 
		for(i = 0; i < n; i++) 
			WADDCH(g.win, query[i]);
	WPRINTW(g.win, "':");

	wsyncup(g.win);
	wcursyncup(g.win);
	aug_screen_panel_update();
	aug_screen_doupdate();

	aug_unlock_screen();	
}

void window_render() {
	ui_state_name state;

	switch( (state = ui_state_current()) ) {
	case UI_STATE_QUERY:
		window_render_query();
		break;
	default:
		err_panic(0, "invalid ui state %d", state);
	}
}

