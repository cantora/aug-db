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
#ifndef AUG_DB_ENCODING_H
#define AUG_DB_ENCODING_H

#include <stdint.h>
#include <stddef.h>

int encoding_init();
void encoding_free();
/* returns the number of bytes left in utf8_data */
size_t encoding_wchar_to_utf8(uint8_t *utf8_data, size_t utf8_len,
		const uint32_t *wchar_data, size_t wchar_len);

#endif /* AUG_DB_ENCODING_H */
