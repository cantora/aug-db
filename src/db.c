#include "db.h"
#include "err.h"
#include "api_calls.h"
#include "util.h"

#include <ccan/array_size/array_size.h>
#include <ccan/talloc/talloc.h>

#define AUG_DB_SCHEMA_VERSION 1
/* SCHEMA
 *
 * version 1:
 * 		admin: INTEGER version
 *		tags: INTEGER id, TEXT name
 *		blobs: INTEGER id, BLOB value
 *		fk_blobs_tags: INTEGER blob_id, INTEGER tag_id
 */
const char db_qm1_admin[] = 
	"CREATE TABLE admin ("
		"version INTEGER NOT NULL ON CONFLICT ROLLBACK, "
		"created_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now')),"
		"updated_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now'))"
	")";
const char db_qm1_admin_populate[] = 
	"INSERT INTO admin (version) VALUES (1)";

const char db_qm1_tags[] = 
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

const char db_qm1_blobs[] = 
	"CREATE TABLE blobs ("
		"id INTEGER NOT NULL ON CONFLICT ROLLBACK "
			" PRIMARY KEY ON CONFLICT ROLLBACK AUTOINCREMENT,"
		"value BLOB NOT NULL ON CONFLICT ROLLBACK "
			" UNIQUE ON CONFLICT ROLLBACK,"
		"created_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now')),"
		"updated_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now'))"
		"chosen_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now'))"
	")";

const char db_qm1_fk_blobs_tags[] = 
	"CREATE TABLE fk_blobs_tags ("
		"blob_id INTEGER NOT NULL ON CONFLICT ROLLBACK, "
		"tag_id INTEGER NOT NULL ON CONFLICT ROLLBACK, "
		"UNIQUE (blob_id, tag_id) ON CONFLICT ROLLBACK "
	")";

const char db_qs_version[] = "SELECT version FROM admin LIMIT 1";

static struct {
	sqlite3 *handle;
} g;

static int db_version(int *);
static int db_migrate();
static void db_query_fmt(size_t, size_t, char **);

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
	RUN_QM(db_qm1_admin);
	RUN_QM(db_qm1_tags);
	RUN_QM(db_qm1_blobs);
	RUN_QM(db_qm1_fk_blobs_tags);
	RUN_QM(db_qm1_admin_populate);
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


void db_query_prepare(struct db_query *query, const char **queries, size_t nqueries,
		const char **tags, size_t ntags) {
	char *sql;
	int status;
	size_t i;
	
	if(nqueries < 1 && ntags < 1) {
		sql = 
			"SELECT DISTINCT b.value, 0 AS score "
			"FROM blobs b " 
			"ORDER BY b.chosen_at DESC";
	}
	else 
		db_query_fmt(nqueries, ntags, &sql);

	if( (status = sqlite3_prepare_v2(g.handle, sql, \
			-1, &query->stmt, NULL)) != SQLITE_OK) {
		err_panic(0, "failed to prepare query %s: %s", sql, sqlite3_errmsg(g.handle));
	}

	for(i = 0; i < nqueries; i++) {
		sqlite3_bind_text(query->stmt, i+1, queries[i], -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(query->stmt, nqueries+ntags+(i+1), 
				queries[i], -1, SQLITE_TRANSIENT);
	}
	for(i = 0; i < ntags; i++) {
		sqlite3_bind_text(query->stmt, nqueries+i+1, tags[i], 
				-1, SQLITE_TRANSIENT);
		sqlite3_bind_text(query->stmt, nqueries*2+ntags+(i+1), 
				tags[i], -1, SQLITE_TRANSIENT);
	}

	if(!(nqueries < 1 && ntags < 1))
		talloc_free(sql);
}

void db_query_free(struct db_query *query) {
	if(sqlite3_finalize(query->stmt) != SQLITE_OK) {
		err_warn(0, "failed to finalize statement: %s", sqlite3_errmsg(g.handle));
	}
}

int db_query_step(struct db_query *query) {
	int status;
	if( (status = sqlite3_step(query->stmt)) != SQLITE_ROW ) {
		if(status == SQLITE_DONE) {
			sqlite3_reset(query->stmt);
			return -1;
		}
		else
			err_panic(0, "error while trying to get next row: %s", sqlite3_errmsg(g.handle));
	}

	return 0;
}

void db_query_value(struct db_query *query, char **value, size_t *size) {
	const unsigned char *txt;
	int n;
	if( (txt = sqlite3_column_text(query->stmt, 0)) == NULL)
		err_panic(0, "column data is NULL: %s", sqlite3_errmsg(g.handle));

	if( (n = sqlite3_column_bytes(query->stmt, 0)) < 0)
		err_panic(0, "value size is negative");

	*size = n;
	*value = talloc_array(NULL, char, *size);
	memcpy(*value, txt, *size);
}

static void db_query_fmt(size_t nqueries, size_t ntags, char **result) {
#define MAX_INPUTS 9
	char *q_fmt, *t_add_fmt, *t_fmt;
	char query_like[] = 
		"(b.value LIKE '%'||?||'%' OR t.name LIKE '%'||?||'%')";
	char tag_like[] = "(t.name LIKE ?||'%')";
	const char fmt1[] = 
		"SELECT DISTINCT b.value, ((%s)*10 + (%s)) AS score "
		"FROM blobs b " 
			"INNER JOIN fk_blobs_tags bt ON bt.blob_id = b.id " 
			"INNER JOIN tags t ON bt.tag_id = t.id "
		"WHERE %s AND (%s) "
		"ORDER BY score DESC, b.chosen_at DESC";

	if(nqueries < 1 && ntags < 1)
		err_panic(0, "must provide at least one query or tag");
	if(nqueries > MAX_INPUTS || ntags > MAX_INPUTS)
		err_panic(0, "too many input values");

	q_fmt = util_tal_multiply(NULL, query_like, " AND ", nqueries);
	t_add_fmt = util_tal_multiply(NULL, tag_like, "+", nqueries);
	t_fmt = util_tal_multiply(NULL, tag_like, " OR ", nqueries);
	
	*result = talloc_asprintf(NULL, fmt1, q_fmt, t_add_fmt, q_fmt, t_fmt);

	talloc_free(q_fmt);
	talloc_free(t_add_fmt);
	talloc_free(t_fmt);

	return;
}

