#include "db.h"
#include "err.h"
#include "api_calls.h"

#include <ccan/array_size/array_size.h>
#include <sqlite3.h>

#define AUG_DB_SCHEMA_VERSION 1
/* SCHEMA
 *
 * version 1:
 * 		admin: INTEGER version, INTEGER last_updated
 *		tags: INTEGER id, TEXT name
 *		data: INTEGER id, BLOB value
 *		data_tags: INTEGER data_id, INTEGER tag_id
 */
const char db_qm_admin[] = 
	"CREATE TABLE admin ("
		"version INTEGER NOT NULL ON CONFLICT ROLLBACK, "
		"created_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now')),"
		"updated_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now'))"
	")";
const char db_qm_admin_populate[] = 
	"INSERT INTO admin (version) VALUES (1)";

const char db_qm_tags[] = 
	"CREATE TABLE tags ("
		"id INTEGER NOT NULL ON CONFLICT ROLLBACK "
			" PRIMARY KEY ON CONFLICT ROLLBACK AUTOINCREMENT,"
		"name TEXT NOT NULL ON CONFLICT ROLLBACK "
			" UNIQUE ON CONFLICT ROLLBACK,"
		"created_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now')),"
		"updated_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now'))"
	")";

const char db_qm_data[] = 
	"CREATE TABLE data ("
		"id INTEGER NOT NULL ON CONFLICT ROLLBACK "
			" PRIMARY KEY ON CONFLICT ROLLBACK AUTOINCREMENT,"
		"value BLOB NOT NULL ON CONFLICT ROLLBACK "
			" UNIQUE ON CONFLICT ROLLBACK,"
		"created_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now')),"
		"updated_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now'))"
	")";

const char db_qs_version[] = "SELECT version FROM admin LIMIT 1";

static struct {
	sqlite3 *handle;
} g;

static int db_version(int *version);
static int db_migrate();


#define db_execute(_query, _err_msg) \
	do { \
		if(sqlite3_exec(g.handle, _query, NULL, NULL, NULL) != SQLITE_OK) { \
			err_panic(0, _err_msg); \
		} \
	} while(0)

#define db_begin() \
	db_execute("BEGIN", "failed to begin db transaction")
#define db_commit() \
	db_execute("COMMIT", "failed to commit db transaction")

int db_init(const char *fpath) {
	if(sqlite3_open(fpath, &g.handle) != SQLITE_OK) {
		err_warn(0, "failed to open sqlite db: %s", sqlite3_errmsg(g.handle));
		goto fail;
	}

	if(db_migrate() != 0) {
		err_warn(0, "failed to migrate db");
		goto fail;
	}

	return 0;

fail:
	sqlite3_close(g.handle);
	return -1;
}

static int db_migrate_v1() {
	const char *query;

	aug_log("migrate to schema v1\n");
#define RUN_QM(_query) \
	do { \
		if(sqlite3_exec(g.handle, _query, NULL, NULL, NULL) != SQLITE_OK) { \
			query = _query; \
			goto fail; \
		} \
	} while(0)

	RUN_QM("BEGIN");
	RUN_QM(db_qm_admin);
	RUN_QM(db_qm_tags);
	RUN_QM(db_qm_data);
	RUN_QM(db_qm_admin_populate);
	RUN_QM("COMMIT");
#undef RUN_QM

	return 0;
	
fail:
	err_warn(0, "failed to execute query %s: %s", query, sqlite3_errmsg(g.handle));
	return -1;	
}

static int db_migrate() {
	int version;

	if(db_version(&version) != 0) {
		err_warn(0, "failed to get db version number");
		return -1;
	}
	aug_log("db version: %d\n", version);

	switch(version) {
	case 0:
		if(db_migrate_v1() != 0)
			return -1;
	case 1: /* fall through */
		/* current version */
		break;
	default:
		err_warn(0, "dont know how to migrate from db version %d", version);
		return -1;
	}		

	return 0;
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

