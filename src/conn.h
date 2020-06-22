#ifndef _CONN_H
#define _CONN_H

#include "sqlite3.h"

const char *lib_version();


typedef struct {
    sqlite3 *db;
    sqlite3_stmt *stmt;
} conn;

conn *conn_new();
void conn_free(conn *this);
int conn_open(conn *this, const char *filename, char *errmsg, int maxerrmsg);
int conn_prepare(conn *this, const char *sql, char *errmsg, int maxerrmsg);
int conn_bind_null(conn *this, int iparam, char *errmsg, int maxerrmsg);
int conn_bind_int(conn *this, int iparam, int value, char *errmsg, int maxerrmsg);
int conn_bind_int64(conn *this, int iparam, int64 value, char *errmsg, int maxerrmsg);
int conn_bind_double(conn *this, int iparam, double value, char *errmsg, int maxerrmsg);
int conn_bind_text(conn *this, int iparam, const char *value, char *errmsg, int maxerrmsg);
int conn_bind_blob(conn *this, int iparam, const byte *value, int blob_size, char *errmsg, int maxerrmsg);
int conn_step(conn *this, bool *more, char *errmsg, int maxerrmsg);
int conn_reset(conn *this, char *errmsg, int maxerrmsg);
void conn_changes(conn *this, int *pchanges);
void conn_column_int(conn *this, int icol, bool *set, int *value);
void conn_column_int64(conn *this, int icol, bool *set, int64 *value);
void conn_column_double(conn *this, int icol, bool *set, double *value);
void conn_column_text(conn *this, int icol, bool *set, const char **pvalue);
void conn_column_blob(conn *this, int icol, bool *set, const byte **pvalue, int *len);
void conn_finalize(conn *this);
int conn_close(conn *this, char *errmsg, int maxerrmsg);

#endif // _CONN_H
