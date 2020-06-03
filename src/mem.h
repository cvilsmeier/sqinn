#ifndef _MEM_H
#define _MEM_H

#define MEM_MALLOC(size)       mem_malloc(__FILE__, __LINE__, (size))
#define MEM_STRDUP(src)        mem_strdup(__FILE__, __LINE__, (src))
#define MEM_REALLOC(ptr, size) mem_realloc(__FILE__, __LINE__, (ptr), (size))
#define MEM_FREE(p)            mem_free(__FILE__, __LINE__, (p))

void *mem_malloc(const char* file, int line, size_t size);
char *mem_strdup(const char* file, int line, const char *s);
void *mem_realloc(const char* file, int line, void *ptr, size_t new_size);
void mem_free(const char* file, int line, void *p);
int mem_usage();

int decode_int32(byte b[4]);
void encode_int32(int value, byte b[4]);

typedef struct {
    size_t cap;  // capacity of buf
    size_t sz;   // size of bytes in buf
    byte *buf;   // byte buffer in memory
    int off;   // read offset
} dbuf;

dbuf *dbuf_new();
void dbuf_free(dbuf *this);
void dbuf_reset(dbuf *this);
void dbuf_write_bytes(dbuf *this, const void *buf, size_t len);
void dbuf_write_dbuf(dbuf *this, const dbuf *that);
void dbuf_write_byte(dbuf *this, byte value);
byte dbuf_read_byte(dbuf *this);
void dbuf_write_bool(dbuf *this, bool value);
bool dbuf_read_bool(dbuf *this);
void dbuf_write_int32(dbuf *this, int value);
int dbuf_read_int32(dbuf *this);
void dbuf_write_int64(dbuf *this, int64 value);
int64 dbuf_read_int64(dbuf *this);
void dbuf_write_string(dbuf *this, const char *str);
const char *dbuf_read_string(dbuf *this);
void dbuf_write_blob(dbuf *this, const byte *data, int len);
const byte *dbuf_read_blob(dbuf *this, int *len);
void dbuf_write_double(dbuf *this, double value);
double dbuf_read_double(dbuf *this);

#endif // _MEM_H

