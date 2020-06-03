#include "util.h"
#include "mem.h"
#include "mem_test.h"

void test_dbuf() {
    INFO("TEST %s\n", __func__);
    dbuf *req = dbuf_new();
    dbuf_write_bool(req, TRUE);
    bool yes = dbuf_read_bool(req);
    ASSERT(yes, "want TRUE but have %d", yes);
    dbuf_write_byte(req, 13);
    byte thirteen = dbuf_read_byte(req);
    ASSERT(thirteen==13, "want 13 but have %d", thirteen);
    dbuf_write_int32(req, 2<<30);
    int giga= dbuf_read_int32(req);
    ASSERT(giga==2<<30, "want 2<<30 but have %d", giga);
    dbuf_write_double(req, 0.02);
    double dbl = dbuf_read_double(req);
    ASSERT(dbl==0.02, "want 0.02 but have %g", dbl);
    dbuf_write_string(req, "Hello");
    const char *str = dbuf_read_string(req);
    ASSERT(strcmp(str, "Hello") == 0, "want 'Hello' but have '%s'", str);
    dbuf_free(req);
    INFO("TEST %s OK\n", __func__);
}

void bench_dbuf(int nrounds) {
    INFO("BENCH %s\n", __func__);
    DEBUG("nrounds = %d\n", nrounds);
    double t1 = mono_time();
    for (int r=0 ; r<nrounds ; r++) {
        dbuf *req = dbuf_new();
        for (int i=0 ; i<1*1000 ; i++) {
            dbuf_reset(req);
            dbuf_write_bool(req, TRUE);
            dbuf_write_byte(req, 'a');
            dbuf_write_int32(req, 100);
            dbuf_write_int64(req, ((int64)2 << 60) + 3);
            dbuf_write_double(req, 128.25);
            dbuf_write_string(req, "Gallia est omnis divisa in partes tres");
            bool bl = dbuf_read_bool(req);
            byte by = dbuf_read_byte(req);
            int i32 = dbuf_read_int32(req);
            int64 i64 = dbuf_read_int64(req);
            double db = dbuf_read_double(req);
            const char *str = dbuf_read_string(req);
            ASSERT(bl, "wrong %d", bl);
            ASSERT(by == 'a', "wrong %d", by);
            ASSERT(i32 == 100, "wrong %d", i32);
            ASSERT(i64 == ((int64)2 << 60) + 3, "wrong %I64d", i64);
            ASSERT(db == 128.25, "wrong %g", db);
            ASSERT(strcmp(str, "Gallia est omnis divisa in partes tres") == 0, "wrong '%s'", str);
        }
        dbuf_free(req);
    }
    double t2 = mono_time();
    DEBUG("%d rounds took %gs\n", nrounds, mono_diff_sec(t1, t2));    
    INFO("BENCH %s OK\n", __func__);
}
