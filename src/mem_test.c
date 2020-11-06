#include "util.h"
#include "mem.h"
#include "mem_test.h"

void test_dbuf() {
    INFO("TEST %s\n", __func__);
    dbuf *req = dbuf_new();
    // write bool
    dbuf_write_bool(req, TRUE);
    // read bool
    bool yes = dbuf_read_bool(req);
    ASSERT(yes, "want TRUE but have %d", yes);
    // write byte
    dbuf_write_byte(req, 13);
    // read byte
    byte thirteen = dbuf_read_byte(req);
    ASSERT(thirteen==13, "want 13 but have %d", thirteen);
    // write int32
    dbuf_write_int32(req, 2<<30);
    // read int32
    int giga= dbuf_read_int32(req);
    ASSERT(giga==2<<30, "want 2<<30 but have %d", giga);
    // write double as string
    dbuf_write_double_str(req, 0.02);
    // read double as string
    double dbl_str = dbuf_read_double_str(req);
    ASSERT(dbl_str==0.02, "want 0.02 but have %g", dbl_str);
    // write double as ieee
    dbuf_write_double_ieee(req, 128.5);
    // read double as ieee
    double dbl_ieee = dbuf_read_double_ieee(req);
    ASSERT(dbl_ieee==128.5, "want 128.5 but have %g", dbl_ieee);
    // write string
    dbuf_write_string(req, "Hello");
    // read string
    const char *str = dbuf_read_string(req);
    ASSERT(strcmp(str, "Hello") == 0, "want 'Hello' but have '%s'", str);
    // check memory
    ASSERT(req->sz == 33, "wrong req->sz %d", req->sz);
    ASSERT(req->off == 33, "wrong req->off %d", req->off);
    // bool
    ASSERT(req->buf[ 0] == 0x01, "wrong mem %02X", req->buf[ 0]); // bool
    // byte
    ASSERT(req->buf[ 1] == 0x0D, "wrong mem %02X", req->buf[ 1]); // byte
    // int32
    ASSERT(req->buf[ 2] == 0x80, "wrong mem %02X", req->buf[ 2]); // int32
    ASSERT(req->buf[ 3] == 0x00, "wrong mem %02X", req->buf[ 3]);
    ASSERT(req->buf[ 4] == 0x00, "wrong mem %02X", req->buf[ 4]);
    ASSERT(req->buf[ 5] == 0x00, "wrong mem %02X", req->buf[ 5]);
    // double_str
    ASSERT(req->buf[ 6] == 0x00, "wrong mem %02X", req->buf[ 6]); // double_str (len)
    ASSERT(req->buf[ 7] == 0x00, "wrong mem %02X", req->buf[ 7]);
    ASSERT(req->buf[ 8] == 0x00, "wrong mem %02X", req->buf[ 8]);
    ASSERT(req->buf[ 9] == 0x05, "wrong mem %02X", req->buf[ 9]);
    ASSERT(req->buf[10] == 0x30, "wrong mem %02X", req->buf[10]); // double_str (content)
    ASSERT(req->buf[11] == 0x2E, "wrong mem %02X", req->buf[11]);
    ASSERT(req->buf[12] == 0x30, "wrong mem %02X", req->buf[12]);
    ASSERT(req->buf[13] == 0x32, "wrong mem %02X", req->buf[13]);
    ASSERT(req->buf[14] == 0x00, "wrong mem %02X", req->buf[14]);
    // double_ieee
    ASSERT(req->buf[15] == 0x40, "wrong mem %02X", req->buf[15]);
    ASSERT(req->buf[16] == 0x60, "wrong mem %02X", req->buf[16]);
    ASSERT(req->buf[17] == 0x10, "wrong mem %02X", req->buf[17]);
    ASSERT(req->buf[18] == 0x00, "wrong mem %02X", req->buf[18]);
    ASSERT(req->buf[19] == 0x00, "wrong mem %02X", req->buf[19]);
    ASSERT(req->buf[20] == 0x00, "wrong mem %02X", req->buf[20]);
    ASSERT(req->buf[21] == 0x00, "wrong mem %02X", req->buf[21]);
    ASSERT(req->buf[22] == 0x00, "wrong mem %02X", req->buf[22]);
    // string
    ASSERT(req->buf[23] == 0x00, "wrong mem %02X", req->buf[23]);
    ASSERT(req->buf[24] == 0x00, "wrong mem %02X", req->buf[24]);
    ASSERT(req->buf[25] == 0x00, "wrong mem %02X", req->buf[25]);
    ASSERT(req->buf[26] == 0x06, "wrong mem %02X", req->buf[26]);
    ASSERT(req->buf[27] == 0x48, "wrong mem %02X", req->buf[27]);
    ASSERT(req->buf[28] == 0x65, "wrong mem %02X", req->buf[28]);
    ASSERT(req->buf[29] == 0x6C, "wrong mem %02X", req->buf[29]);
    ASSERT(req->buf[30] == 0x6C, "wrong mem %02X", req->buf[30]);
    ASSERT(req->buf[31] == 0x6F, "wrong mem %02X", req->buf[31]);
    ASSERT(req->buf[32] == 0x00, "wrong mem %02X", req->buf[32]);
    // cleanup
    dbuf_free(req);
    INFO("TEST %s OK\n", __func__);
}

// see https://en.wikipedia.org/wiki/Double-precision_floating-point_format
void test_dbuf_double() {
    INFO("TEST %s\n", __func__);
    dbuf *req = dbuf_new();
    // write
    dbuf_write_double_ieee(req, -2);
    ASSERT(req->sz == 8, "wrong %d", req->sz);
    ASSERT(req->buf[0] == 0xC0, "wrong %02X", req->buf[0]);
    ASSERT(req->buf[1] == 0x00, "wrong %02X", req->buf[1]);
    ASSERT(req->buf[2] == 0x00, "wrong %02X", req->buf[2]);
    ASSERT(req->buf[3] == 0x00, "wrong %02X", req->buf[3]);
    ASSERT(req->buf[4] == 0x00, "wrong %02X", req->buf[4]);
    ASSERT(req->buf[5] == 0x00, "wrong %02X", req->buf[5]);
    ASSERT(req->buf[6] == 0x00, "wrong %02X", req->buf[6]);
    ASSERT(req->buf[7] == 0x00, "wrong %02X", req->buf[7]);
    // write
    dbuf_write_double_ieee(req, 2);
    ASSERT(req->sz == 16, "wrong %d", req->sz);
    ASSERT(req->buf[ 8] == 0x40, "wrong %02X", req->buf[ 8]);
    ASSERT(req->buf[ 9] == 0x00, "wrong %02X", req->buf[ 9]);
    ASSERT(req->buf[10] == 0x00, "wrong %02X", req->buf[10]);
    ASSERT(req->buf[11] == 0x00, "wrong %02X", req->buf[11]);
    ASSERT(req->buf[12] == 0x00, "wrong %02X", req->buf[12]);
    ASSERT(req->buf[13] == 0x00, "wrong %02X", req->buf[13]);
    ASSERT(req->buf[14] == 0x00, "wrong %02X", req->buf[14]);
    ASSERT(req->buf[15] == 0x00, "wrong %02X", req->buf[15]);
    // write
    dbuf_write_double_ieee(req, 128.5);
    ASSERT(req->sz == 24, "wrong %d", req->sz);
    ASSERT(req->buf[16] == 0x40, "wrong %02X", req->buf[16]);
    ASSERT(req->buf[17] == 0x60, "wrong %02X", req->buf[17]);
    ASSERT(req->buf[18] == 0x10, "wrong %02X", req->buf[18]);
    ASSERT(req->buf[19] == 0x00, "wrong %02X", req->buf[19]);
    ASSERT(req->buf[20] == 0x00, "wrong %02X", req->buf[20]);
    ASSERT(req->buf[21] == 0x00, "wrong %02X", req->buf[21]);
    ASSERT(req->buf[22] == 0x00, "wrong %02X", req->buf[22]);
    ASSERT(req->buf[23] == 0x00, "wrong %02X", req->buf[23]);
    // free
    dbuf_free(req);
    INFO("TEST %s OK\n", __func__);
}

void bench_dbuf(int nrounds) {
    INFO("BENCH %s\n", __func__);
    DEBUG("  nrounds = %d\n", nrounds);
    double tstart = mono_time();
    for (int r=0 ; r<nrounds ; r++) {
        dbuf *req = dbuf_new();
        for (int i=0 ; i<1*1000 ; i++) {
            dbuf_reset(req);
            dbuf_write_bool(req, TRUE);
            dbuf_write_byte(req, 'a');
            dbuf_write_int32(req, 100);
            dbuf_write_int64(req, ((int64)2 << 60) + 3);
            dbuf_write_double_str(req, 128.25);
            dbuf_write_double_ieee(req, 111.2);
            dbuf_write_string(req, "Gallia est omnis divisa in partes tres");
            bool bl = dbuf_read_bool(req);
            ASSERT(bl, "wrong %d", bl);
            byte by = dbuf_read_byte(req);
            ASSERT(by == 'a', "wrong %d", by);
            int i32 = dbuf_read_int32(req);
            ASSERT(i32 == 100, "wrong %d", i32);
            int64 i64 = dbuf_read_int64(req);
            ASSERT(i64 == ((int64)2 << 60) + 3, "wrong i64 %"FMT_PRId64, i64);
            double d_str = dbuf_read_double_str(req);
            ASSERT(d_str == 128.25, "wrong d_str %g", d_str);
            double d_ieee = dbuf_read_double_ieee(req);
            ASSERT(d_ieee == 111.2, "wrong d_ieee %g", d_ieee);
            const char *str = dbuf_read_string(req);
            ASSERT(strcmp(str, "Gallia est omnis divisa in partes tres") == 0, "wrong '%s'", str);
        }
        dbuf_free(req);
    }
    DEBUG("  rounds took %gs\n", nrounds, mono_since(tstart));    
    INFO("BENCH %s OK\n", __func__);
}
