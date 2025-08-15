#include "utl.h"
#include "db.h"
#include "sqlite3.h"

// class Db

struct db_s {
    sqlite3 *db;
    sqlite3_stmt *stmt;  // or NULL
    BOOL debug;
};

Db *newDb(const char *dbname, BOOL debug) {
    Db *this = (Db *)memAlloc(sizeof(Db), __FILE__, __LINE__);
    this->stmt = NULL;
    this->debug = debug;
    int rc = sqlite3_open(dbname, &(this->db));
    if (this->debug) {
        LOG_DEBUG2("sqlite3_open '%s' rc=%d", dbname, rc);
    }
    if (rc != SQLITE_OK) {
        ASSERT_FAIL("sqlite3_open rc=%d (%s), errmsg='%s'", rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db))
    }
    ASSERT(this->db);
    return this;
}

void Db_free(Db *this) {
    ASSERT(this);
    ASSERT(this->db);
    int rc = sqlite3_close(this->db);
    if (rc != SQLITE_OK) {
        LOG_INFO3("sqlite3_close rc=%d (%s), errmsg='%s'", rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db));
    }
    memFree(this);
}

BOOL Db_prepare(Db *this, const char *sql) {
    ASSERT(this);
    ASSERT(this->db);
    ASSERT(!this->stmt);
    ASSERT(sql);
    int rc = sqlite3_prepare_v2(this->db, sql, -1, &(this->stmt), NULL);
    if (this->debug) {
        LOG_DEBUG2("sqlite3_prepare_v2 '%s' rc=%d", sql, rc);
    }
    if (rc != SQLITE_OK) {
        LOG_INFO4("sqlite3_prepare_v2 sql='%s', rc=%d (%s), errmsg='%s'", sql, rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db));
        this->stmt = NULL;
        return FALSE;
    }
    return TRUE;
}

void Db_finalize(Db *this) {
    ASSERT(this);
    ASSERT(this->db);
    if(this->stmt) {
        int rc = sqlite3_finalize(this->stmt);
        if (this->debug) {
            LOG_DEBUG2("sqlite3_finalize rc=%d", rc, rc);
        }
        if (rc != SQLITE_OK) {
            LOG_INFO3("sqlite3_finalize rc=%d (%s), errmsg='%s'", rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db));
        }
        this->stmt = NULL;
    }
}

BOOL _bind(Db *this, const Value *params, int nparams) {
    BOOL ok = TRUE;
    for (int i = 0; i < nparams; i++) {
        Value val = params[i];
        switch (val.type) {
            case VT_NULL: {
                int rc = sqlite3_bind_null(this->stmt, i + 1);
                if (this->debug) {
                    LOG_DEBUG2("sqlite3_bind_null rc=%d", rc, sqlite3_errstr(rc));
                }
                if (rc != SQLITE_OK) {
                    LOG_INFO3("sqlite3_bind_null rc=%d (%s), errmsg='%s'", rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db));
                    ok = FALSE;
                }
            } break;
            case VT_INT32: {
                int rc = sqlite3_bind_int(this->stmt, i + 1, val.i32);
                if (this->debug) {
                    LOG_DEBUG3("sqlite3_bind_int value=%d, rc=%d (%s)", val.i32, rc, sqlite3_errstr(rc));
                }
                if (rc != SQLITE_OK) {
                    LOG_INFO3("sqlite3_bind_int rc=%d (%s), errmsg='%s'", rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db));
                    ok = FALSE;
                }
            } break;
            case VT_INT64: {
                int rc = sqlite3_bind_int64(this->stmt, i + 1, val.i64);
                if (this->debug) {
                    LOG_DEBUG3("sqlite3_bind_int64 value=%ld, rc=%d (%s)", val.i64, rc, sqlite3_errstr(rc));
                }
                if (rc != SQLITE_OK) {
                    LOG_INFO3("sqlite3_bind_int64 rc=%d (%s), errmsg='%s'", rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db));
                    ok = FALSE;
                }
            } break;
            case VT_DOUBLE: {
                int rc = sqlite3_bind_double(this->stmt, i + 1, val.d);
                if (this->debug) {
                    LOG_DEBUG3("sqlite3_bind_double value=%f, rc=%d (%s)", val.d, rc, sqlite3_errstr(rc));
                }
                if (rc != SQLITE_OK) {
                    LOG_INFO3("sqlite3_bind_double rc=%d (%s), errmsg='%s'", rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db));
                    ok = FALSE;
                }
            } break;
            case VT_STRING: {
                int rc = sqlite3_bind_text(this->stmt, i + 1, val.p, -1, SQLITE_TRANSIENT);
                if (this->debug) {
                    LOG_DEBUG3("sqlite3_bind_text value='%s', rc=%d (%s)", val.p, rc, sqlite3_errstr(rc));
                }
                if (rc != SQLITE_OK) {
                    LOG_INFO3("sqlite3_bind_text rc=%d (%s), errmsg='%s'", rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db));
                    ok = FALSE;
                }
            } break;
            case VT_BLOB: {
                int rc = sqlite3_bind_blob(this->stmt, i + 1, val.p, val.sz, SQLITE_TRANSIENT);
                if (this->debug) {
                    LOG_DEBUG3("sqlite3_bind_blob value=[%d], rc=%d (%s)", val.sz, rc, sqlite3_errstr(rc));
                }
                if (rc != SQLITE_OK) {
                    LOG_INFO3("sqlite3_bind_blob rc=%d (%s), errmsg='%s'", rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db));
                    ok = FALSE;
                }
            } break;
            default:
                ASSERT_FAIL("Db_bind: invalid param type %d, i=%d", val.type, i);
        }
    }
    return ok;
}

BOOL _step(Db *this, BOOL *phasRowOrNull) {
    int rc = sqlite3_step(this->stmt);
    if (this->debug) {
        LOG_DEBUG2("sqlite3_step rc=%d (%s)", rc, sqlite3_errstr(rc));
    }
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        LOG_INFO3("sqlite3_step rc=%d (%s), errmsg='%s'", rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db));
        return FALSE;
    }
    if (phasRowOrNull) {
        *phasRowOrNull = rc == SQLITE_ROW;
    }
    return TRUE;
}

void _reset(Db *this) {
    if(this->stmt){
        int rc = sqlite3_reset(this->stmt);
        if (this->debug) {
            LOG_DEBUG1("sqlite3_reset rc=%d", rc);
        }
        if (rc != SQLITE_OK) {
            LOG_INFO3("sqlite3_reset rc=%d (%s), errmsg='%s'", rc, sqlite3_errstr(rc), sqlite3_errmsg(this->db));
        }
    }
}

BOOL Db_bind(Db *this, const Value *params, int nparams) {
    ASSERT(this);
    ASSERT(this->db);
    if (!this->stmt) {
        return FALSE;
    }
    if (!nparams) {
        return TRUE;
    }
    ASSERT(params);
    return _bind(this, params, nparams);
}

BOOL Db_bind_step_reset(Db *this, const Value *params, int nparams) {
    ASSERT(this);
    ASSERT(this->db);
    if(!this->stmt){
        return FALSE;
    }
    BOOL ok = _bind(this, params, nparams);
    if (ok) {
        ok = _step(this, NULL);
        _reset(this);
    }
    return ok;
}

BOOL _fetch(Db *this, Value *values, int nvalues) {
    for (int i = 0; i < nvalues; i++) {
        BOOL isNull = sqlite3_column_type(this->stmt, i) == SQLITE_NULL;
        if (isNull) {
            values[i].type = VT_NULL;
            if (this->debug) {
                LOG_DEBUG0("sqlite3_column_type = SQLITE_NULL");
            }
        } else {
            switch (values[i].type) {
                case VT_INT32:
                    values[i].i32 = sqlite3_column_int(this->stmt, i);
                    if (this->debug) {
                        LOG_DEBUG1("sqlite3_column_int v=%ld", values[i].i32);
                    }
                    break;
                case VT_INT64:
                    values[i].i64 = (int64_t)sqlite3_column_int64(this->stmt, i);
                    if (this->debug) {
                        LOG_DEBUG1("sqlite3_column_int64 v=%ld", values[i].i64);
                    }
                    break;
                case VT_DOUBLE:
                    values[i].d = sqlite3_column_double(this->stmt, i);
                    if (this->debug) {
                        LOG_DEBUG1("sqlite3_column_double v=%f", values[i].d);
                    }
                    break;
                case VT_STRING:
                    values[i].p = (const char *)sqlite3_column_text(this->stmt, i);
                    values[i].sz = sqlite3_column_bytes(this->stmt, i);
                    if (this->debug) {
                        LOG_DEBUG2("sqlite3_column_text v=[%d]'%s'", values[i].sz, values[i].p);
                    }
                    break;
                case VT_BLOB:
                    values[i].p = (const char *)sqlite3_column_blob(this->stmt, i);
                    values[i].sz = sqlite3_column_bytes(this->stmt, i);
                    if (this->debug) {
                        LOG_DEBUG1("sqlite3_column_blob v=[%d]", values[i].sz);
                    }
                    break;
                default:
                    ASSERT_FAIL("_fetch: invalid values[%d].type %d", i, values[i].type);
            }
        }
    }
    return TRUE;
}

BOOL Db_step_fetch(Db *this, BOOL *phasRow, Value *values, int nvalues) {    
    ASSERT(this);
    ASSERT(this->db);
    ASSERT(phasRow);
    if(!this->stmt){
        return FALSE;
    }
    BOOL ok = _step(this, phasRow);
    if (ok && *phasRow){    
        ok = _fetch(this, values, nvalues);
    }
    return ok;
}

const char *Db_errmsg(Db *this){
    ASSERT(this);
    ASSERT(this->db);
    return sqlite3_errmsg(this->db);
}

// TEST

static void sq_open(const char *dbname, sqlite3 **pdb) {
    int rc = sqlite3_open(dbname, pdb);
    ASSERT(rc == SQLITE_OK);
}

static void sq_close(sqlite3 *db) {
    int rc = sqlite3_close(db);
    ASSERT(rc == SQLITE_OK);
}

static void sq_prepare(sqlite3 *db, const char *sql, sqlite3_stmt **pstmt) {
    int rc = sqlite3_prepare(db, sql, -1, pstmt, NULL);
    ASSERT_INT(SQLITE_OK, rc);
}

static BOOL sq_step(sqlite3_stmt *stmt) {
    // int sqlite3_step(sqlite3_stmt*);
    int rc = sqlite3_step(stmt);
    ASSERT(rc == SQLITE_ROW || rc == SQLITE_DONE);
    return rc == SQLITE_DONE;
}

static void sq_finalize(sqlite3_stmt *stmt) {
    // int sqlite3_finalize(sqlite3_stmt *pStmt);
    int rc = sqlite3_finalize(stmt);
    ASSERT(rc == SQLITE_OK);
}

static void sq_exec(sqlite3 *db, const char *sql) {
    sqlite3_stmt *stmt;
    sq_prepare(db, sql,  &stmt);
    ASSERT(sq_step(stmt));
    sq_finalize(stmt);
}

static int sq_queryInt(sqlite3 *db, const char *sql) {
    sqlite3_stmt *stmt;
    sq_prepare(db, sql,  &stmt);
    ASSERT(!sq_step(stmt));
    int value = sqlite3_column_int(stmt, 0);
    sq_finalize(stmt);
    return value;
}

static void testSqlite() {
    sqlite3 *db = NULL;
    sq_open(":memory:", &db);
    sq_exec(db, "CREATE TABLE users(name TEXT)");
    {
        sqlite3_stmt *stmt;
        sq_prepare(db, "INSERT INTO users(name) VALUES (?)", &stmt);
        {
            // int sqlite3_bind_text(sqlite3_stmt*,int paramOneBased ,const char* value,int lenOrNegative, void(*)(void*) freefunc );
            ASSERT(sqlite3_bind_text(stmt, 1, "Alice", -1, SQLITE_STATIC) == SQLITE_OK);
            ASSERT(sq_step(stmt));
        }
        sq_finalize(stmt);
    }
    {
        sqlite3_stmt *stmt;
        sq_prepare(db, "SELECT name FROM users ORDER BY name", &stmt);
        ASSERT(!sq_step(stmt));
        ASSERT(sqlite3_column_type(stmt, 0) == SQLITE3_TEXT);
        ASSERT_STR("Alice", (const char *)sqlite3_column_text(stmt, 0));
        ASSERT(sqlite3_step(stmt));
        sq_finalize(stmt);
    }
    sq_close(db);
}

static void testSqliteStmtCaching() {
    const int nrounds = 2;
    const int ninserts = 1000;
    for (int r = 0; r < nrounds; r++) {
        {
            clock_t t0 = clock();
            sqlite3 *db = NULL;
            sq_open(":memory:", &db);
            sq_exec(db, "CREATE TABLE users(id INTEGER PRIMARY KEY NOT NULL)");
            for (int i = 0; i < ninserts; i++) {
                sq_exec(db, "BEGIN");
                sqlite3_stmt *stmt;
                sq_prepare(db, "INSERT INTO users(id) VALUES (?)", &stmt);
                ASSERT_INT(SQLITE_OK, sqlite3_bind_int(stmt, 1, i));
                ASSERT(sq_step(stmt));
                sq_finalize(stmt);
                sq_exec(db, "COMMIT");
            }
            clock_t t1 = clock();
            LOG_INFO1("testSqliteStmtCaching without caching took %4ld clocks", t1 - t0);
            int count = sq_queryInt(db, "SELECT COUNT(*) FROM users");
            ASSERT_INT(ninserts, count);
            sq_close(db);
        }
        {
            clock_t t0 = clock();
            sqlite3 *db = NULL;
            sq_open(":memory:", &db);
            sq_exec(db, "CREATE TABLE users(id INTEGER PRIMARY KEY NOT NULL)");
            sqlite3_stmt *stmtBegin;
            sq_prepare(db, "BEGIN", &stmtBegin);
            sqlite3_stmt *stmtInsert;
            sq_prepare(db, "INSERT INTO users(id) VALUES (?)", &stmtInsert);
            sqlite3_stmt *stmtCommit;
            sq_prepare(db, "COMMIT", &stmtCommit);
            for (int i = 0; i < ninserts; i++) {
                // BEGIN
                sqlite3_reset(stmtBegin);
                ASSERT(sq_step(stmtBegin));
                // INSERT INTO..
                sqlite3_reset(stmtInsert);
                ASSERT_INT(SQLITE_OK, sqlite3_bind_int(stmtInsert, 1, i));
                ASSERT(sq_step(stmtInsert));
                // COMMIT
                sqlite3_reset(stmtCommit);
                ASSERT(sq_step(stmtCommit));
            }
            sq_finalize(stmtBegin);
            sq_finalize(stmtInsert);
            sq_finalize(stmtCommit);
            clock_t t1 = clock();
            LOG_INFO1("testSqliteStmtCaching with    caching took %4ld clocks", t1 - t0);
            int count = sq_queryInt(db, "SELECT COUNT(*) FROM users");
            ASSERT_INT(ninserts, count);
            sq_close(db);
        }
    }
    // TODO Statement caching is a thing.
    // without caching took 1658 clocks
    // with    caching took  517 clocks
    // without caching took 1521 clocks
    // with    caching took  512 clocks
}

static void testMemoryDb() {
    Db *db = newDb(":memory:", FALSE);
    // CREATE TABLE users
    {
        ASSERT(Db_prepare(db, "CREATE TABLE users(i INTEGER PRIMARY KEY, d FLOAT, s TEXT, b BLOB)"));
        ASSERT(Db_bind_step_reset(db, NULL, 0));
        // ASSERT(Db_step(db, NULL));
        // Db_reset(db);
        Db_finalize(db);
    }
    // INSERT INTO users
    {
        ASSERT(Db_prepare(db, "INSERT INTO users(i,d,s,b) VALUES(?,?,?,?)"));
        // user 1
        Value params[] = {
            {.type = VT_INT64, .i64 = 1},
            {.type = VT_DOUBLE, .d = 13.14},
            {.type = VT_STRING, .p = "Alice" },
            {.type = VT_BLOB, .p = "aaaaaaaaa_aaaaaaaaa_", .sz=20 },
        };
        ASSERT(Db_bind_step_reset(db, params, 4));
        // user 2
        params[0] = (Value) {.type = VT_INT64, .i64 = 2};
        params[1] = (Value) {.type = VT_DOUBLE, .d = 23.14};
        params[2] = (Value) {.type = VT_STRING, .p = "Bob" };
        params[3] = (Value) {.type = VT_BLOB, .p = "bbbbbbbbb_bbbbbbbbb_", .sz=10 };
        ASSERT(Db_bind_step_reset(db, params, 4));
        Db_finalize(db);
    }
    // SELECT * FROM users
    {
        ASSERT(Db_prepare(db, "SELECT i,d,s,b FROM users WHERE i>? ORDER BY i"));
        {
            Value param = {.type = VT_INT64, .i64 = 0};
            ASSERT(Db_bind(db, &param, 1));
        }
        // row
        {
            BOOL hasRow;
            Value values[] = {
                {.type = VT_INT64},
                {.type = VT_DOUBLE},
                {.type = VT_STRING},
                {.type = VT_BLOB},
            };
            // row
            ASSERT(Db_step_fetch(db, &hasRow, values, 4));
            ASSERT(hasRow);
            ASSERT_INT(VT_INT64, values[0].type);
            ASSERT_INT64(1, values[0].i64);
            ASSERT_INT(VT_DOUBLE, values[1].type);
            ASSERT_DOUBLE(13.14, values[1].d);
            ASSERT_INT(VT_STRING, values[2].type);
            ASSERT_STR("Alice", values[2].p);
            ASSERT_INT(VT_BLOB, values[3].type);
            ASSERT_INT(20, values[3].sz);
            ASSERT_INT(0, memcmp(values[3].p, "aaaaaaaaa_aaaaaaaaa_", 20));
            // row
            ASSERT(Db_step_fetch(db, &hasRow, values, 4));
            ASSERT(hasRow);
            ASSERT_INT(VT_INT64, values[0].type);
            ASSERT_INT64(2, values[0].i64);
            ASSERT_INT(VT_DOUBLE, values[1].type);
            ASSERT_DOUBLE(23.14, values[1].d);
            ASSERT_INT(VT_STRING, values[2].type);
            ASSERT_STR("Bob", values[2].p);
            ASSERT_INT(VT_BLOB, values[3].type);
            ASSERT_INT(10, values[3].sz);
            ASSERT_INT(0, memcmp(values[3].p, "bbbbbbbbb_", 10));
            // no more rows
            ASSERT(Db_step_fetch(db, &hasRow, values, 4));
            ASSERT(!hasRow);
        }
        Db_finalize(db);
    }
    // done
    Db_free(db);
}

static void testErrors() {
    Db *db = newDb(":memory:", TRUE);
    // good
    ASSERT(Db_prepare(db, "CREATE TABLE users(i INTEGER)"));
    ASSERT_STR("not an error", Db_errmsg(db));
    Db_finalize(db);
    ASSERT_STR("not an error", Db_errmsg(db));
    // bad
    ASSERT(!Db_prepare(db, "CREATE TABLE users with blablbla"));
    ASSERT_STR("near \"with\": syntax error", Db_errmsg(db));
    Db_finalize(db);
    ASSERT_STR("near \"with\": syntax error", Db_errmsg(db));
    // good
    ASSERT(Db_prepare(db, "CREATE TABLE users(i INTEGER)"));
    ASSERT_STR("not an error", Db_errmsg(db));
    Db_finalize(db);
    ASSERT_STR("not an error", Db_errmsg(db));
    // bad
    ASSERT(!Db_prepare(db, "INSERT INTO blabla(i) VALUES(?)"));
    ASSERT_STR("no such table: blabla", Db_errmsg(db));
    Db_finalize(db);
    ASSERT_STR("no such table: blabla", Db_errmsg(db));
    Db_finalize(db);
    ASSERT_STR("no such table: blabla", Db_errmsg(db));
    //
    Db_free(db);
}

void testDb() {
    LOG_INFO0("testDb testSqlite");
    testSqlite();
    LOG_INFO0("testDb testSqliteStmtCaching");
    testSqliteStmtCaching();
    LOG_INFO0("testDb testMemoryDb");
    testMemoryDb();
    LOG_INFO0("testDb testErrors");
    testErrors();
}
