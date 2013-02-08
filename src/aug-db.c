#include "aug_plugin.h"

#include "err.h"
#include "ui.h"

#include <strings.h>

#include "api_calls.h"

const char aug_plugin_name[] = "aug-db";

static void on_cmd_key(int, void *);
static void on_input(int *, aug_action *, void *);
static void on_dims_change(int, int, void *);

/* requires screen lock b.c. curses calls are invoked */
static int char_rep_to_char(const char *, int *);

struct aug_plugin_cb g_callbacks = {
	.input_char = on_input,
	.cell_update = NULL,
	.cursor_move = NULL,
	.screen_dims_change = on_dims_change
};

static int g_cmd_ch;
static int g_freed;

void aug_plugin_err_cleanup(int error) {
	(void)(error);
	aug_log("fatal error. cleaning up...\n");
	aug_log("call aug_unload()\n");
	/* this will call aug_plugin_free */
	aug_unload();
}

int aug_plugin_init(struct aug_plugin *plugin, const struct aug_api *api) {
	const char *key;
	const char default_key[] = "^R";

	G_plugin = plugin;	
	G_api = api;

	aug_log("init\n");

	g_freed = 0;
	g_callbacks.user = NULL;

	if(err_init() != 0)
		return -1;

	if(err_dispatch_init(aug_plugin_err_cleanup) != 0) {
		aug_log("failed to init err_dispatch\n");
		return -1;
	}

	if( aug_conf_val(aug_plugin_name, "key", &key) == 0) {
		aug_log("command key: %s\n", key);
	}
	else {
		key = default_key;
		aug_log("no command key configured, using default: %s\n", key);
	}	

	aug_lock_screen();
	if( char_rep_to_char(key, &g_cmd_ch) != 0 ) {
		aug_log("failed to map character key to %s\n", key);
		goto unlock_screen;
	}
	aug_unlock_screen();

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
unlock_screen:
	aug_unlock_screen();
	return -1;
}

void aug_plugin_free() {
	if(g_freed != 0)
		err_panic(0, "aug-db was already freed!");

	g_freed = 1;
	aug_log("free\n");
	aug_key_unbind(g_cmd_ch);
#ifdef AUG_DB_DEBUG
	if(ui_free() != 0)
		err_panic(0, "failed to free ui");
#else
	ui_free();
#endif
	err_free();
}

/* cannot call aug_unload in a callback, so we have to 
 * signall the err_dispatch module
 */
#define ERR_IN_CB(...) \
	do { \
		aug_log(__VA_ARGS__); \
		err_dispatch_signal(0); \
	} while(0)

static void on_cmd_key(int ch, void *user) {
	(void)(ch);
	(void)(user);

	if(ui_on_cmd_key() != 0)
		ERR_IN_CB("error in ui_on_cmd_key. unload...\n");
}

static void on_input(int *ch, aug_action *action, void *user) {
	(void)(action);
	(void)(user);

	if(ui_on_input(ch) != 0)
		ERR_IN_CB("error in ui_on_input. unload...");
}

static void on_dims_change(int rows, int cols, void *user) {
	(void)(user);
	(void)(rows);
	(void)(cols);
}

static int char_rep_to_char(const char *str, int *ch) {
	int i;
	const char *s;

	for(i = 0; i <= 0xff; i++) {
		if( (s = unctrl(i)) == NULL )
			return -1;

		if(strcasecmp(str, s) == 0) {
			*ch = i;
			break;	
		}
	}

	if(i > 0xff)
		return -1;

	return 0;
}

