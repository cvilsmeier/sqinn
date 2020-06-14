#include "util.h"
#include "mem.h"

// mem: memory allocation primitives

static int mem_alloc_count = 0;
static int mem_free_count = 0;

void *mem_malloc(const char* file, int line, size_t size) {
    mem_alloc_count++;
    void *p = malloc(size);
    ASSERT(p, "malloc: %s", "out of memory");
    return p;
}

void *mem_realloc(const char* file, int line, void *ptr, size_t new_size) {
    void *p = realloc(ptr, new_size);
    ASSERT(p, "realloc: %s", "out of memory");
    return p;
}

void mem_free(const char* file, int line, void *p) {
    if(p) {
        mem_free_count++;
        free(p);
    }
}

int mem_usage() {
    return mem_alloc_count - mem_free_count;
}

// decode/encode int32 values

int decode_int32(byte b[4]) {
    return
        ((int) b[0]) << 24 |
        ((int) b[1]) << 16 |
        ((int) b[2]) <<  8 |
        ((int) b[3]) <<  0 ;
}

void encode_int32(int value, byte b[4]) {
     b[0] = ((byte)(value >> 24)) & 0xFF;
     b[1] = ((byte)(value >> 16)) & 0xFF;
     b[2] = ((byte)(value >>  8)) & 0xFF;
     b[3] = ((byte)(value >>  0)) & 0xFF;
}

// dbuf: dynamic byte buffer

#define MAX_RETAIN 10*1024

dbuf *dbuf_new() {
    dbuf *this = (dbuf *)MEM_MALLOC(sizeof(dbuf));
    this->cap = 64;
    this->sz = 0;
    this->buf = MEM_MALLOC(this->cap*sizeof(byte));
    this->off = 0;
    return this;
}

void dbuf_free(dbuf *this) {
    MEM_FREE(this->buf);
    MEM_FREE(this);
}

void dbuf_reset(dbuf *this) {
    if (this->cap > MAX_RETAIN) {
        this->cap = MAX_RETAIN;
        this->buf = MEM_REALLOC(this->buf, MAX_RETAIN*sizeof(byte));
    }
    this->sz = 0;
    this->off = 0;
}

void _dbuf_ensure_cap(dbuf *this, size_t min_cap) {
    size_t new_cap = this->cap;
    while (new_cap < min_cap) {
        new_cap *= 2;
    }
    if (new_cap <= this->cap) {
        return;
    }
    // INFO("dbuf cap now %d\n", (int)new_cap);
    this->buf = MEM_REALLOC(this->buf, new_cap*sizeof(byte));
}

void dbuf_write_bytes(dbuf *this, const void *buf, size_t len) {
    _dbuf_ensure_cap(this, this->sz + len);
    memcpy(&this->buf[this->sz], buf, len);
    this->sz += len;
}

byte *_dbuf_read_bytes(dbuf *this, size_t len) {
    size_t left = this->sz - this->off;
    ASSERT(left >= len, "dbuf read beyond limit: left=%"PRIu64", len=%"PRIu64, left, len);
    byte *p = &this->buf[this->off];
    this->off += len;
    return p;
}

void dbuf_write_dbuf(dbuf *this, const dbuf *that) {
     dbuf_write_bytes(this, that->buf, that->sz);
}

void dbuf_write_byte(dbuf *this, byte value) {
     byte b = value;
     dbuf_write_bytes(this, &b, 1);
}

byte dbuf_read_byte(dbuf *this) {
    byte *p = _dbuf_read_bytes(this, 1);
    return *p;
}

void dbuf_write_bool(dbuf *this, bool value) {
     byte b = (value ? 1 :0);
     dbuf_write_bytes(this, &b, 1);
}

bool dbuf_read_bool(dbuf *this) {
    byte *p = _dbuf_read_bytes(this, 1);
    return *p ? TRUE : FALSE;
}

void dbuf_write_int32(dbuf *this, int value) {
     byte b[4];
     encode_int32(value, b);
     dbuf_write_bytes(this, b, 4);
}

int dbuf_read_int32(dbuf *this) {
    byte *p = _dbuf_read_bytes(this, 4);
    return decode_int32(p);
}

void dbuf_write_int64(dbuf *this, int64 value) {
    byte b[8];
    b[0] = ((byte)(value >> 56)) & 0xFF;
    b[1] = ((byte)(value >> 48)) & 0xFF;
    b[2] = ((byte)(value >> 40)) & 0xFF;
    b[3] = ((byte)(value >> 32)) & 0xFF;
    b[4] = ((byte)(value >> 24)) & 0xFF;
    b[5] = ((byte)(value >> 16)) & 0xFF;
    b[6] = ((byte)(value >>  8)) & 0xFF;
    b[7] = ((byte)(value >>  0)) & 0xFF;
    dbuf_write_bytes(this, b, 8);
}

int64 dbuf_read_int64(dbuf *this) {
    byte *p = _dbuf_read_bytes(this, 8);
    return
        ((int64) p[0]) << 56 |
        ((int64) p[1]) << 48 |
        ((int64) p[2]) << 40 |
        ((int64) p[3]) << 32 |
        ((int64) p[4]) << 24 |
        ((int64) p[5]) << 16 |
        ((int64) p[6]) <<  8 |
        ((int64) p[7]) <<  0 ;
}

void dbuf_write_string(dbuf *this, const char *str) {
    size_t len = strlen(str);
    dbuf_write_int32(this, len+1);
    dbuf_write_bytes(this, (byte*)str, len+1);
}

const char *dbuf_read_string(dbuf *this) {
    int size = dbuf_read_int32(this);
    byte *p = _dbuf_read_bytes(this, size);
    return (const char *)p;
}

void dbuf_write_blob(dbuf *this, const byte *data, int len) {
    dbuf_write_int32(this, len);
    dbuf_write_bytes(this, data, len);
}

const byte *dbuf_read_blob(dbuf *this, int *len) {
    *len = dbuf_read_int32(this);
    const byte *data = _dbuf_read_bytes(this, *len);
    return data;
}

void dbuf_write_double_str(dbuf *this, double value) {
    char tmp[64];
    snprintf(tmp, sizeof(tmp)-1, "%g", value);
    dbuf_write_string(this, tmp);
}

double dbuf_read_double_str(dbuf *this) {
    const char *str = dbuf_read_string(this);
    return strtod(str, NULL);
}

void dbuf_write_double_ieee(dbuf *this, double value) {
    byte *byte_ptr = (byte *)&value;
    byte buf[8];
    buf[0] = byte_ptr[7];
    buf[1] = byte_ptr[6];
    buf[2] = byte_ptr[5];
    buf[3] = byte_ptr[4];
    buf[4] = byte_ptr[3];
    buf[5] = byte_ptr[2];
    buf[6] = byte_ptr[1];
    buf[7] = byte_ptr[0];
    dbuf_write_bytes(this, buf, 8);
}

double dbuf_read_double_ieee(dbuf *this) {
    byte *byte_ptr = _dbuf_read_bytes(this, 8);
    byte buf[8];
    buf[0] = byte_ptr[7];
    buf[1] = byte_ptr[6];
    buf[2] = byte_ptr[5];
    buf[3] = byte_ptr[4];
    buf[4] = byte_ptr[3];
    buf[5] = byte_ptr[2];
    buf[6] = byte_ptr[1];
    buf[7] = byte_ptr[0];
    double *double_ptr = (double *)buf;
    return *double_ptr;
}
