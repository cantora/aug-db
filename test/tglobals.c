#include "aug.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

const struct aug_api *G_api;
struct aug_plugin *G_plugin;

int test_log(struct aug_plugin *plugin, const char *format, ...) {
	va_list args;
	int result;
	(void)(plugin);

	va_start(args, format);
	result = vfprintf(stderr, format, args);
	va_end(args);

	return result;
}

void test_init_api() {
	struct aug_api *api;

	G_plugin = NULL;

	G_api = malloc( sizeof(struct aug_api) );
	api = (struct aug_api *) G_api;
	memset(api, 0, sizeof(struct aug_api) );
	api->log = test_log;
}

void test_free_api() {
	free( (struct aug_api *) G_api);
}

