#include "db.h"
#include "err.h"

#include <sqlite3.h>


static struct {
	sqlite3 *handle;
} g;

int db_init(const char *fpath) {
	if(sqlite3_open(fpath, &g.handle) != SQLITE_OK) {
		err_warn(0, "failed to open sqlite db: %s", sqlite3_errmsg(g.handle));
		sqlite3_close(g.handle);
		return -1;
	}

	return 0;
}

void db_free() {
	if(sqlite3_close(g.handle) != SQLITE_OK)
		err_warn(0, "failed to close sqlite db: %s", sqlite3_errmsg(g.handle));
}

