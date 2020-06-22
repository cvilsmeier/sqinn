#include "util.h"
#include "mem.h"
#include "conn.h"

const char *lib_version() {
    return sqlite3_libversion();
}

conn *conn_new() {
    conn *this = MEM_MALLOC(sizeof(conn));
    this->db = NULL;
    this->stmt = NULL;
    return this;
}

void conn_free(conn *this) {
    MEM_FREE(this);
}

int conn_open(conn *this, const char *filename, char *errmsg, int maxerrmsg) {
    if (this->db) {
        snprintf(errmsg, maxerrmsg, "cannot open, db is already open");
        return SQLITE_ERROR;
    }
    int err = sqlite3_open_v2(filename, &this->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (err != SQLITE_OK) {
        snprintf(errmsg, maxerrmsg, "sqlite3_open_v2: err=%d, msg=%s", err, sqlite3_errmsg(this->db));
    }
    return err;
}

int conn_prepare(conn *this, const char *sql, char *errmsg, int maxerrmsg) {
    if (this->stmt) {
        snprintf(errmsg, maxerrmsg, "cannot prepare, stmt is already set, must finalize first");
        return SQLITE_ERROR;
    }
    int err = sqlite3_prepare_v2(this->db, sql, -1, &this->stmt, 0);
    if (err != SQLITE_OK) {
        this->stmt = NULL;
        snprintf(errmsg, maxerrmsg, "sqlite3_prepare_v2: err=%d, msg=%s", err, sqlite3_errmsg(this->db));
    }
    return err;
}

int conn_bind_null(conn *this, int iparam, char *errmsg, int maxerrmsg) {
    int err = sqlite3_bind_null(this->stmt, iparam);
    if (err != SQLITE_OK) {
        snprintf(errmsg, maxerrmsg, "sqlite3_bind_null: err=%d, msg=%s", err, sqlite3_errmsg(this->db));
    }
    return err;
}

int conn_bind_int(conn *this, int iparam, int value, char *errmsg, int maxerrmsg) {
    int err = sqlite3_bind_int(this->stmt, iparam, value);
    if (err != SQLITE_OK) {
        snprintf(errmsg, maxerrmsg, "sqlite3_bind_int: err=%d, msg=%s", err, sqlite3_errmsg(this->db));
    }
    return err;
}

int conn_bind_int64(conn *this, int iparam, int64 value, char *errmsg, int maxerrmsg) {
    int err = sqlite3_bind_int64(this->stmt, iparam, (sqlite_int64)value);
    if (err != SQLITE_OK) {
        snprintf(errmsg, maxerrmsg, "sqlite3_bind_int64: err=%d, msg=%s", err, sqlite3_errmsg(this->db));
    }
    return err;
}

int conn_bind_double(conn *this, int iparam, double value, char *errmsg, int maxerrmsg) {
    int err = sqlite3_bind_double(this->stmt, iparam, value);
    if (err != SQLITE_OK) {
        snprintf(errmsg, maxerrmsg, "sqlite3_bind_double: err=%d, msg=%s", err, sqlite3_errmsg(this->db));
    }
    return err;
}

int conn_bind_text(conn *this, int iparam, const char *value, char *errmsg, int maxerrmsg) {
    int err = sqlite3_bind_text(this->stmt, iparam, value, -1, SQLITE_TRANSIENT);
    if (err != SQLITE_OK) {
        snprintf(errmsg, maxerrmsg, "conn_bind_text: err=%d, msg=%s", err, sqlite3_errmsg(this->db));
    }
    return err;
}

int conn_bind_blob(conn *this, int iparam, const byte *value, int len, char *errmsg, int maxerrmsg) {
    int err = sqlite3_bind_blob(this->stmt, iparam, value, len, SQLITE_TRANSIENT);
    if (err != SQLITE_OK) {
        snprintf(errmsg, maxerrmsg, "conn_bind_blob: err=%d, msg=%s", err, sqlite3_errmsg(this->db));
    }
    return err;
}

int conn_step(conn *this, bool *more, char *errmsg, int maxerrmsg) {
    int err = sqlite3_step(this->stmt);
    if (err == SQLITE_OK || err == SQLITE_ROW || err == SQLITE_DONE) {
        if (more != NULL) {
            *more = (err == SQLITE_ROW) ? TRUE : FALSE;
        }
        err = SQLITE_OK;
    } else {
        snprintf(errmsg, maxerrmsg, "sqlite3_step: err=%d, msg=%s", err, sqlite3_errmsg(this->db));
    }
    return err;
}

int conn_reset(conn *this, char *errmsg, int maxerrmsg) {
    int err = sqlite3_reset(this->stmt);
    if (err != SQLITE_OK) {
        snprintf(errmsg, maxerrmsg, "sqlite3_reset: err=%d, msg=%s", err, sqlite3_errmsg(this->db));
    }
    return err;
}

void conn_changes(conn *this, int *pchanges) {
    *pchanges = sqlite3_changes(this->db);
}

void conn_column_int(conn *this, int icol, bool *set, int *value) {
    *value = sqlite3_column_int(this->stmt, icol);
    *set = TRUE;
    if (!*value) {
        int type = sqlite3_column_type(this->stmt, icol);
        if (type == SQLITE_NULL) {
            *set = FALSE;
        }
    }
}

void conn_column_int64(conn *this, int icol, bool *set, int64 *value) {
    *value = (int64)sqlite3_column_int64(this->stmt, icol);
    *set = TRUE;
    if (!*value) {
        int type = sqlite3_column_type(this->stmt, icol);
        if (type == SQLITE_NULL) {
            *set = FALSE;
        }
    }
}

void conn_column_double(conn *this, int icol, bool *set, double *value) {
    *value = sqlite3_column_double(this->stmt, icol);
    *set = TRUE;
    if (*value == (double)0) {
        int type = sqlite3_column_type(this->stmt, icol);
        if (type == SQLITE_NULL) {
            *set = FALSE;
        }
    }
}

void conn_column_text(conn *this, int icol, bool *set, const char **pvalue) {
    const char *val = (const char *)sqlite3_column_text(this->stmt, icol);
    *pvalue = val;
    *set = (val != NULL);
}

void conn_column_blob(conn *this, int icol, bool *set, const byte **pvalue, int *len) {
    const byte *val = (const byte *)sqlite3_column_blob(this->stmt, icol);
    *pvalue = val;
    *set = (val != NULL);
    if (val) {
        *len = sqlite3_column_bytes(this->stmt, icol);
    }
}

void conn_finalize(conn *this) {
    sqlite3_finalize(this->stmt);
    this->stmt = NULL;
}

int conn_close(conn *this, char *errmsg, int maxerrmsg) {
    int err = sqlite3_close(this->db);
    if (err == SQLITE_OK) {
        this->db = NULL;
    } else {
        snprintf(errmsg, maxerrmsg, "sqlite3_close: err=%d, msg=%s", err, sqlite3_errmsg(this->db));
    }
    return err;
}
