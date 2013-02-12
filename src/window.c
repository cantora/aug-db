#include "window.h"

#include "api_calls.h"
#include "err.h"
#include "ui_state.h"

#include <pthread.h>

static struct {
	PANEL *panel;
	pthread_mutex_t mtx;
	int off;
	WINDOW *win;
} g;

static void window_reset_vars();

#define ERR_UNLOCK_WARN(...) \
	do { \
		pthread_mutex_unlock(&g.mtx); \
		aug_log(__VA_ARGS__); \
	} while(0)

int window_init() {
	if(pthread_mutex_init(&g.mtx, NULL) != 0)
		return -1;	
	window_reset_vars();
	return 0;
}

static void window_reset_vars() {
	g.off = 1;
	g.panel = NULL;
	g.win = NULL;
}

int window_free() {
	if(pthread_mutex_destroy(&g.mtx) != 0)
		return -1;
		
	return 0;
}

static void window_lock() {
	int status;
	if( (status = pthread_mutex_lock(&g.mtx)) != 0 )
		err_panic(status, "failed to lock window");
}
static void window_unlock() {
	int status;
	if( (status = pthread_mutex_unlock(&g.mtx)) != 0 )
		err_panic(status, "failed to lock window");
}

int window_off() {
	int result;

	window_lock();
	result = g.off;
	window_unlock();
	
	return result;
}

int window_start() {
	WINDOW *win;
	int rows, cols;

	window_lock();
	err_assert(g.off != 0);

	g.off = 0;
	aug_screen_panel_alloc(0, 0, 0, 0, &g.panel);
	aug_lock_screen();
	if( (win = panel_window(g.panel)) == NULL) {
		ERR_UNLOCK_WARN("could not get window from panel\n");
		aug_unlock_screen();
		aug_screen_panel_dealloc(g.panel);
		return -1;
	}

	getmaxyx(win, rows, cols);
	box(win, 0, 0);
	g.win = derwin(win, rows-2, cols-2, 1, 1);
	aug_unlock_screen();
	if(g.win == NULL) {
		ERR_UNLOCK_WARN("derwin was null\n");
		aug_screen_panel_dealloc(g.panel);
		return -1;
	}

	window_unlock();
	return 0;
}

/* an error here means we cant be sure that the screen is or can 
 * be cleaned up. thus, no error in this function is recoverable.
 */
void window_end() {
	int status;

	window_lock();
	err_assert(g.off == 0);

	aug_lock_screen();
	status = delwin(g.win);
	aug_unlock_screen();
	if(status == ERR)
		err_panic(0, "failed to delete window\n");

	aug_screen_panel_dealloc(g.panel);
	window_reset_vars();
	window_unlock();
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
		if(waddch(_win, _ch) == ERR) { err_panic(0, "failed to write char to window"); } \
	} while(0)
	
#define WPRINTW(_win, ...) \
	do { \
		if(wprintw(_win, __VA_ARGS__) == ERR) { err_panic(0, "failed to printw to window"); } \
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

