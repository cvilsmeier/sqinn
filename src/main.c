#include <fcntl.h>
#include <limits.h>
#include "util.h"
#include "mem.h"
#include "mem_test.h"
#include "conn.h"
#include "conn_test.h"
#include "handler.h"
#include "handler_test.h"

void test_int64() {
    INFO("TEST %s\n", __func__);
    DEBUG("PRIu64=%s, PRId64=%s\n", PRIu64, PRId64);
    ASSERT(sizeof(size_t)==8, "expected size_t to be 8 bytes but was %"PRIu64, sizeof(size_t));
    ASSERT(sizeof(int64)==8, "expected int64 to be 8 bytes but was %"PRIu64, sizeof(int64));
    char buf[64];
    int64 x = ((int64)1 << 63);
    snprintf(buf, sizeof(buf), "%lld", x);
    ASSERT(strcmp(buf, "-9223372036854775808")==0, "expected x to be '-9223372036854775808' but was '%s'", buf);
    x = ((int64)1 << 62);
    snprintf(buf, sizeof(buf), "%lld", x);
    ASSERT(strcmp(buf, "4611686018427387904")==0, "expected x to be '4611686018427387904' but was '%s'", buf);
    x = 
        ((int64)0x7F) << 56 |
        ((int64)0xFF) << 48 |
        ((int64)0xFF) << 40 |
        ((int64)0xFF) << 32 |
        ((int64)0xFF) << 24 |
        ((int64)0xFF) << 16 |
        ((int64)0xFF) <<  8 |
        ((int64)0xFF) <<  0;
    snprintf(buf, sizeof(buf), "%lld", x);
    // 9.223.372.036.854.775.807
    ASSERT(strcmp(buf, "9223372036854775807")==0, "expected x to be '9223372036854775807' but was '%s'", buf);
    INFO("TEST %s OK\n", __func__);
}

void loop() {
    handler *hd = handler_new();
    dbuf *req = dbuf_new();
    dbuf *resp = dbuf_new();
    bool quit = FALSE;
    while (!quit) {
        dbuf_reset(req);
        dbuf_reset(resp);
        // read req
        {
            byte sz_buf[4];
            // DBG("waiting for sz bytes\n");
            size_t c = fread(sz_buf, 1, 4, stdin);
            if (c != 4) {
                INFO("EOF: expected 4 bytes, got %I64u\n", c);
                return;
            }
            int sz = decode_int32(sz_buf);
            // DBG("received 4 sz bytes, sz = %I64u\n", sz);
            if (sz) {
                byte *data_buf = MEM_MALLOC(sz * sizeof(byte));
                // DBG("waiting for %I64u data bytes\n", sz);
                c = fread(data_buf, 1, sz, stdin);
                if (c != sz) {
                    MEM_FREE(data_buf);
                    INFO("EOF: expected %d bytes, got %I64u\n", sz, c);
                    return;
                }
                dbuf_write_bytes(req, data_buf, sz);
                // TODO we could optimize here: since we already have the raw data in memory,
                // we should not malloc it by copying over to dbuf.
                MEM_FREE(data_buf);
            }
        }
        if (req->sz > 0) {
            handler_handle(hd, req, resp);
            // write resp
            byte sz_buf[4];
            encode_int32(resp->sz, sz_buf);
            fwrite(sz_buf, 1, 4, stdout);
            fwrite(resp->buf, 1, resp->sz, stdout);
            fflush(stdout);
        } else {
            quit = TRUE;
        }
    }
    dbuf_free(req);
    dbuf_free(resp);
    handler_free(hd);
}

int main(int argc, char **argv) {
    for (int i=0 ; i<argc ; i++) {
        if (strcmp(argv[i], "help") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("sqinn v%s\nsqlite over stdin/out\nsee https://www.github.com/cvilsmeier/sqinn for details\n", SQINN_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "version") == 0) {
            printf("sqinn v%s", SQINN_VERSION);
            #if defined(_WIN32) || defined(WIN32)
                printf(" win32");
            #endif
            #if defined(__CYGWIN__)
                printf(" cygwin");
            #endif
            #if defined(__MINGW64__)
                printf(" minGW64");
            #else
                #if defined(__MINGW32__)
                    printf(" minGW32");
                #endif
            #endif
            #if defined(__LINUX__)
                printf(" linux");
            #endif
            #if defined(__GNUC__)
                printf(" gcc %d.%d.%d.", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
            #endif
            printf("\n");
            return 0;
        }
        if (strcmp(argv[i], "test") == 0) {            
            test_int64();
            test_dbuf();
            test_conn();
            test_handler_versions();
            test_handler_functions();
            test_handler_exec_query();
            test_handler_errors();
            bench_dbuf(2);
            bench_conn_users(2);
            bench_conn_complex(2,2,2);
            ASSERT(mem_usage()==0, "mem_usage: %d\n", mem_usage());
            return 0;
        }
        if (strcmp(argv[i], "bench") == 0) {
            bench_dbuf(100);
            bench_conn_users(100*1000);
            bench_conn_complex(100,100,10);
            ASSERT(mem_usage()==0, "mem_usage: %d\n", mem_usage());
            return 0;
        }
    }
    //setbuf(stdin, NULL);
    //setbuf(stdout, NULL);
#ifdef HAVE_SETMODE
    setmode(fileno(stdin), O_BINARY);
    setmode(fileno(stdout), O_BINARY);
#endif
    loop();
    if (mem_usage() != 0) {
        INFO("mem_usage: %d\n", mem_usage());
    }
    return 0;
}
