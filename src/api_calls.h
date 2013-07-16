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
#ifndef AUG_DB_API_CALLS_H
#define AUG_DB_API_CALLS_H

#include "aug.h"

extern const struct aug_api *G_api;
extern struct aug_plugin *G_plugin;

#define AUG_API_HANDLE G_api
#define AUG_PLUGIN_HANDLE G_plugin
#include "aug_api_macros.h"

#endif /* AUG_DB_API_CALLS_H */
