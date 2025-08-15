#ifndef DB_H
#define DB_H

/* A Value holds a typed value. */
typedef struct value_s {
    char type;      // see VT_...
    int i32;        // VT_INT32
    int64_t i64;    // VT_INT64
    double d;       // VT_DOUBLE
    const char *p;  // VT_STRING and VT_BLOB
    size_t sz;      // VT_BLOB
} Value;

#define VT_NULL   0
#define VT_INT32  1
#define VT_INT64  2
#define VT_DOUBLE 3
#define VT_STRING 4
#define VT_BLOB   5

/* A Db provides access to a SQLite database. */
typedef struct db_s Db;
Db *newDb(const char *dbname, BOOL debug);
void Db_free(Db *this);
BOOL Db_prepare(Db *this, const char *sql);
void Db_finalize(Db *this);
BOOL Db_bind(Db *this, const Value *params, int nparams);
BOOL Db_bind_step_reset(Db *this, const Value *params, int nparams);
BOOL Db_step_fetch(Db *this, BOOL *phasRow, Value *values, int nvalues);
const char *Db_errmsg(Db *this);

//
// Test
//

void testDb();

#endif  // DB_H
