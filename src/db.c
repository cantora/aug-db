#include "db.h"
#include "err.h"
#include "api_calls.h"

#include <ccan/array_size/array_size.h>
#include <sqlite3.h>

/* SCHEMA
 *
 * version 1:
 * 		admin: INTEGER version, INTEGER last_updated
 *		tags: INTEGER id, TEXT name
 *		data: INTEGER id, BLOB value
 *		data_tags: INTEGER data_id, INTEGER tag_id
 */
const char db_qc_admin[] = 
	"CREATE TABLE admin ("
		"version INTEGER NOT NULL ON CONFLICT ROLLBACK,"
		"last_updated INTEGER NOT NULL ON CONFLICT ROLLBACK"
	")";

const char db_qc_tags[] = 
	"CREATE TABLE tags ("
		"id INTEGER NOT NULL ON CONFLICT ROLLBACK "
			" PRIMARY KEY ON CONFLICT ROLLBACK AUTOINCREMENT,"
		"name TEXT NOT NULL ON CONFLICT ROLLBACK "
			" UNIQUE ON CONFLICT ROLLBACK,"
		"created_at INTEGER NOT NULL ON CONFLICT ROLLBACK,"
		"updated_at INTEGER NOT NULL ON CONFLICT ROLLBACK"
	")";

const char db_qc_data[] = 
	"CREATE TABLE data ("
		"id INTEGER NOT NULL ON CONFLICT ROLLBACK "
			" PRIMARY KEY ON CONFLICT ROLLBACK AUTOINCREMENT,"
		"value BLOB NOT NULL ON CONFLICT ROLLBACK "
			" UNIQUE ON CONFLICT ROLLBACK,"
		"created_at INTEGER NOT NULL ON CONFLICT ROLLBACK,"
		"updated_at INTEGER NOT NULL ON CONFLICT ROLLBACK"
	")";

const char db_qs_version[] = "SELECT version FROM admin LIMIT 1";

static struct {
	sqlite3 *handle;
} g;

static int db_version(int *version);

int db_init(const char *fpath) {
	int version;

	if(sqlite3_open(fpath, &g.handle) != SQLITE_OK) {
		err_warn(0, "failed to open sqlite db: %s", sqlite3_errmsg(g.handle));
		goto fail;
	}

	if(db_version(&version) != 0) {
		err_warn(0, "failed to get db version number");
		goto fail;
	}

	aug_log("db version: %d\n", version);
	return 0;

fail:
	sqlite3_close(g.handle);
	return -1;
}

/* #define SELECT_VAL(_hndl, _sql, _nbyte, _pp_stmt_ptr)  */
	
static int db_version(int *version) {
	int status;
	sqlite3_stmt *stmt;

	if( (status = sqlite3_prepare_v2(g.handle, db_qs_version, \
			ARRAY_SIZE(db_qs_version), &stmt, NULL)) != SQLITE_OK) {
		if(status == 1) {
			*version = 0;
			goto finalize;
		}
		else {
			err_warn(0, "failed to find db version: (%d) %s", status, sqlite3_errmsg(g.handle));
			goto fail;
		}
	}

	if(sqlite3_step(stmt) != SQLITE_ROW) {
		err_warn(0, "failed to get version data from the db: %s", sqlite3_errmsg(g.handle));
		goto fail;
	}

	*version = sqlite3_column_int(stmt, 0);

finalize:
	if(sqlite3_finalize(stmt) != SQLITE_OK) {
		err_warn(0, "failed to finalize statement: %s", sqlite3_errmsg(g.handle));
		return -1;
	}

	return 0;
fail:
	if(sqlite3_finalize(stmt) != SQLITE_OK)
		err_warn(0, "failed to finalize statement: %s", sqlite3_errmsg(g.handle));
	return -1;
}

void db_free() {
	if(sqlite3_close(g.handle) != SQLITE_OK)
		err_warn(0, "failed to close sqlite db: %s", sqlite3_errmsg(g.handle));
}

