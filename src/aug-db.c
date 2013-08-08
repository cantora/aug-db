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
#include "aug_plugin.h"

#include "err.h"
#include "ui.h"
#include "db.h"
#include "util.h"

#include <strings.h>

#include "api_calls.h"

const char aug_plugin_name[] = "aug-db";

static void on_cmd_key(uint32_t, void *);
static void on_input(uint32_t *, aug_action *, void *);
static void on_dims_change(int, int, void *);

struct aug_plugin_cb g_callbacks;

static uint32_t g_cmd_ch;
static int g_freed;

int aug_plugin_init(struct aug_plugin *plugin, const struct aug_api *api) {
	const char *key, *dbpath;
	const char default_key[] = "^R";
	wordexp_t exp;

	G_plugin = plugin;	
	G_api = api;

	aug_log("init\n");

	g_freed = 0;
	aug_callbacks_init(&g_callbacks);
	g_callbacks.input_char = on_input;
	g_callbacks.screen_dims_change = on_dims_change;

	if(aug_conf_val(aug_plugin_name, "db", &dbpath) == 0) {
		aug_log("db file: %s\n", dbpath);
	} 
	else {
		dbpath = "~/.aug-db.sqlite";
		aug_log("no db file configured, using default: %s\n", dbpath);
	}

	if(util_expand_path(dbpath, &exp) != 0) {
		aug_log("failed to expand db path\n");
		return -1; /* exp is cleaned up by expand path */
	}
	if(db_init(exp.we_wordv[0]) != 0) {
		wordfree(&exp);
		aug_log("db failed to initialize\n");
		return -1;
	}
	wordfree(&exp);

	if( aug_conf_val(aug_plugin_name, "key", &key) == 0) {
		aug_log("command key: %s\n", key);
	} 
	else {
		key = default_key;
		aug_log("no command key configured, using default: %s\n", key);
	}

	if( aug_keyname_to_key(key, &g_cmd_ch) != 0 ) {
		aug_log("failed to map character key to %s\n", key);
		return -1;
	}

	/* register callback for when the user wants to activate aug-db features */
	if( aug_key_bind(g_cmd_ch, on_cmd_key, NULL) != 0) {
		aug_log("expected to be able to bind to extension '%s'. abort...");
		return -1;
	}
	aug_log("bound to key character: 0x%02x\n", g_cmd_ch);

	if(ui_init() != 0) {
		aug_log("failed to initialize ui\n");
		aug_key_unbind(g_cmd_ch);
		return -1;
	}

	aug_callbacks(&g_callbacks, NULL);
	
	return 0;
}

void aug_plugin_free() {
	if(g_freed != 0)
		err_panic(0, "aug-db was already freed!");

	g_freed = 1;
	aug_log("free\n");
	aug_key_unbind(g_cmd_ch);
	ui_free();
	db_free();
}

static void on_cmd_key(uint32_t ch, void *user) {
	(void)(ch);
	(void)(user);

	ui_on_cmd_key();
}

static void on_input(uint32_t *ch, aug_action *action, void *user) {
	int status;
	(void)(action);
	(void)(user);

	if( (status = ui_on_input(ch)) < 0)
		err_warn(0, "ui input buffer is full");
	
	if(status == 0)
		*action = AUG_ACT_CANCEL;
}

static void on_dims_change(int rows, int cols, void *user) {
	(void)(user);
	ui_on_dims_change(rows, cols);
}

