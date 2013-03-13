#include "db.h"
#include "err.h"
#include "api_calls.h"
#include "util.h"

#include <ccan/array_size/array_size.h>
#include <ccan/talloc/talloc.h>
#include <ccan/str/str.h>

#define AUG_DB_SCHEMA_VERSION 1
/* SCHEMA
 *
 * version 1:
 * 		admin: INTEGER version
 *		tags: INTEGER id, TEXT name
 *		blobs: INTEGER id, BLOB value, INTEGER raw
 *		fk_blobs_tags: INTEGER blob_id, INTEGER tag_id
 *
 * if raw = 0, the blob value will be interpreted as 
 * utf-8 encoded text. if raw != 0, then the blob
 * value will be interpreted as raw bytes, so no
 * decoding will take place.
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
			"PRIMARY KEY ON CONFLICT ROLLBACK AUTOINCREMENT,"
		"value BLOB NOT NULL ON CONFLICT ROLLBACK "
			"UNIQUE ON CONFLICT ROLLBACK, "
		"raw INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT 0, "
		"created_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now')), "
		"updated_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (strftime('%s','now')), "
		"chosen_at INTEGER NOT NULL ON CONFLICT ROLLBACK "
			"DEFAULT (0)"
	")";

const char db_qm1_fk_blobs_tags[] = 
	"CREATE TABLE fk_blobs_tags ("
		"blob_id INTEGER NOT NULL ON CONFLICT ROLLBACK, "
		"tag_id INTEGER NOT NULL ON CONFLICT ROLLBACK, "
		"UNIQUE (blob_id, tag_id) ON CONFLICT IGNORE "
	")";

const char db_qs_version[] = "SELECT version FROM admin LIMIT 1";

static struct {
	sqlite3 *handle;
} g;

static int db_version(int *);
static int db_migrate();

static void db_query_fmt(size_t, size_t, char **);

#define DB_EXECUTE(_query, _err_msg) \
	do { \
		if(sqlite3_exec(g.handle, _query, NULL, NULL, NULL) != SQLITE_OK) { \
			err_panic(0, _err_msg); \
		} \
	} while(0)

#define DB_BEGIN() \
	do { \
		aug_log("BEGIN transaction\n"); \
		DB_EXECUTE("BEGIN", "failed to begin db transaction"); \
	} while(0)
#define DB_COMMIT() \
	do { \
		aug_log("COMMIT transaction\n"); \
		DB_EXECUTE("COMMIT", "failed to commit db transaction"); \
	} while(0)

#define DB_ROLLBACK() \
	do { \
		aug_log("ROLLBACK transaction\n"); \
		DB_EXECUTE("ROLLBACK", "failed to rollback db transaction"); \
	} while(0)

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
			goto rollback; \
		} \
	} while(0)

	if(sqlite3_exec(g.handle, "BEGIN", NULL, NULL, NULL) != SQLITE_OK) {
		query = "BEGIN";
		goto fail;
	}
	RUN_QM(db_qm1_admin);
	RUN_QM(db_qm1_tags);
	RUN_QM(db_qm1_blobs);
	RUN_QM(db_qm1_fk_blobs_tags);
	RUN_QM(db_qm1_admin_populate);
	RUN_QM("COMMIT");
#undef RUN_QM

	return 0;

rollback:
	if(sqlite3_exec(g.handle, "ROLLBACK", NULL, NULL, NULL) != SQLITE_OK)
		err_warn(0, "failed to rollback: %s", sqlite3_errmsg(g.handle));
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

static int db_version(int *version) {
	int status;
	sqlite3_stmt *stmt;

	if( (status = sqlite3_prepare_v2(g.handle, db_qs_version, \
			ARRAY_SIZE(db_qs_version), &stmt, NULL)) != SQLITE_OK) {
		if(status == SQLITE_ERROR) { /* sql error => the table doesnt exist */
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


#define DB_STMT_PREP(_sql, _stmt_pptr) \
	do { \
		if(sqlite3_prepare_v2(g.handle, _sql, -1, _stmt_pptr, NULL) != SQLITE_OK) { \
			err_panic(0, "failed to prepare %s: %s", \
							_sql, sqlite3_errmsg(g.handle)); \
		} \
	} while(0)

#define DB_STMT_FINALIZE(_stmt_ptr) \
	do { \
		if(sqlite3_finalize(_stmt_ptr) != SQLITE_OK) { \
			err_warn(0, "failed to finalize statement: %s", sqlite3_errmsg(g.handle)); \
		} \
	} while(0) 

#define DB_STMT_RESET(_stmt_ptr) \
	do { \
		if(sqlite3_reset(_stmt_ptr) != SQLITE_OK) { \
			err_panic(0, "expected reset to return SQLITE_OK: %s", sqlite3_errmsg(g.handle)); \
		} \
	} while(0)

static int db_stmt_step(sqlite3_stmt *stmt) {
	int status;

	if( (status = sqlite3_step(stmt)) != SQLITE_ROW) {
		if(status == SQLITE_DONE)
			return -1;
		else 
			err_panic(0, "failed to step: %s", sqlite3_errmsg(g.handle));
	}
	
	return 0;
}

#define DB_STMT_EXEC(_stmt_ptr) \
	do { \
		if(db_stmt_step(_stmt_ptr) == 0) { \
			err_panic(0, "expected SQLITE_DONE"); \
		} \
		DB_STMT_FINALIZE(_stmt_ptr); \
	} while(0)

#define DB_BIND_BUF(_type, _stmt_ptr, _idx, _data, _len, _dtor_type) \
	do { \
		if(sqlite3_bind_ ## _type (_stmt_ptr, _idx, _data, _len, _dtor_type) != SQLITE_OK) { \
			err_panic(0, "failed to bind " stringify(_type) ": %s", sqlite3_errmsg(g.handle)); \
		} \
	} while(0) 

#define DB_BIND_BUF_STATIC(_type, _stmt_ptr, _idx, _data, _len) \
	DB_BIND_BUF(_type, _stmt_ptr, _idx, _data, _len, SQLITE_STATIC)

#define DB_BIND_BLOB(_stmt_ptr, _idx, _data, _len) \
	DB_BIND_BUF_STATIC(blob, _stmt_ptr, _idx, _data, _len)

#define DB_BIND_TEXT(_stmt_ptr, _idx, _data) \
	DB_BIND_BUF_STATIC(text, _stmt_ptr, _idx, _data, -1)

#define DB_BIND_INT(_stmt_ptr, _idx, _val) \
	do { \
		if(sqlite3_bind_int(_stmt_ptr, _idx, _val) != SQLITE_OK) { \
			err_panic(0, "failed to bind int: %s", sqlite3_errmsg(g.handle)); \
		} \
	} while(0) 

#define DB_BIND_PRM_IDX(_stmt_ptr, _name, _idx_ptr) \
	do { \
		if( ((*_idx_ptr) = sqlite3_bind_parameter_index(_stmt_ptr, _name)) < 1) { \
			err_panic(0, "failed to find index for %s", _name); \
		} \
	} while(0)
		
static int db_blob_id(const void *data, size_t bytes) {
	int id;
	sqlite3_stmt *stmt;
	
	id = 0;
	DB_STMT_PREP("SELECT id FROM blobs WHERE value = ?", &stmt);
	DB_BIND_BLOB(stmt, 1, data, bytes);
	if(db_stmt_step(stmt) != 0) 
		goto done; /* no rows in the blobs table */

	id = sqlite3_column_int(stmt, 0);
done:
	DB_STMT_FINALIZE(stmt);
	return id;
}

/* this function should be run within a transaction */
static int db_find_or_create_blob(const void *data, size_t bytes, int raw) {
	sqlite3_stmt *stmt;
	int bid;

	if( (bid = db_blob_id(data, bytes)) > 0) {
		return bid;
	}
	
	DB_STMT_PREP("INSERT INTO blobs (value, raw) VALUES (?, ?)", &stmt);
	DB_BIND_BLOB(stmt, 1, data, bytes);
	DB_BIND_INT(stmt, 2, raw);
	DB_STMT_EXEC(stmt);

	bid = sqlite3_last_insert_rowid(g.handle);
	return bid;
}

static int db_tag_id(const char *tag) {
	sqlite3_stmt *stmt;
	int id;

	id = 0;
	DB_STMT_PREP("SELECT id FROM tags WHERE name = ?", &stmt);
	DB_BIND_TEXT(stmt, 1, tag);
	if(db_stmt_step(stmt) != 0) 
		goto done; /* no rows in the tags table */

	id = sqlite3_column_int(stmt, 0);
done:
	DB_STMT_FINALIZE(stmt);
	return id;
}

/* this should be run within a transaction */
static int db_find_or_create_tag(const char *tag) {
	int tid;
	sqlite3_stmt *stmt;

	if( (tid = db_tag_id(tag)) > 0) {
		return tid;
	}

	DB_STMT_PREP("INSERT INTO tags (name) VALUES (?)", &stmt);
	DB_BIND_TEXT(stmt, 1, tag);
	DB_STMT_EXEC(stmt);

	tid = sqlite3_last_insert_rowid(g.handle);
	return tid;
}

static void db_tag_blob(int bid, const char **tags, size_t ntags) {
	int tid;
	size_t i;
	sqlite3_stmt *stmt;
	const char *sql = 
		"INSERT INTO fk_blobs_tags (blob_id, tag_id) "
		"VALUES (?, ?)";

	err_assert(bid > 0);

	if(ntags < 1)
		return;

	DB_STMT_PREP(sql, &stmt);
	DB_BIND_INT(stmt, 1, bid);
	for(i = 0; i < ntags; i++) {
		if(i > 0)
			DB_STMT_RESET(stmt);

		tid = db_find_or_create_tag(tags[i]);
		DB_BIND_INT(stmt, 2, tid);
		if(db_stmt_step(stmt) == 0)
			err_panic(0, "expected SQLITE_DONE from %s: ", sql);
	}

	DB_STMT_FINALIZE(stmt);
}

void db_add(const void *data, size_t bytes, int raw, const char **tags, size_t ntags) {
	int bid;
	
	DB_BEGIN();
	bid = db_find_or_create_blob(data, bytes, raw);
	db_tag_blob(bid, tags, ntags);
	DB_COMMIT();
}

#define DB_QUERY_COLUMNS "b.value, b.raw, b.id"
#define DB_QUERY_LIMIT "LIMIT 200 OFFSET @offset"

void db_query_prepare(struct db_query *query, unsigned int offset, const uint8_t **queries, 
		size_t nqueries, const uint8_t **tags, size_t ntags) {
	char *sql;
	size_t i;
	int offset_idx;
	
	if(nqueries < 1 && ntags < 1) {
		sql = 
			"SELECT DISTINCT " 
				DB_QUERY_COLUMNS ", 0 AS score "
			"FROM blobs b " 
			"ORDER BY b.chosen_at DESC "
			DB_QUERY_LIMIT ;
	}
	else 
		db_query_fmt(nqueries, ntags, &sql);

	DB_STMT_PREP(sql, &query->stmt);
	aug_log("db: prepare sql (%p) %s\n", query->stmt, sql);

#define DB_QP_BIND(_idx, _ptr) \
	DB_BIND_BUF(text, query->stmt, _idx, _ptr, -1, SQLITE_TRANSIENT)

	for(i = 0; i < nqueries; i++) {
		/*aug_log("bind %s to ?(%d)\n", queries[i], i+1);*/
		DB_QP_BIND(i+1, (const char *) queries[i]);
	}
	for(i = 0; i < ntags; i++) {
		/*aug_log("bind %s to ?(%d)\n", tags[i], nqueries+i+1);*/
		DB_QP_BIND(nqueries+i+1, (const char *) tags[i]);
	}
#undef DB_QP_BIND

	DB_BIND_PRM_IDX(query->stmt, "@offset", &offset_idx);
	DB_BIND_INT(query->stmt, offset_idx, offset);

	if(!(nqueries < 1 && ntags < 1))
		talloc_free(sql);
}

void db_query_free(struct db_query *query) {
	aug_log("db: free %p\n", query->stmt);
	DB_STMT_FINALIZE(query->stmt);
	query->stmt = NULL;
}

int db_query_step(struct db_query *query) {
	if(db_stmt_step(query->stmt) != 0) {
		db_query_reset(query);
		return -1;
	}

	return 0;
}

void db_query_reset(struct db_query *query) {
	DB_STMT_RESET(query->stmt);
}

void db_query_value(struct db_query *query, uint8_t **value, size_t *size, 
		int *raw, int *id) {
	const void *data;
	int n;
	
	*raw = sqlite3_column_int(query->stmt, 1);
	*id = sqlite3_column_int(query->stmt, 2);
	if( (data = sqlite3_column_blob(query->stmt, 0)) == NULL)
		err_panic(0, "column data is NULL: %s", sqlite3_errmsg(g.handle));
	if( (n = sqlite3_column_bytes(query->stmt, 0)) < 0)
		err_panic(0, "value size is negative");

	*size = n;
	if(*size > 0) {
		*value = talloc_array(NULL, uint8_t, *size);
		memcpy(*value, data, *size);
	}
}

void db_update_chosen_at(int id) {
	sqlite3_stmt *stmt;

	DB_BEGIN();
	DB_STMT_PREP(
		"UPDATE blobs "
			"SET chosen_at = strftime('%s','now') " 
			"WHERE id = ?", 
		&stmt
	);
	DB_BIND_INT(stmt, 1, id);
	if(db_stmt_step(stmt) != -1)
		err_panic(0, "didnt expect statement to return rows");

	DB_STMT_FINALIZE(stmt);	
	DB_COMMIT();
}

static void db_query_fmt(size_t nqueries, size_t ntags, char **result) {
#define DB_QUERY_MAX_INPUTS 9 /* max 9 queries and 9 tags */
	char *q_score_fmt, *q_fmt, *t_score_fmt, *t_fmt;
	size_t i;
	char query_like[] = 
		"(b.value LIKE '%%'||?%03d||'%%' OR t.name LIKE '%%'||?%03d||'%%')";
	char tag_like[] = "(t.name LIKE '%%'||?%03d||'%%')";
	const char fmt1[] = 
		"SELECT DISTINCT "
			DB_QUERY_COLUMNS ", ((%s)*10 + (%s)) AS score "
		"FROM blobs b " 
			"INNER JOIN fk_blobs_tags bt ON bt.blob_id = b.id " 
			"INNER JOIN tags t ON bt.tag_id = t.id "
		"WHERE %s AND (%s) "
		"ORDER BY score DESC, b.chosen_at DESC "
		DB_QUERY_LIMIT;

	if(nqueries < 1 && ntags < 1)
		err_panic(0, "must provide at least one query or tag");
	if(nqueries > DB_QUERY_MAX_INPUTS || ntags > DB_QUERY_MAX_INPUTS)
		err_panic(0, "too many input values");

	if(nqueries > 0) {
		char **list = talloc_array(NULL, char *, nqueries+1);
		list[nqueries] = NULL;
		for(i = 0; i < nqueries; i++) {
			list[i] = talloc_asprintf(list, query_like, i+1, i+1);
		}
		
		q_score_fmt = util_tal_join(NULL, list, " AND ");
		q_fmt = q_score_fmt;
		talloc_free(list); /* asprintf's are freed here too */
	}
	else {
		q_score_fmt = "0";
		q_fmt = "1";
	}
	/*aug_log("db: q_score_fmt => %s\n", q_score_fmt);*/
	/*aug_log("db: q_fmt => %s\n", q_fmt);*/

	if(ntags > 0) {
		char **list = talloc_array(NULL, char *, ntags+1);
		list[ntags] = NULL;
		for(i = 0; i < ntags; i++) {
			list[i] = talloc_asprintf(list, tag_like, nqueries + i+1);
		}

		t_score_fmt = util_tal_join(NULL, list, "+");
		t_fmt = util_tal_join(NULL, list, " OR ");
		talloc_free(list); /* asprintf's are freed here too */
	}
	else {
		t_score_fmt = "0";
		t_fmt = "1";
	}
	/*aug_log("db: t_score_fmt => %s\n", t_score_fmt);*/
	/*aug_log("db: t_fmt => %s\n", t_fmt);*/
	
	*result = talloc_asprintf(NULL, fmt1, q_score_fmt, t_score_fmt, q_fmt, t_fmt);

	if(nqueries > 0) 
		talloc_free(q_score_fmt);
	
	if(ntags > 0) {
		talloc_free(t_score_fmt);
		talloc_free(t_fmt);
	}

	return;
}

