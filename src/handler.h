#ifndef _HANDLER_H
#define _HANDLER_H

// function codes

#define FC_SQINN_VERSION     1
#define FC_IO_VERSION        2
#define FC_SQLITE_VERSION    3
#define FC_OPEN             10
#define FC_PREPARE          11
#define FC_BIND             12
#define FC_STEP             13
#define FC_RESET            14
#define FC_CHANGES          15
#define FC_COLUMN           16
#define FC_FINALIZE         17
#define FC_CLOSE            18
#define FC_COLUMN_COUNT     19
#define FC_COLUMN_TYPE      20
#define FC_COLUMN_NAME      21
#define FC_EXEC             51
#define FC_QUERY            52

// value types

#define VAL_NULL        0
#define VAL_INT         1
#define VAL_INT64       2
#define VAL_DOUBLE_STR  3
#define VAL_TEXT        4
#define VAL_BLOB        5
#define VAL_DOUBLE_IEEE 6

// handler struct

typedef struct {
    conn *con;
} handler;

handler *handler_new();
void handler_free(handler *this);
void handler_handle(handler *this, dbuf *req, dbuf *resp);

#endif // _HANDLER_H

