#ifndef AUG_DB_API_CALLS_H
#define AUG_DB_API_CALLS_H

#include "aug.h"

extern const struct aug_api *G_api;
extern struct aug_plugin *G_plugin;

#define AUG_API_HANDLE G_api
#define AUG_PLUGIN_HANDLE G_plugin
#include "aug_api_macros.h"

#endif /* AUG_DB_API_CALLS_H */