#include "utl.h"
#include "io.h"
#include "db.h"
#include "app.h"

// function codes

#define FC_EXEC 1
#define FC_QUERY 2
#define FC_QUIT 9

// class App

struct app_s {
    Db *db;
    Reader *r;
    Writer *w;
};

App *newApp(Db *db, Reader *r, Writer *w) {
    ASSERT(db);
    ASSERT(r);
    ASSERT(w);
    App *this = (App *)memAlloc(sizeof(App), __FILE__, __LINE__);
    this->db = db;
    this->r = r;
    this->w = w;
    return this;
}

void App_free(App *this) {
    ASSERT(this);
    memFree(this);
}

static void _fcExec(App *this) {
    const char *sql = Reader_readString(this->r);
    ASSERT(sql);
    BOOL ok = Db_prepare(this->db, sql);
    int niterations = Reader_readInt32(this->r);
    int nparams = Reader_readInt32(this->r);
    Value *params = (Value *)memAlloc(nparams * sizeof(Value), __FILE__, __LINE__);
    for (int i = 0; i < niterations; i++) {
        for (int iparam = 0; iparam < nparams; iparam++) {
            params[iparam].type = Reader_readByte(this->r);
            switch (params[iparam].type) {
                case VT_NULL:
                    // NULL has no further value
                    break;
                case VT_INT32:
                    params[iparam].i32 = Reader_readInt32(this->r);
                    break;
                case VT_INT64:
                    params[iparam].i64 = Reader_readInt64(this->r);
                    break;
                case VT_DOUBLE:
                    params[iparam].d = Reader_readDouble(this->r);
                    break;
                case VT_STRING:
                    params[iparam].p = Reader_readString(this->r);  // TODO optimize: get string length and pass that to bind function
                    break;
                case VT_BLOB:
                    params[iparam].p = Reader_readBlob(this->r, &(params[iparam].sz));
                    break;
                default:
                    ASSERT_FAIL("_fcExec: invalid params[%d].type = %d", iparam, params[iparam].type);
            }
        }
        if (ok) {
            ok = Db_bind_step_reset(this->db, params, nparams);
        }
    }
    memFree(params);
    Writer_writeByte(this->w, ok);
    if (!ok) {
        Writer_writeString(this->w, Db_errmsg(this->db));
    }
    Db_finalize(this->db);
}

static void _fcQuery(App *this) {
    const char *sql = Reader_readString(this->r);
    BOOL ok = Db_prepare(this->db, sql);
    // read and bind parameters
    {
        int nparams = Reader_readInt32(this->r);
        Value *params = (Value *)memAlloc(nparams * sizeof(Value), __FILE__, __LINE__);
        for (int iparam = 0; iparam < nparams; iparam++) {
            params[iparam].type = Reader_readByte(this->r);
            switch (params[iparam].type) {
                case VT_NULL:
                    // NULL has no further value
                    break;
                case VT_INT32:
                    params[iparam].i32 = Reader_readInt32(this->r);
                    break;
                case VT_INT64:
                    params[iparam].i64 = Reader_readInt64(this->r);
                    break;
                case VT_DOUBLE:
                    params[iparam].d = Reader_readDouble(this->r);
                    break;
                case VT_STRING:
                    params[iparam].p = Reader_readString(this->r);  // TODO optimize get string length and pass that to bind function
                    break;
                case VT_BLOB:
                    params[iparam].p = Reader_readBlob(this->r, &params[iparam].sz);
                    break;
                default:
                    ASSERT_FAIL("_fcQuery: invalid params[%d].type = %d", iparam, params[iparam].type);
            }
        }
        if (ok) {
            ok = Db_bind(this->db, params, nparams);
        }
        memFree(params);
    }
    // fetch column values
    {
        // read column types
        int ncols = Reader_readInt32(this->r);
        char *coltypes = (char *)memAlloc(ncols, __FILE__, __LINE__);
        for (int icol = 0; icol < ncols; icol++) {
            coltypes[icol] = Reader_readByte(this->r);
        }
        Value *values = (Value *)memAlloc(ncols * sizeof(Value), __FILE__, __LINE__);
        // fetch all rows
        BOOL hasRow = TRUE;
        while (ok && hasRow) {
            for (int icol = 0; icol < ncols; icol++) {
                values[icol].type = coltypes[icol];
            }
            ok = Db_step_fetch(this->db, &hasRow, values, ncols);
            if (ok && hasRow) {
                Writer_writeByte(this->w, 1);  // hasRow = TRUE
                for (int icol = 0; icol < ncols; icol++) {
                    Value val = values[icol];
                    Writer_writeByte(this->w, val.type);
                    switch (val.type) {
                        case VT_NULL:
                            // no furhter data
                            break;
                        case VT_INT32:
                            Writer_writeInt32(this->w, val.i32);
                            break;
                        case VT_INT64:
                            Writer_writeInt64(this->w, val.i64);
                            break;
                        case VT_DOUBLE:
                            Writer_writeDouble(this->w, val.d);
                            break;
                        case VT_STRING:
                            Writer_writeString(this->w, val.p);
                            break;
                        case VT_BLOB:
                            Writer_writeBlob(this->w, val.p, val.sz);
                            break;
                        default:
                            ASSERT_FAIL("_fcQuery: unknown values[%d].type %d", icol, val.type);
                    }
                }
                Writer_markFrame(this->w);
            }
        }  // end while
        Writer_writeByte(this->w, 0);  // hasRow = FALSE
        memFree(values);
        memFree(coltypes);
    }
    Writer_writeByte(this->w, ok);
    if (!ok) {
        Writer_writeString(this->w, Db_errmsg(this->db));
    }
    Db_finalize(this->db);
}

static void _fcQuit(App *this) {
    Writer_writeByte(this->w, TRUE);  // ok
}

BOOL App_step(App *this) {
    ASSERT(this);
    LOG_DEBUG0("App_step: await request");
    char fc = Reader_readByte(this->r);
    BOOL next = TRUE;
    switch (fc) {
        case FC_EXEC:
            LOG_DEBUG0("App_step: FC_EXEC");
            _fcExec(this);
            break;
        case FC_QUERY:
            LOG_DEBUG0("App_step: FC_QUERY");
            _fcQuery(this);
            break;
        case FC_QUIT:
            LOG_DEBUG0("App_step: FC_QUIT");
            _fcQuit(this);
            next = FALSE;
            break;
        default:
            ASSERT_FAIL("App_step: unknown function code %d", fc);
            break;
    }
    Writer_flush(this->w);
    return next;
}

// TEST

static void testBasic() {
    // setup
    Db *db = newDb(":memory:", FALSE);
    char buf[1099];
    Reader *r = newMemReader(buf, sizeof(buf));
    Writer *w = newMemWriter(buf, sizeof(buf));
    App *app = newApp(db, r, w);
    {
        Writer_writeByte(w, FC_EXEC);
        Writer_writeString(w, "CREATE TABLE users(id INTEGER PRIMARY KEY NOT NULL)");
        Writer_writeInt32(w, 1);  // 1 iteration
        Writer_writeInt32(w, 0);  // 0 params per iteration
        //
        BOOL next = App_step(app);
        ASSERT(next);
        //
        ASSERT_INT(1, Reader_readByte(r));              // ok
    }
    {
        Writer_writeByte(w, FC_EXEC);
        Writer_writeString(w, "_this_must_fail_dfidafiaodfiuoadhfoahdf");
        Writer_writeInt32(w, 1);  // 1 iteration
        Writer_writeInt32(w, 0);  // 0 params per iteration
        //
        BOOL next = App_step(app);
        ASSERT(next);
        //
        ASSERT_INT(0, Reader_readByte(r));     // not ok
        const char *p = Reader_readString(r);  // errmsg
        ASSERT_STR("near \"_this_must_fail_dfidafiaodfiuoadhfoahdf\": syntax error", p);
    }
    {
        Writer_writeByte(w, FC_EXEC);
        Writer_writeString(w, "INSERT INTO users(id) VALUES (?)");
        Writer_writeInt32(w, 2);        // 2 iterations
        Writer_writeInt32(w, 1);        // 1 param per iteration
        Writer_writeByte(w, VT_INT64);  // params[0].type
        Writer_writeInt64(w, 1);        // params[0].value
        Writer_writeByte(w, VT_INT64);  // params[1].type
        Writer_writeInt64(w, 2);        // params[1].value
        //
        BOOL next = App_step(app);
        ASSERT(next);
        // -> must be ok
        ASSERT(Reader_readByte(r));                     // ok
    }
    {
        Writer_writeByte(w, FC_QUERY);
        Writer_writeString(w, "SELECT id FROM users WHERE id>? ORDER BY id");
        Writer_writeInt32(w, 1);        // 1 param
        Writer_writeByte(w, VT_INT64);  //     param 0 type
        Writer_writeInt64(w, 0);        //     param 0 value
        Writer_writeInt32(w, 1);        // 1 column
        Writer_writeByte(w, VT_INT64);  //     column 0 type
        ASSERT(App_step(app));
        // -> must have result rows
        ASSERT(Reader_readByte(r));  // has row
        {
            ASSERT(Reader_readByte(r));            // has value
            ASSERT_INT64(1, Reader_readInt64(r));  // int value
        }
        ASSERT(Reader_readByte(r));  // has row
        {
            ASSERT(Reader_readByte(r));            // has value
            ASSERT_INT64(2, Reader_readInt64(r));  // int value
        }
        ASSERT(!Reader_readByte(r));                    // no more row
        ASSERT(Reader_readByte(r));                     // ok
    }
    {
        Writer_writeByte(w, FC_QUIT);
        //
        BOOL next = App_step(app);
        ASSERT(!next);
        // FC_QUIT must be ok
        ASSERT_INT(1, Reader_readByte(r));              // ok
    }
    // free
    App_free(app);
    Writer_free(w);
    Reader_free(r);
    Db_free(db);
}

static void testValueTypesAndErrors() {
    // setup
    Db *db = newDb(":memory:", FALSE);
    char buf[4 * 1024];
    memset(buf, 255, sizeof(buf));
    Reader *r = newMemReader(buf, sizeof(buf));
    Writer *w = newMemWriter(buf, sizeof(buf));
    App *app = newApp(db, r, w);
    {
        Writer_writeByte(w, FC_EXEC);
        Writer_writeString(w, "CREATE TABLE users(i INTEGER, d FLOAT, s TEXT, b BLOB, UNIQUE(i))");
        Writer_writeInt32(w, 1);  // 1 iteration
        Writer_writeInt32(w, 0);  // 0 params per iteration
        //
        ASSERT(App_step(app));
        //
        ASSERT_INT(1, Reader_readByte(r)); // ok
    }
    {
        Writer_writeByte(w, FC_EXEC);
        Writer_writeString(w, "INSERT INTO users(i,d,s,b) VALUES (?,?,?,?)");
        Writer_writeInt32(w, 2);          // 2 iterations
        Writer_writeInt32(w, 4);          // 4 params per iteration
        Writer_writeByte(w, VT_INT64);    // iter 0 param 0 type
        Writer_writeInt64(w, 1);          // iter 0 param 0 value
        Writer_writeByte(w, VT_DOUBLE);   // iter 0 param 1 type
        Writer_writeDouble(w, 1.1);       // iter 0 param 1 value
        Writer_writeByte(w, VT_STRING);   // iter 0 param 2 type
        Writer_writeString(w, "Alice");   // iter 0 param 2 value
        Writer_writeByte(w, VT_BLOB);     // iter 0 param 3 type
        Writer_writeBlob(w, "Alice", 6);  // iter 0 param 3 value
        Writer_writeByte(w, VT_INT64);    // iter 1 param 0 type
        Writer_writeInt64(w, 2);          // iter 1 param 0 value
        Writer_writeByte(w, VT_DOUBLE);   // iter 1 param 1 type
        Writer_writeDouble(w, 2.2);       // iter 1 param 1 value
        Writer_writeByte(w, VT_STRING);   // iter 1 param 2 type
        Writer_writeString(w, "Bob");     // iter 1 param 2 value
        Writer_writeByte(w, VT_BLOB);     // iter 1 param 3 type
        Writer_writeBlob(w, "Bob", 4);    // iter 1 param 3 value
        //
        ASSERT(App_step(app));
        //
        ASSERT_INT(1, Reader_readByte(r));              // ok
    }
    {
        Writer_writeByte(w, FC_QUERY);
        Writer_writeString(w, "SELECT i FROM users WHERE i>? ORDER BY i");
        Writer_writeInt32(w, 1);        // 1 param
        Writer_writeByte(w, VT_INT64);  // params[0].type
        Writer_writeInt64(w, 0);        // params[0].value
        Writer_writeInt32(w, 1);        // 1 column
        Writer_writeByte(w, VT_INT64);  // columns[0].type
        //
        ASSERT(App_step(app));
        //
        ASSERT_INT(1, Reader_readByte(r));  // has row
        {
            ASSERT_INT(VT_INT64, Reader_readByte(r));   // value type
            ASSERT_INT(1, Reader_readInt64(r));  // value
        }
        ASSERT_INT(1, Reader_readByte(r));  // has row
        {
            ASSERT_INT(VT_INT64, Reader_readByte(r));   // value type
            ASSERT_INT(2, Reader_readInt64(r));  // value
        }
        ASSERT_INT(0, Reader_readByte(r));              // no more rows
        ASSERT_INT(1, Reader_readByte(r));              // ok
    }
    {
        Writer_writeByte(w, FC_QUERY);
        Writer_writeString(w, "SELECT i,d,s,b FROM users WHERE i>? ORDER BY i");
        Writer_writeInt32(w, 1);         // 1 param
        Writer_writeByte(w, VT_INT64);   //   params[0].type
        Writer_writeInt64(w, 0);         //   params[0].value
        Writer_writeInt32(w, 4);         // 4 columns
        Writer_writeByte(w, VT_INT64);   //   columns[0].type
        Writer_writeByte(w, VT_DOUBLE);  //   columns[1].type
        Writer_writeByte(w, VT_STRING);  //   columns[2].type
        Writer_writeByte(w, VT_BLOB);    //   columns[3].type
        //
        ASSERT(App_step(app));
        //
        ASSERT(Reader_readByte(r));  // has row
        {
            ASSERT(Reader_readByte(r));                // has value
            ASSERT_INT64(1, Reader_readInt64(r));      // value
            ASSERT(Reader_readByte(r));                // has value
            ASSERT_DOUBLE(1.1, Reader_readDouble(r));  // double 1.1
            ASSERT(Reader_readByte(r));                // has value
            const char *str = Reader_readString(r);
            ASSERT_STR("Alice", str);
            size_t len;
            ASSERT(Reader_readByte(r));  // has value
            const char *p = Reader_readBlob(r, &len);
            ASSERT_INT(6, len);
            ASSERT(!memcmp(p, "Alice", 6));
        }
        ASSERT(Reader_readByte(r));  // has row
        {
            ASSERT(Reader_readByte(r));  // has value
            ASSERT_INT64(2, Reader_readInt64(r));
            ASSERT(Reader_readByte(r));             // has value
            ASSERT_DOUBLE(2.2, Reader_readDouble(r));  // double 2.2
            ASSERT(Reader_readByte(r));             // has value
            const char *str = Reader_readString(r);
            ASSERT_STR("Bob", str);
            size_t len;
            ASSERT(Reader_readByte(r));  // has value
            const char *p = Reader_readBlob(r, &len);
            ASSERT_INT(4, len);
            ASSERT(!memcmp(p, "Bob", 4));
        }
        ASSERT(!Reader_readByte(r));                    // no more rows
        ASSERT(Reader_readByte(r));                     // ok
    }
    {
        // insert NULL values
        Writer_writeByte(w, FC_EXEC);
        Writer_writeString(w, "INSERT INTO users(i,d,s,b) VALUES (?,?,?,?)");
        Writer_writeInt32(w, 1);       // 1 iteration
        Writer_writeInt32(w, 4);       // 4 params per iteration
        Writer_writeByte(w, VT_NULL);  // iter0 param0 type
        Writer_writeByte(w, VT_NULL);  //       param1 type
        Writer_writeByte(w, VT_NULL);  //       param2 type
        Writer_writeByte(w, VT_NULL);  //       param3 type
        //
        ASSERT(App_step(app));
        //
        ASSERT(Reader_readByte(r));                     // ok
    }
    {
        // select users with NULLs
        Writer_writeByte(w, FC_QUERY);
        Writer_writeString(w, "SELECT i,d,s,b FROM users ORDER BY i");
        Writer_writeInt32(w, 0);         // 0 params
        Writer_writeInt32(w, 4);         // 4 columns
        Writer_writeByte(w, VT_INT64);   // columns[0].type
        Writer_writeByte(w, VT_DOUBLE);  // columns[1].type
        Writer_writeByte(w, VT_STRING);  // columns[2].type
        Writer_writeByte(w, VT_BLOB);    // columns[3].type
        //
        ASSERT(App_step(app));
        //
        ASSERT(Reader_readByte(r));  // has row
        {
            ASSERT(!Reader_readByte(r));  // no value
            ASSERT(!Reader_readByte(r));  // no value
            ASSERT(!Reader_readByte(r));  // no value
            ASSERT(!Reader_readByte(r));  // no value
        }
        ASSERT_INT(1, Reader_readByte(r));  // has row
        {
            ASSERT(Reader_readByte(r));                // has value
            ASSERT_INT(1, Reader_readInt64(r));        // value
            ASSERT(Reader_readByte(r));                // has value
            ASSERT_DOUBLE(1.1, Reader_readDouble(r));  // double 1.1
            ASSERT(Reader_readByte(r));                // has value
            const char *str = Reader_readString(r);
            ASSERT_STR("Alice", str);
            size_t sz;
            ASSERT(Reader_readByte(r));  // has value
            const char *p = Reader_readBlob(r, &sz);
            ASSERT_INT(6, sz);
            ASSERT(memcmp(p, "Alice", 6) == 0);
        }
        ASSERT_INT(1, Reader_readByte(r));  // has row
        {
            ASSERT(Reader_readByte(r));  // has value
            ASSERT_INT(2, Reader_readInt64(r));
            ASSERT(Reader_readByte(r));                // has value
            ASSERT_DOUBLE(2.2, Reader_readDouble(r));  // double 2.2
            ASSERT(Reader_readByte(r));                // has value
            const char *str = Reader_readString(r);
            ASSERT_STR("Bob", str);
            size_t sz;
            ASSERT(Reader_readByte(r));  // has value
            const char *p = Reader_readBlob(r, &sz);
            ASSERT_INT(4, sz);
            ASSERT(memcmp(p, "Bob", 4) == 0);
        }
        ASSERT(!Reader_readByte(r));                    // no more rows
        ASSERT(Reader_readByte(r));                     // ok
    }
    {
        // select with NULL params
        Writer_writeByte(w, FC_QUERY);
        Writer_writeString(w, "SELECT i,d,s,b FROM users WHERE i IS NULL");
        Writer_writeInt32(w, 0);         // no params
        Writer_writeInt32(w, 4);         // 4 columns
        Writer_writeByte(w, VT_INT64);   // columns[0].type
        Writer_writeByte(w, VT_DOUBLE);  // columns[1].type
        Writer_writeByte(w, VT_STRING);  // columns[2].type
        Writer_writeByte(w, VT_BLOB);    // columns[3].type
        //
        ASSERT(App_step(app));
        //
        ASSERT_INT(1, Reader_readByte(r));              // has row
        ASSERT(!Reader_readByte(r));                    //   no value
        ASSERT(!Reader_readByte(r));                    //   no value
        ASSERT(!Reader_readByte(r));                    //   no value
        ASSERT(!Reader_readByte(r));                    //   no value
        ASSERT(!Reader_readByte(r));                    // no more rows
        ASSERT(Reader_readByte(r));                     // ok
    }
    {
        // SELECT WHERE i = 2345 must select no rows
        Writer_writeByte(w, FC_QUERY);
        Writer_writeString(w, "SELECT i,d,s,b FROM users WHERE i =?");
        Writer_writeInt32(w, 1);         // 1 param
        Writer_writeByte(w, VT_INT64);   //     param 0 type
        Writer_writeInt64(w, 2345);      //     param 0 value
        Writer_writeInt32(w, 4);         // 4 columns
        Writer_writeByte(w, VT_INT64);   //     column 0 type
        Writer_writeByte(w, VT_DOUBLE);  //     column 1 type
        Writer_writeByte(w, VT_STRING);  //     column 2 type
        Writer_writeByte(w, VT_BLOB);    //     column 3 type
        //
        ASSERT(App_step(app));
        //
        ASSERT(!Reader_readByte(r));                    // no more rows
        ASSERT(Reader_readByte(r));                     // ok
    }
    {
        // SELECT WHERE s = "Alice" must select one row
        Writer_writeByte(w, FC_QUERY);
        Writer_writeString(w, "SELECT i,d,s,b FROM users WHERE s = ?");
        Writer_writeInt32(w, 1);         // 1 param
        Writer_writeByte(w, VT_STRING);  //     param 0 type
        Writer_writeString(w, "Alice");  //     param 0 value
        Writer_writeInt32(w, 4);         // 4 columns
        Writer_writeByte(w, VT_INT64);   //     column 0 type
        Writer_writeByte(w, VT_DOUBLE);  //     column 1 type
        Writer_writeByte(w, VT_STRING);  //     column 2 type
        Writer_writeByte(w, VT_BLOB);    //     column 3 type
        //
        ASSERT(App_step(app));
        //
        ASSERT_INT(1, Reader_readByte(r));         // has row
        ASSERT(Reader_readByte(r));                //   has value 0
        ASSERT_INT64(1, Reader_readInt64(r));      //   value 0
        ASSERT(Reader_readByte(r));                //   has value 1
        ASSERT_DOUBLE(1.1, Reader_readDouble(r));  //   value 1
        ASSERT(Reader_readByte(r));                //   has value 2
        const char *str = Reader_readString(r);    //   value 2
        ASSERT_STR("Alice", str);
        ASSERT(Reader_readByte(r));  //   has value 3
        size_t len;
        const char *p = Reader_readBlob(r, &len);  //   value 3
        ASSERT_INT(6, len);
        ASSERT_INT(0, memcmp(p, "Alice", 6));
        ASSERT(!Reader_readByte(r));                    // no more rows
        ASSERT(Reader_readByte(r));                     // ok
    }
    {
        // SELECT WHERE b = "Bob" must select one row
        Writer_writeByte(w, FC_QUERY);
        Writer_writeString(w, "SELECT i,d FROM users WHERE b = ?");
        Writer_writeInt32(w, 1);         // 1 param
        Writer_writeByte(w, VT_BLOB);    //     param 0 type
        Writer_writeBlob(w, "Bob", 4);   //     param 0 value
        Writer_writeInt32(w, 2);         // 2 columns
        Writer_writeByte(w, VT_INT64);   //     column 0 type
        Writer_writeByte(w, VT_DOUBLE);  //     column 1 type
        //
        ASSERT(App_step(app));
        //
        ASSERT_INT(1, Reader_readByte(r));              // has row
        ASSERT(Reader_readByte(r));                     //   has value 0
        ASSERT_INT64(2, Reader_readInt64(r));           //   value 0
        ASSERT(Reader_readByte(r));                     //   has value 1
        ASSERT_DOUBLE(2.2, Reader_readDouble(r));       //   value 1
        ASSERT(!Reader_readByte(r));                    // no more rows
        ASSERT(Reader_readByte(r));                     // ok
    }
    {
        Writer_writeByte(w, FC_EXEC);
        Writer_writeString(w, "INSERT INTO unknown_table(a) VALUES (?)");
        Writer_writeInt32(w, 1);        // 1 iterations
        Writer_writeInt32(w, 1);        // 1 params per iteration
        Writer_writeByte(w, VT_INT64);  //     iter 0 param 0 type
        Writer_writeInt64(w, 1);        //     iter 0 param 0 value
        //
        ASSERT(App_step(app));
        //
        ASSERT(!Reader_readByte(r));             // not ok
        const char *str = Reader_readString(r);  // errmsg
        ASSERT_STR("no such table: unknown_table", str);
    }
    {
        Writer_writeByte(w, FC_EXEC);
        Writer_writeString(w, "INSERT INTO users(i,d,s,b) VALUES (1,1,NULL,NULL)");
        Writer_writeInt32(w, 1);  // 1 iterations
        Writer_writeInt32(w, 0);  // 0 params per iteration
        //
        ASSERT(App_step(app));
        //
        ASSERT(!Reader_readByte(r));             // not ok
        const char *str = Reader_readString(r);  // errmsg
        ASSERT_STR("UNIQUE constraint failed: users.i", str);
    }
    {
        Writer_writeByte(w, FC_EXEC);
        Writer_writeString(w, "INSERT INTO users(i,d,s,b) VALUES (3,3,NULL,NULL,NULL,NULL,NULL)");
        Writer_writeInt32(w, 1);  // 1 iterations
        Writer_writeInt32(w, 0);  // 0 params per iteration
        //
        ASSERT(App_step(app));
        //
        ASSERT(!Reader_readByte(r));             // not ok
        const char *msg = Reader_readString(r);  // errmsg
        ASSERT_STR("7 values for 4 columns", msg);
    }
    {
        Writer_writeByte(w, FC_EXEC);
        Writer_writeString(w, "INSERT INTO users(i,d,s,b) VALUES (3,3)");
        Writer_writeInt32(w, 1);  // 1 iterations
        Writer_writeInt32(w, 0);  // 0 params per iteration
        //
        ASSERT(App_step(app));
        //
        ASSERT(!Reader_readByte(r));             // not ok
        const char *msg = Reader_readString(r);  // errmsg
        ASSERT_STR("2 values for 4 columns", msg);
    }
    {
        Writer_writeByte(w, FC_EXEC);
        Writer_writeString(w, "INSERT INTO users(a,b) VALUES (1,1)");
        Writer_writeInt32(w, 1);  // 1 iterations
        Writer_writeInt32(w, 0);  // 0 params per iteration
        //
        ASSERT(App_step(app));
        //
        ASSERT(!Reader_readByte(r));             // not ok
        const char *msg = Reader_readString(r);  // errmsg
        ASSERT_STR("table users has no column named a", msg);
    }
    {
        Writer_writeByte(w, FC_QUIT);
        //
        ASSERT(!App_step(app));
        //
        ASSERT_INT(1, Reader_readByte(r));              // ok
    }
    // free
    App_free(app);
    Writer_free(w);
    Reader_free(r);
    Db_free(db);
}

void testApp() {
    LOG_INFO0("testApp testBasic");
    testBasic();
    LOG_INFO0("testApp testValueTypesAndErrors");
    testValueTypesAndErrors();
}
