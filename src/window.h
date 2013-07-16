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
