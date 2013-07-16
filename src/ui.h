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
#ifndef AUG_DB_UI_H
#define AUG_DB_UI_H

#include <stdint.h>

int ui_init();
void ui_free();
void ui_lock();
void ui_unlock();
void ui_on_cmd_key();
int ui_on_input(const uint32_t *ch);
void ui_on_dims_change(int rows, int cols);

#endif /* AUG_DB_UI_H */
