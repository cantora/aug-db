#ifndef AUG_DB_WINDOW_H
#define AUG_DB_WINDOW_H

#include "api_calls.h"
/* these expect to be called by the main thread */
int window_init();
void window_free();

int window_off();
/* these expect to be called by the ui thread */
int window_start();
void window_end();
void window_refresh();
void window_render();
void window_ncwin(WINDOW **win);

#endif /* AUG_DB_WINDOW_H */
