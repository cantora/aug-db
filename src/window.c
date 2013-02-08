#include "window.h"

#include "api_calls.h"
#include "err.h"

#include <pthread.h>

static struct {
	PANEL *panel;
	pthread_mutex_t mtx;
	int off;
} g;

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
	g.off = 1;
	g.panel = NULL;
	return 0;
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

void window_start() {
	window_lock();
	if(g.off == 0)
		ERR_UNLOCK_DIE(0, "window is already started!");

	aug_screen_panel_alloc(0, 0, 0, 0, &g.panel);
	g.off = 0;
	window_unlock();	
}

void window_end() {
	window_lock();
	if(g.off != 0)
		ERR_UNLOCK_DIE(0, "window is not started!");

	g.off = 1;
	aug_screen_panel_dealloc(g.panel);
	
	window_unlock();
}

int window_ncwin(WINDOW **win) {
	if( (*win = panel_window(g.panel)) == NULL)
		return -1;

	return 0;
}
