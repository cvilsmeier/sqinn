#include "util.h"
#include "mem.h"
#include "conn.h"
#include "handler.h"
#include "handler_test.h"


void test_handler_versions() {
    INFO("TEST %s\n", __func__);
    handler *hd = handler_new();
    dbuf *req = dbuf_new();
    dbuf *resp = dbuf_new();
    // sqinn version
    dbuf_reset(req);
    dbuf_reset(resp);
    dbuf_write_byte(req, FC_SQINN_VERSION);
    handler_handle(hd, req, resp);
    ASSERT0(dbuf_read_bool(resp), "was not ok");
    const char *sqinn_version = dbuf_read_string(resp);
    ASSERT(strcmp(sqinn_version, SQINN_VERSION)==0, "wrong sqinn_version %s", sqinn_version);
    // io protocol version
    dbuf_reset(req);
    dbuf_reset(resp);
    dbuf_write_byte(req, FC_IO_VERSION);
    handler_handle(hd, req, resp);
    ASSERT0(dbuf_read_bool(resp), "was not ok");
    byte io_version = dbuf_read_byte(resp);
    ASSERT(io_version==1, "wrong io_version %d", io_version);
    // sqlite version
    dbuf_reset(req);
    dbuf_reset(resp);
    dbuf_write_byte(req, FC_SQLITE_VERSION);
    handler_handle(hd, req, resp);
    ASSERT0(dbuf_read_bool(resp), "was not ok");
    const char *sqlite_version = dbuf_read_string(resp);
    ASSERT(strcmp(sqlite_version, "3.49.2")==0, "wrong sqlite_version %s", sqlite_version);
    // cleanup
    dbuf_free(req);
    dbuf_free(resp);
    handler_free(hd);
    INFO("TEST %s OK\n", __func__);
}

void _prep_step_fin(handler *hd, dbuf *req, dbuf *resp, const char *sql) {
    dbuf_reset(req);
    dbuf_reset(resp);
    dbuf_write_byte(req, FC_PREPARE);
    dbuf_write_string(req, sql);
    handler_handle(hd, req, resp);
    ASSERT0(dbuf_read_bool(resp), "was not ok");
    dbuf_reset(req);
    dbuf_reset(resp);
    dbuf_write_byte(req, FC_STEP);
    handler_handle(hd, req, resp);
    ASSERT0(dbuf_read_bool(resp), "was not ok");
    bool more = dbuf_read_bool(resp);
    ASSERT(!more, "wrong more %d", more);
    dbuf_reset(req);
    dbuf_reset(resp);
    dbuf_write_byte(req, FC_FINALIZE);
    handler_handle(hd, req, resp);
    ASSERT0(dbuf_read_bool(resp), "was not ok");
}

void test_handler_functions(const char *dbfile) {
    INFO("TEST %s\n", __func__);
    DEBUG("  dbfile=%s\n", dbfile);
    handler *hd = handler_new();
    dbuf *req = dbuf_new();
    dbuf *resp = dbuf_new();
    bool ok;
    // open db
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_OPEN);
        dbuf_write_string(req, dbfile);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
    }
    // prepare schema
    {
        _prep_step_fin(hd, req, resp, "DROP TABLE IF EXISTS users");
        _prep_step_fin(hd, req, resp, "CREATE TABLE users (id INTEGER PRIMARY KEY NOT NULL, name VARCHAR, age BIGINT, rating REAL, balance REAL, image BLOB)");
    }
    // insert user
    int id = 1;
    const char *name = "Alice";
    int64 age = ((int64)2 << 62) + 3;
    double rating = 13.24;
    double balance = 32.0;
    byte image[128];
    {
        _prep_step_fin(hd, req, resp, "BEGIN");
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_PREPARE);
        dbuf_write_string(req, "INSERT INTO users (id, name, age, rating, balance, image) VALUES (?, ?, ?, ?, ?, ?);");
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        // bind id
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_BIND);
        dbuf_write_int32(req, 1); // iparam
        dbuf_write_byte(req, VAL_INT);
        dbuf_write_int32(req, id);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        // bind name
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_BIND);
        dbuf_write_int32(req, 2); // iparam
        dbuf_write_byte(req, VAL_TEXT);
        dbuf_write_string(req, name);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        // bind age
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_BIND);
        dbuf_write_int32(req, 3); // iparam
        dbuf_write_byte(req, VAL_INT64);
        dbuf_write_int64(req, age);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        // bind rating
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_BIND);
        dbuf_write_int32(req, 4); // iparam
        dbuf_write_byte(req, VAL_DOUBLE_STR);
        dbuf_write_double_str(req, rating);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        // bind balance
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_BIND);
        dbuf_write_int32(req, 5); // iparam
        dbuf_write_byte(req, VAL_DOUBLE_IEEE);
        dbuf_write_double_ieee(req, balance);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        // bind image
        for (int i = 0; i < (sizeof(image) / sizeof(image[0])); i++) {
            image[i] = (byte)i;
        }
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_BIND);
        dbuf_write_int32(req, 6);
        dbuf_write_byte(req, VAL_BLOB);
        dbuf_write_blob(req, image, sizeof(image)); // image blob
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        // step
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_STEP);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        bool more = dbuf_read_bool(resp);
        ASSERT(!more, "expected !more but was %d", more);
        // reset
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_RESET);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        // changes
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_CHANGES);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        int changes = dbuf_read_int32(resp);
        ASSERT(changes==1, "wrong changes %d", changes);
        // finalize
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_FINALIZE);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        // commit
        _prep_step_fin(hd, req, resp, "COMMIT");
    }
    // select user
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_PREPARE);
        dbuf_write_string(req, "SELECT id, name, age, rating, balance, image FROM users ORDER BY id;");
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_STEP);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        bool more = dbuf_read_bool(resp);
        ASSERT(more, "expected more but was %d\n", more);
        // fetch id
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_COLUMN);
        dbuf_write_int32(req, 0);
        dbuf_write_byte(req, VAL_INT);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        ASSERT0(dbuf_read_bool(resp), "id not set");
        int sel_id = dbuf_read_int32(resp);
        ASSERT(sel_id==id, "wrong sel_id %d", sel_id);
        // fetch name
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_COLUMN);
        dbuf_write_int32(req, 1);
        dbuf_write_byte(req, VAL_TEXT);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        ASSERT0(dbuf_read_bool(resp), "was not set");
        const char *sel_name = dbuf_read_string(resp);
        ASSERT(strcmp(sel_name, name)==0, "wrong sel_name '%s'", sel_name);
        // fetch age
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_COLUMN);
        dbuf_write_int32(req, 2);
        dbuf_write_byte(req, VAL_INT64);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        ASSERT0(dbuf_read_bool(resp), "was not set");
        int64 sel_age = dbuf_read_int64(resp);
        ASSERT(sel_age==age, "wrong sel_age %I64d", sel_age);
        // fetch rating
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_COLUMN);
        dbuf_write_int32(req, 3);
        dbuf_write_byte(req, VAL_DOUBLE_STR);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        ASSERT0(dbuf_read_bool(resp), "was not set");
        double sel_rating = dbuf_read_double_str(resp);
        ASSERT(sel_rating == rating, "wrong sel_rating %g", sel_rating);
        // fetch balance
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_COLUMN);
        dbuf_write_int32(req, 4);
        dbuf_write_byte(req, VAL_DOUBLE_IEEE);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        ASSERT0(dbuf_read_bool(resp), "was not set");
        double sel_balance = dbuf_read_double_ieee(resp);
        ASSERT(sel_balance == balance, "wrong sel_balance %g", sel_balance);
        // fetch image
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_COLUMN);
        dbuf_write_int32(req, 5);
        dbuf_write_byte(req, VAL_BLOB);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        ASSERT0(dbuf_read_bool(resp), "was not set");
        int blob_size;
        const byte *sel_image = dbuf_read_blob(resp, &blob_size);
        ASSERT(sel_image != NULL, "wrong sel_image %p", sel_image);
        ASSERT(blob_size == sizeof(image), "wrong blob_size %d", blob_size);
        for (int i=0 ; i<blob_size ; i++) {
            ASSERT(sel_image[i] == image[i], "wrong sel_image[%d] %d", i, sel_image[i]);
        }
        // step must have no more
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_STEP);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        ASSERT(!dbuf_read_bool(resp), "want !more but was %d", TRUE);
        // finalize
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_FINALIZE);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
    }
    // close db
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_CLOSE);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
    }
    // cleanup
    dbuf_free(req);
    dbuf_free(resp);
    handler_free(hd);
    INFO("TEST %s OK\n", __func__);
}

void test_handler_exec_query(const char *dbfile) {
    INFO("TEST %s\n", __func__);
    DEBUG("  dbfile=%s\n", dbfile);
    handler *hd = handler_new();
    dbuf *req = dbuf_new();
    dbuf *resp = dbuf_new();
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_OPEN);
        dbuf_write_string(req, dbfile);
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
    }
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_EXEC);
        dbuf_write_string(req, "DROP TABLE IF EXISTS users");
        dbuf_write_int32(req, 1); // niterations
        dbuf_write_int32(req, 0); // nparams per iteration
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
        int changes = dbuf_read_int32(resp);
        ASSERT(changes==0, "expected 0 changes but was %d", changes);
    }
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_EXEC);
        dbuf_write_string(req, "CREATE TABLE users (id INTEGER PRIMARY KEY NOT NULL, name VARCHAR, age INTEGER, rating REAL)");
        dbuf_write_int32(req, 1); // niterations
        dbuf_write_int32(req, 0); // nparams per iteration
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
        int changes = dbuf_read_int32(resp);
        ASSERT(changes==0, "expected 0 changes but was %d", changes);
    }
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_EXEC);
        dbuf_write_string(req, "BEGIN");
        dbuf_write_int32(req, 1); // niterations
        dbuf_write_int32(req, 0); // nparams per iteration
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
        int changes = dbuf_read_int32(resp);
        ASSERT(changes==0, "expected 0 changes but was %d", changes);
    }
    int nusers = 2;
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_EXEC);
        dbuf_write_string(req, "INSERT INTO users (id,name,age,rating) VALUES (?,?,?,?)");
        dbuf_write_int32(req, nusers); // niterations
        dbuf_write_int32(req, 4); // nparams per iteration
        for (int i=0 ; i<nusers ; i++) {
            int id = i+1;
            char name[32];
            sprintf(name, "User_%d", id);
            int age = 32+i;
            double rating = 0.12 * (double)(i+1);
            dbuf_write_byte(req, VAL_INT); // col_type
            dbuf_write_int32(req, id); // val
            dbuf_write_byte(req, VAL_TEXT); // col_type
            dbuf_write_string(req, name); // val
            dbuf_write_byte(req, VAL_INT); // col_type
            dbuf_write_int32(req, age); // val
            dbuf_write_byte(req, VAL_DOUBLE_STR); // col_type
            dbuf_write_double_str(req, rating); // val
        }
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
        for (int i=0 ; i<nusers ; i++) {
            int changes = dbuf_read_int32(resp);
            ASSERT(changes==1, "for i=%d: expected 1 change but was %d", i, changes);
        }
    }
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_EXEC);
        dbuf_write_string(req, "COMMIT");
        dbuf_write_int32(req, 1);
        dbuf_write_int32(req, 0);
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
    }
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_QUERY);
        dbuf_write_string(req, "SELECT id,name,age,rating FROM users WHERE id>=? AND name LIKE ? ORDER BY id");
        dbuf_write_int32(req, 2); // nparams
        dbuf_write_byte(req, VAL_INT); // param 1 type
        dbuf_write_int32(req, 1); // param 1 value
        dbuf_write_byte(req, VAL_TEXT); // param 2 type
        dbuf_write_string(req, "%"); // param 2 value
        dbuf_write_int32(req, 4); // ncols
        dbuf_write_byte(req, VAL_INT); // col 0 type
        dbuf_write_byte(req, VAL_TEXT); // col 1 type
        dbuf_write_byte(req, VAL_INT); // col 2 type
        dbuf_write_byte(req, VAL_DOUBLE_STR); // col 3 type
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
        int nrows = dbuf_read_int32(resp);
        ASSERT(nrows == nusers, "want %d rows, have %d\n", nusers, nrows);
        for (int i=0 ; i<nrows ; i++) {
            // id
            bool set = dbuf_read_bool(resp);
            ASSERT(set, "expected id set but was %d\n", set);
            int id = dbuf_read_int32(resp);
            if (i == 0) {
                ASSERT(id==1, "expected id 1 but was %d\n", id);
            } else if (i == 1) {
                ASSERT(id==2, "expected id 2 but was %d\n", id);
            }
            // name
            set = dbuf_read_bool(resp);
            ASSERT(set, "expected name set but was %d\n", set);
            const char *name = dbuf_read_string(resp);
            if (i == 0) {
                ASSERT(strcmp(name, "User_1") == 0, "expected name User_1 but was '%s'\n", name);
            } else if (i == 1) {
                ASSERT(strcmp(name, "User_2") == 0, "expected name User_2 but was '%s'\n", name);
            }
            // age
            set = dbuf_read_bool(resp);
            ASSERT(set, "expected age set but was %d\n", set);
            int age = dbuf_read_int32(resp);
            if (i == 0) {
                ASSERT(age == 32, "expected age 31 but was %d\n", age);
            } else if (i == 1) {
                ASSERT(age == 33, "expected age 33 but was %d\n", age);
            }
            // rating
            set = dbuf_read_bool(resp);
            ASSERT(set, "expected rating set but was %d\n", set);
            double rating = dbuf_read_double_str(resp);
            if (i == 0) {
                ASSERT(rating == 0.12, "expected rating == 0.12 but was %g\n", rating);
            } else if (i == 1) {
                ASSERT(rating == 0.24, "expected rating == 0.24 but was %g\n", rating);
            }
        }
    }
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_CLOSE);
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
    }
    dbuf_free(req);
    dbuf_free(resp);
    handler_free(hd);
    INFO("TEST %s OK\n", __func__);
}

void test_handler_exec_failure(const char *dbfile) {
    INFO("TEST %s\n", __func__);
    DEBUG("  dbfile=%s\n", dbfile);
    handler *hd = handler_new();
    dbuf *req = dbuf_new();
    dbuf *resp = dbuf_new();
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_OPEN);
        dbuf_write_string(req, dbfile);
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
    }
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_EXEC);
        dbuf_write_string(req, "DROP TABLE IF EXISTS users");
        dbuf_write_int32(req, 1); // niterations
        dbuf_write_int32(req, 0); // nparams per iteration
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
        int changes = dbuf_read_int32(resp);
        ASSERT(changes==0, "expected 0 changes but was %d", changes);
    }
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_EXEC);
        dbuf_write_string(req, "CREATE TABLE users (id INTEGER NOT NULL)");
        dbuf_write_int32(req, 1); // niterations
        dbuf_write_int32(req, 0); // nparams per iteration
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
        int changes = dbuf_read_int32(resp);
        ASSERT(changes==0, "expected 0 changes but was %d", changes);
    }
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_EXEC);
        dbuf_write_string(req, "INSERT INTO users (id) VALUES (?)");
        dbuf_write_int32(req, 1); // niterations
        dbuf_write_int32(req, 1); // nparams per iteration
        dbuf_write_byte(req, VAL_NULL); // col_type
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(!ok, "expected !ok but ok was %d", ok);
        const char *errmsg = dbuf_read_string(resp);
        ASSERT(strcmp(errmsg, "sqlite3_step: err=19, msg=NOT NULL constraint failed: users.id")==0, "wrong errmsg '%s'", errmsg);
    }
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_EXEC);
        dbuf_write_string(req, "INSERT INTO users (id) VALUES (?)");
        dbuf_write_int32(req, 1); // niterations
        dbuf_write_int32(req, 1); // nparams per iteration
        dbuf_write_byte(req, VAL_INT); // col_type
        dbuf_write_int32(req, 42); // id=42
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
    }
    {
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_CLOSE);
        handler_handle(hd, req, resp);
        bool ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
    }
    dbuf_free(req);
    dbuf_free(resp);
    handler_free(hd);
    INFO("TEST %s OK\n", __func__);
}

void test_handler_errors() {
    INFO("TEST %s\n", __func__);
    handler *hd = handler_new();
    dbuf *req = dbuf_new();
    dbuf *resp = dbuf_new();
    bool ok;
    const char *errmsg;
    {
        // close before open is ok
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_CLOSE);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);        
        ASSERT(ok, "expected ok but ok was %d", ok);
        // prepare before open is err
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_PREPARE);
        dbuf_write_string(req, "SELECT 1");
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(!ok, "expected !ok but ok was %d", ok);
        errmsg = dbuf_read_string(resp);
        ASSERT(strcmp(errmsg, "sqlite3_prepare_v2: err=21, msg=out of memory")==0, "wrong errmsg '%s'", errmsg);
    }
    {
        // open mem db
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_OPEN);
        dbuf_write_string(req, ":memory:");
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but ok was %d", ok);
        // double open is err
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_OPEN);
        dbuf_write_string(req, ":memory:");
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(!ok, "expected !ok but ok was %d", ok);
        errmsg = dbuf_read_string(resp);
        ASSERT(strcmp(errmsg, "cannot open, db is already open")==0, "wrong errmsg '%s'", errmsg);
        // prepare invalid sql is err
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_PREPARE);
        dbuf_write_string(req, "SELECT something FROM anything WHERE AND SUBST;");
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(!ok, "expected !ok but ok was %d", ok);
        errmsg = dbuf_read_string(resp);
        ASSERT(strcmp(errmsg, "sqlite3_prepare_v2: err=1, msg=near \"AND\": syntax error")==0, "wrong errmsg '%s'", errmsg);
        // finalize is ok
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_FINALIZE);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but ok was %d", ok);
        // prepare is ok
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_PREPARE);
        dbuf_write_string(req, "SELECT 1 WHERE 2 < ?;");
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but ok was %d", ok);
        // bind iparam -1/0/2 is err
        int iparams[] = {-1,0,2};
        for (int i=0 ; i<sizeof(iparams)/sizeof(iparams[0]) ; i++) {
            int iparam = iparams[i];
            dbuf_reset(req);
            dbuf_reset(resp);
            dbuf_write_byte(req, FC_BIND);
            dbuf_write_int32(req, iparam);
            dbuf_write_byte(req, VAL_INT);
            dbuf_write_int32(req, 42);
            handler_handle(hd, req, resp);
            ok = dbuf_read_bool(resp);
            ASSERT(!ok, "iparam %d: expected !ok but was %d", iparam, ok);
            errmsg = dbuf_read_string(resp);
            ASSERT(strcmp(errmsg, "sqlite3_bind_int: err=25, msg=column index out of range")==0, "iparam %d: wrong errmsg '%s'", iparam, errmsg);
        }
        // bind iparam 1 is ok
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_BIND);
        dbuf_write_int32(req, 1);
        dbuf_write_byte(req, VAL_INT);
        dbuf_write_int32(req, 42);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        // column before step is ok (but always NULL)
        int icols[] = {-1,0,1,2};
        for (int i=0 ; i<sizeof(icols)/sizeof(icols[0]) ; i++) {
            dbuf_reset(req);
            dbuf_reset(resp);
            dbuf_write_byte(req, FC_COLUMN);
            dbuf_write_int32(req, icols[i]);
            dbuf_write_byte(req, VAL_INT);
            handler_handle(hd, req, resp);
            ok = dbuf_read_bool(resp);
            ASSERT(ok, "expected ok but was %d", ok);
            bool set = dbuf_read_bool(resp);
            ASSERT(!set, "expected !set but was %d", set);
        }
        // step is ok
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_STEP);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        bool more = dbuf_read_bool(resp);
        ASSERT(more, "wrong more %d", more);
        // column is ok
        for (int i=0 ; i<sizeof(icols)/sizeof(icols[0]) ; i++) {
            int icol = icols[i];
            dbuf_reset(req);
            dbuf_reset(resp);
            dbuf_write_byte(req, FC_COLUMN);
            dbuf_write_int32(req, icol);
            dbuf_write_byte(req, VAL_INT);
            handler_handle(hd, req, resp);
            ok = dbuf_read_bool(resp);
            ASSERT(ok, "icol %d: wrong ok %d", icol, ok);
            bool set = dbuf_read_bool(resp);
            if (icol==0) {
                ASSERT(set, "icol %d: wrong set %d", icol, set);
                int i32 = dbuf_read_int32(resp);
                ASSERT(i32==1, "icol %d: wrong i32 %d", icol, i32);
            } else {
                ASSERT(!set, "icol %d: wrong set %d", icol, set);
            }
        }
        // step is ok
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_STEP);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(ok, "expected ok but was %d", ok);
        more = dbuf_read_bool(resp);
        ASSERT(!more, "wrong more %d", more);
        // close with active stmt is err
        dbuf_reset(req);
        dbuf_reset(resp);
        dbuf_write_byte(req, FC_CLOSE);
        handler_handle(hd, req, resp);
        ok = dbuf_read_bool(resp);
        ASSERT(!ok, "expected !ok but was %d", ok);
        errmsg = dbuf_read_string(resp);
        ASSERT(strcmp(errmsg, "sqlite3_close: err=5, msg=unable to close due to unfinalized statements or unfinished backups")==0, "wrong errmsg '%s'", errmsg);
    }
    dbuf_free(req);
    dbuf_free(resp);
    handler_free(hd);
    INFO("TEST %s OK\n", __func__);
}
