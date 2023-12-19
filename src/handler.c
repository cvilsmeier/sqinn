#include "util.h"
#include "mem.h"
#include "conn.h"
#include "handler.h"

#define MAX_MESSAGE 128

handler *handler_new() {
    handler *this = MEM_MALLOC(sizeof(handler));
    this->con = conn_new();
    return this;
}

void handler_free(handler *this) {
    conn_free(this->con);
    MEM_FREE(this);
}

void _write_ok_or_err(int err, const char *errmsg, dbuf *resp) {
    if(!err) {
        dbuf_write_bool(resp, TRUE);
    } else {
        dbuf_write_bool(resp, FALSE);
        dbuf_write_string(resp, errmsg);
    }
}

int _bind_param(conn *con, int iparam, dbuf *req, char *errmsg, int maxerrmsg) {
    byte val_type = dbuf_read_byte(req);
    switch (val_type) {
        case VAL_NULL:
        {
            return conn_bind_null(con, iparam, errmsg, maxerrmsg);
        }
        case VAL_INT:
        {
            int ival = dbuf_read_int32(req);
            return conn_bind_int(con, iparam, ival, errmsg, maxerrmsg);
        }
        case VAL_INT64:
        {
            int64 ival = dbuf_read_int64(req);
            return conn_bind_int64(con, iparam, ival, errmsg, maxerrmsg);
        }
        case VAL_DOUBLE_STR:
        {
            double dval = dbuf_read_double_str(req);
            return conn_bind_double(con, iparam, dval, errmsg, maxerrmsg);
        }
        case VAL_DOUBLE_IEEE:
        {
            double dval = dbuf_read_double_ieee(req);
            return conn_bind_double(con, iparam, dval, errmsg, maxerrmsg);
        }
        case VAL_TEXT:
        {
            const char *sval = dbuf_read_string(req);
            return conn_bind_text(con, iparam, sval, errmsg, maxerrmsg);
        }
        case VAL_BLOB:
        {
            int len;
            const byte *bval = dbuf_read_blob(req, &len);
            return conn_bind_blob(con, iparam, bval, len, errmsg, maxerrmsg);
        }
    }
    snprintf(errmsg, maxerrmsg, "unknown bind val_type %d", val_type);
    INFO("%s\n", errmsg);
    return SQLITE_ERROR;
}

int _column(conn *con, byte val_type, int icol, dbuf *dest, char *errmsg, int maxerrmsg) {
    bool set;
    switch (val_type) {
        case VAL_INT: {
            int ival;
            conn_column_int(con, icol, &set, &ival);
            dbuf_write_bool(dest, set);
            if (set) {
                dbuf_write_int32(dest, ival);
            }
            return SQLITE_OK;
        }
        case VAL_INT64: {
            int64 ival;
            conn_column_int64(con, icol, &set, &ival);
            dbuf_write_bool(dest, set);
            if (set) {
                dbuf_write_int64(dest, ival);
            }
            return SQLITE_OK;
        }
        case VAL_DOUBLE_STR: {
            double dval;
            conn_column_double(con, icol, &set, &dval);
            dbuf_write_bool(dest, set);
            if (set) {
                dbuf_write_double_str(dest, dval);
            }
            return SQLITE_OK;
        }
        case VAL_DOUBLE_IEEE: {
            double dval;
            conn_column_double(con, icol, &set, &dval);
            dbuf_write_bool(dest, set);
            if (set) {
                dbuf_write_double_ieee(dest, dval);
            }
            return SQLITE_OK;
        }
        case VAL_TEXT: {
            const char *sval;
            conn_column_text(con, icol, &set, &sval);
            dbuf_write_bool(dest, set);
            if (set) {
                dbuf_write_string(dest, sval);
            }
            return SQLITE_OK;
        }
        case VAL_BLOB: {
            const byte *bval;
            int len;
            conn_column_blob(con, icol, &set, &bval, &len);
            dbuf_write_bool(dest, set);
            if (set) {
                dbuf_write_blob(dest, bval, len);
            }
            return SQLITE_OK;
        }
    }
    snprintf(errmsg, maxerrmsg, "unknown column col_type %d", val_type);
    INFO("%s\n", errmsg);
    return SQLITE_ERROR;
}

int _fetch_rows(conn *con, dbuf *req, int *nrows, dbuf *vbuf, char *errmsg, int maxerrmsg) {
    int ncols = dbuf_read_int32(req);
    byte *val_types = (byte *)MEM_MALLOC(ncols*sizeof(byte));
    for (int icol=0 ; icol<ncols ; icol++) {
        val_types[icol] = dbuf_read_byte(req);
    }
    bool more = TRUE;
    int err = SQLITE_OK;
    *nrows = 0;
    while (!err && more) {
        err = conn_step(con, &more, errmsg, maxerrmsg);
        if (!err && more) {
            *nrows = *nrows + 1;
            for (int icol=0 ; !err && icol<ncols ; icol++) {
                byte val_type = val_types[icol];
                err = _column(con, val_type, icol, vbuf, errmsg, maxerrmsg);
            }
        }
    }
    MEM_FREE(val_types);
    return err;
}

byte _type_from_sqlite(int sqlite_type) {
    switch (sqlite_type) {
        case SQLITE_INTEGER:
            return VAL_INT64;
        case SQLITE_FLOAT:
            return VAL_DOUBLE_IEEE;
        case SQLITE3_TEXT:
            return VAL_TEXT;
        case SQLITE_BLOB:
            return VAL_BLOB;
        default:
            return VAL_NULL;
    }
    return VAL_NULL;
}

void handler_handle(handler *this, dbuf *req, dbuf *resp) {
    char message[MAX_MESSAGE+1];
    conn *con = this->con;
    byte fc = dbuf_read_byte(req);
    switch (fc) {
        case FC_SQINN_VERSION:
        {
            dbuf_write_bool(resp, TRUE);
            dbuf_write_string(resp, SQINN_VERSION);
        }
        break;
        case FC_IO_VERSION:
        {
            dbuf_write_bool(resp, TRUE);
            dbuf_write_byte(resp, IO_VERSION);
        }
        break;
        case FC_SQLITE_VERSION:
        {
            const char *v = lib_version(con);
            dbuf_write_bool(resp, TRUE);
            dbuf_write_string(resp, v);
        }
        break;
        case FC_OPEN:
        {
            const char *filename = dbuf_read_string(req);
            int err = conn_open(con, filename, message, MAX_MESSAGE);
            _write_ok_or_err(err, message, resp);
        }
        break;
        case FC_PREPARE:
        {
            const char *sql = dbuf_read_string(req);
            int err = conn_prepare(con, sql, message, MAX_MESSAGE);
            _write_ok_or_err(err, message, resp);
        }
        break;
        case FC_BIND:
        {
            int iparam = dbuf_read_int32(req);
            int err = _bind_param(con, iparam, req, message, MAX_MESSAGE);
            _write_ok_or_err(err, message, resp);
        }
        break;
        case FC_STEP:
        {
            bool more;
            int err = conn_step(con, &more, message, MAX_MESSAGE);
            _write_ok_or_err(err, message, resp);
            if(!err) {
                dbuf_write_bool(resp, more);
            }
        }
        break;
        case FC_RESET:
        {
            int err = conn_reset(con, message, MAX_MESSAGE);
            _write_ok_or_err(err, message, resp);
        }
        break;
        case FC_CHANGES:
        {
            int changes;
            conn_changes(con, &changes);
            dbuf_write_bool(resp, TRUE);
            dbuf_write_int32(resp, changes);
        }
        break;
        case FC_COLUMN:
        {
            int icol = dbuf_read_int32(req);
            byte col_type = dbuf_read_byte(req);
            dbuf *vbuf = dbuf_new();
            int err = _column(con, col_type, icol, vbuf, message, MAX_MESSAGE);
            _write_ok_or_err(err, message, resp);
            if(!err) {
                dbuf_write_dbuf(resp, vbuf);
            }
            dbuf_free(vbuf);
        }
        break;
        case FC_FINALIZE:
        {
            conn_finalize(con);
            _write_ok_or_err(SQLITE_OK, message, resp);
        }
        break;
        case FC_CLOSE:
        {
            int err = conn_close(con, message, MAX_MESSAGE);
            _write_ok_or_err(err, message, resp);
        }
        break;
        case FC_COLUMN_COUNT:
        {
            int count = conn_column_count(con);
            dbuf_write_bool(resp, TRUE);
            dbuf_write_int32(resp, count);
        }
        break;
        case FC_COLUMN_TYPE:
        {
            int icol = dbuf_read_int32(req);
            int type = conn_column_type(con, icol);
            dbuf_write_bool(resp, TRUE);
            dbuf_write_byte(resp, _type_from_sqlite(type));
        }
        break;
        case FC_COLUMN_NAME:
        {
            int icol = dbuf_read_int32(req);
            conn_column_name(con, icol, message, MAX_MESSAGE);
            dbuf_write_bool(resp, TRUE);
            dbuf_write_string(resp, message);
        }
        break;
        case FC_EXEC:
        {
            const char *sql = dbuf_read_string(req);
            bool prepared = FALSE;
            int err = conn_prepare(con, sql, message, MAX_MESSAGE);
            if (!err) {
                prepared = TRUE;
            }
            int niterations = 0;
            int *changes = NULL;
            if (!err) {
                niterations = dbuf_read_int32(req);
                int nparams = dbuf_read_int32(req);
                changes = MEM_MALLOC(niterations * sizeof(int));
                for (int iit = 0; !err && iit < niterations; iit++) {
                    for (int iparam = 1; !err && iparam <= nparams; iparam++) {
                        err = _bind_param(con, iparam, req, message, MAX_MESSAGE);
                    }
                    if (!err) {
                        err = conn_step(con, NULL, message, MAX_MESSAGE);
                    }
                    if (!err) {
                        conn_changes(con, &changes[iit]);
                    }
                    if (!err) {
                        if (iit < niterations-1) {
                            err = conn_reset(con, message, MAX_MESSAGE);
                        }
                    }
                }
            }
            if (prepared) {
                conn_finalize(con);
            }
            _write_ok_or_err(err, message, resp);
            if (!err) {
                for (int iit = 0; iit < niterations; iit++) {
                    dbuf_write_int32(resp, changes[iit]);
                }
            }
            MEM_FREE(changes);
        }
        break;
        case FC_QUERY:
        {
            dbuf *vbuf = dbuf_new();
            const char *sql = dbuf_read_string(req);
            bool prepared = FALSE;
            int err = conn_prepare(con, sql, message, MAX_MESSAGE);
            if (!err) {
                prepared = TRUE;
                int nparams = dbuf_read_int32(req);
                for (int iparam=1 ; !err && iparam<=nparams ; iparam++) {
                    err = _bind_param(con, iparam, req, message, MAX_MESSAGE);
                }
            }
            int nrows = 0;
            if (!err) {
                err = _fetch_rows(con, req, &nrows, vbuf, message, MAX_MESSAGE);
            }
            if (prepared) {
                conn_finalize(con);
            }
            _write_ok_or_err(err, message, resp);
            if (!err) {
                dbuf_write_int32(resp, nrows);
                dbuf_write_dbuf(resp, vbuf);
            }
            dbuf_free(vbuf);
        }
        break;
        default:
        {
            snprintf(message, MAX_MESSAGE, "unknown function code %u", fc);
            dbuf_write_bool(resp, FALSE);
            dbuf_write_string(resp, message);
        }
        break;
    }
}
