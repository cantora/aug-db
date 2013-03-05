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
	WINDOW *search_win;
	WINDOW *result_win;
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
	
	if(rows < 5 || cols < 20) {
		aug_unlock_screen();
		aug_screen_panel_dealloc(g.panel);
		WINDOW_UNLOCK(status);
		return -1;
	}

	box(win, 0, 0);
#define WIN_ROWS (rows-2)
#define WIN_COLS (cols-2)
	g.win = derwin(win, WIN_ROWS, WIN_COLS, 1, 1);
	if(g.win == NULL) 
		err_panic(0, "derwin was null\n");

#define WIN_SEARCH_ROW 2
	g.search_win = derwin(g.win, 1, WIN_COLS, WIN_SEARCH_ROW, 0);
	if(g.search_win == NULL) 
		err_panic(0, "g.search_win was null\n");
	g.result_win = derwin(g.win, WIN_ROWS-(WIN_SEARCH_ROW+1), 
			WIN_COLS, WIN_SEARCH_ROW+1, 0);
	if(g.result_win == NULL) 
		err_panic(0, "g.search_win was null\n");
#undef WIN_ROWS
#undef WIN_COLS
#undef WIN_SEARCH_ROW

	if(keypad(stdscr, 1) == ERR)
		err_panic(0, "failed to enable keypad");
	
	aug_unlock_screen();

	g.off = 0;
	ui_state_query_result_reset();
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
	
	if(delwin(g.result_win) == ERR)
		err_panic(0, "failed to delete window\n");
	if(delwin(g.search_win) == ERR)
		err_panic(0, "failed to delete window\n");
	if(delwin(g.win) == ERR)
		err_panic(0, "failed to delete window\n");
	aug_unlock_screen();

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

#define WERASE(_win) \
	do { \
		if(werase(_win) == ERR) { \
			err_panic(0, "failed to erase window"); \
		} \
	} while(0)

#define WMOVE(_win, _y, _x) \
	do { \
		if(wmove(_win, _y, _x) == ERR) { \
			err_panic(0, "failed to move cursor"); \
		} \
	} while(0)

static void window_render_query() {
	const uint32_t *query;
	uint8_t *result;
	size_t n, i, rsize;
	int j, rows, cols, x, y, raw;

	err_assert(g.win != NULL);

	ui_state_query_value(&query, &n);
	aug_lock_screen();

	WMOVE(g.win, 0, 0);
	WPRINTW(g.win, "^J: choose\t^G: clear");
	WERASE(g.search_win);
	WMOVE(g.search_win, 0, 0);
	getmaxyx(g.search_win, rows, cols);
	WPRINTW(g.search_win, "(search)`");
	if(n > 0) 
		for(i = 0; i < n; i++) {
			getyx(g.search_win, y, x);
			/* if x >= cols-1-2 then the WPRINTW will fail */
			if(y >= rows || x >= (cols-1-2) ) {
				aug_log("exceeded window size of %d,%d\n", rows, cols);
				goto finish_search_win;
			}
			WADDCH(g.search_win, query[i]);
		}
finish_search_win:
	WPRINTW(g.search_win, "':");

	WERASE(g.result_win);
	WMOVE(g.result_win, 0, 0);

	aug_log("window: render results\n");
	getmaxyx(g.result_win, rows, cols);
	while(ui_state_query_result_next(&result, &rsize, &raw) == 0) {
		aug_log("window: render query result\n");
		getyx(g.result_win, y, x); 
		if(y >= rows - 1)
			goto update;

		for(j = 0; j < cols; j++) {
			WADDCH(g.result_win, '-');
		}

		for(i = 0; i < rsize; i++) {
			getyx(g.result_win, y, x); 
			if(y >= rows - 1 && x >= cols - 4) { 
				WPRINTW(g.result_win, "..."); 
				goto update; 
			}
			WADDCH(g.result_win, result[i]);
		}
		waddch(g.result_win, '\n');

		talloc_free(result);
	} /* while(query results) */

update:
	wsyncup(g.result_win);
	wcursyncup(g.result_win);
	wsyncup(g.search_win);
	wcursyncup(g.search_win);
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

