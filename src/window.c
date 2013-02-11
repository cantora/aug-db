#include "window.h"

#include "api_calls.h"
#include "err.h"

#include <pthread.h>

static struct {
	PANEL *panel;
	pthread_mutex_t mtx;
	int off;
	WINDOW *win;
} g;

static void window_reset_vars();

#define ERR_DIE(...) \
	err_die(__VA_ARGS__)

#define ERR_UNLOCK_DIE(...) \
	do { \
		pthread_mutex_unlock(&g.mtx); \
		err_die(__VA_ARGS__); \
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
		ERR_DIE(status, "failed to lock window");
}
static void window_unlock() {
	int status;
	if( (status = pthread_mutex_unlock(&g.mtx)) != 0 )
		ERR_DIE(status, "failed to lock window");
}

int window_off() {
	int result;

	window_lock();
	result = g.off;
	window_unlock();
	
	return result;
}

void window_start() {
	WINDOW *win;
	int rows, cols;

	window_lock();
	if(g.off == 0)
		ERR_UNLOCK_DIE(0, "window is already started!");

	g.off = 0;
	aug_screen_panel_alloc(0, 0, 0, 0, &g.panel);
	aug_lock_screen();
	if( (win = panel_window(g.panel)) == NULL) {
		aug_unlock_screen();
		ERR_UNLOCK_DIE(0, "could not get window from panel");
	}		
	getmaxyx(win, rows, cols);
	box(win, 0, 0);
	g.win = derwin(win, rows-2, cols-2, 1, 1);
	aug_unlock_screen();
	if(g.win == NULL)
		ERR_UNLOCK_DIE(0, "derwin was null");

	window_unlock();
}

void window_end() {
	int status;

	window_lock();
	if(g.off != 0)
		ERR_UNLOCK_DIE(0, "window is not started!");

	aug_lock_screen();
	status = delwin(g.win);
	aug_unlock_screen();
	if(status == ERR)
		ERR_UNLOCK_DIE(0, "failed to delete window");

	aug_screen_panel_dealloc(g.panel);
	window_reset_vars();
	window_unlock();
}

void window_ncwin(WINDOW **win) {
	*win = g.win;
}

