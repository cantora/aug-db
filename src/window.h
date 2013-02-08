#ifndef AUG_DB_WINDOW_H
#define AUG_DB_WINDOW_H

#include "api_calls.h"
/* these expect to be called by the main thread */
int window_init();
int window_free();

/* these expect to be called by the ui thread */
void window_start();
void window_end();

int window_ncwin(WINDOW **win);

#endif /* AUG_DB_WINDOW_H */
