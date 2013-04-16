#include "window.h"

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
static void render_results(WINDOW *);

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

	aug_log("window_start\n");
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

#define WIN_SEARCH_ROW 0
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

	/* TODO: turning this on causes keys to be
	 * interpreted as unicode characters. at some 
	 * point this should be turned off and we should
	 * actually parse the keypad escape sequences 
	 * manually */
	if(keypad(stdscr, 1) == ERR)
		err_panic(0, "failed to enable keypad");
	
	aug_unlock_screen();

	g.off = 0;
	WINDOW_UNLOCK(status);

	return 0;
}

void window_end() {
	int status;

	aug_log("window_end\n");
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

static inline void aug_doupdate() {
	aug_screen_panel_update();
	aug_screen_doupdate();
}

void window_refresh() {
	aug_lock_screen();
	aug_doupdate();
	aug_unlock_screen();
	aug_log("refreshed window\n");
}

static inline void wsync(WINDOW *win) {
	wsyncup(win);
	wcursyncup(win);
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

static void window_render_help_query() {
	size_t amt, remain, i;
	int y, x, rows, cols;
	const char *key, *desc;

	/*aug_log("render help query\n");*/
	aug_lock_screen();

	WERASE(g.win);
	WMOVE(g.win, 0, 0);

	getmaxyx(g.win, rows, cols);
	if(cols < 2)
		goto unlock;
		
	getyx(g.win, y, x);
	for(/*nop*/; (remain = ui_state_help_query_remain()) > 0;
			ui_state_help_query_next()) {
		/*aug_log("render help query line %d\n", y);*/

		if(y == rows-1 && remain > 1) {
			WPRINTW(g.win, "(next page: <SPACE>)");
			
			break;
		}

		ui_state_help_query_value(&key, &desc);
		/* there should always be enough columns for this
		 * b.c. window_start asserts cols >= 20 */
		WPRINTW(g.win, "%s\t", key);
		amt = strlen(desc);
		for(i = 0; i < amt; i++) {
			getyx(g.win, y, x);
			if(x == cols-1) {
				WADDCH(g.win, desc[i]);
				break;
			}
			else if(x > cols-1)
				break;
				
			WADDCH(g.win, desc[i]);
		}

		y += 1;
		if(y >= rows)
			break;

		WMOVE(g.win, y, 0);
		getyx(g.win, y, x);
	}

	wsync(g.win);
	aug_doupdate();
unlock:
	aug_unlock_screen();
}

static void window_render_query() {
	const uint32_t *query;
	size_t n, i;
	int rows, cols, x, y;
	cchar_t cch;
	attr_t attr, *ap;
	short pair, *pp;

	/*aug_log("render query\n");*/
	err_assert(g.win != NULL);

	ui_state_query_value(&query, &n);
	aug_lock_screen();

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
				break;
			}

			ap = &attr;
			pp = &pair;
			if(wattr_get(g.search_win, ap, pp, NULL) == ERR)
				err_panic(0, "wattr_get failed");
			if(setcchar(&cch, (wchar_t *)&query[i], attr, pair, NULL) == ERR)
				err_panic(0, "setcchar failed");
			if(wadd_wch(g.search_win, &cch) == ERR)
				err_panic(0, "wadd_wch failed");
		}

	WPRINTW(g.search_win, "':");

	render_results(g.result_win);

	/* update */
	wsync(g.result_win);
	wsync(g.search_win);
	aug_doupdate();
	
	aug_unlock_screen();	
}

static int result_cb_fn(uint8_t *result, size_t rsize, int raw, int id, int idx, void *user) {
	int j, rows, cols, x, y;
	size_t i;
	char esc[5];
	WINDOW *win;
	(void)(idx);
	(void)(id);

	win = (WINDOW *) user;

#define CHECK_FOR_SPACE() \
	do { \
		getyx(win, y, x); \
		if(y >= rows - 1 && x >= cols - 1) { \
			return -1; \
		} \
	} while(0)


	getmaxyx(win, rows, cols);

	/*aug_log("window: render query result\n");*/
	getyx(win, y, x); 
	if(y >= rows - 1)
		return -1;

	for(j = 0; j < cols; j++) {
		WADDCH(win, '-');
	}

	for(i = 0; i < rsize; i++) {
		CHECK_FOR_SPACE();

		if(raw == 0) {
			if(result[i] == '\n' && y >= rows - 1)
				return -1;
			WADDCH(win, result[i]);
		}
		else {
			if(result[i] >= 0x20 && result[i] <= 0x7e)
				WADDCH(win, result[i]);
			else {
				snprintf(esc, 5, "\\x%02x", result[i]);
				for(j = 0; j < 4; j++) {
					CHECK_FOR_SPACE();
					WADDCH(win, esc[j]);
				}
			}
		}
	}
	waddch(win, '\n');

#undef CHECK_FOR_SPACE
	return 0;
}

static void render_results(WINDOW *win) {
	/*aug_log("window: render results\n");*/

	WERASE(win);
	WMOVE(win, 0, 0);

	ui_state_query_foreach_result(result_cb_fn, win);
}

void window_render() {
	ui_state_name state;

	switch( (state = ui_state_current()) ) {
	case UI_STATE_QUERY:
		window_render_query();
		break;
	case UI_STATE_HELP_QUERY:
		window_render_help_query();
		break;
	default:
		err_panic(0, "invalid ui state %d", state);
	}
}

