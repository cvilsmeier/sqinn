#include <fcntl.h>
#include <limits.h>
#include "util.h"
#include "mem.h"
#include "mem_test.h"
#include "conn.h"
#include "conn_test.h"
#include "handler.h"
#include "handler_test.h"
#include "loop.h"

void test_platform() {
    INFO("TEST %s\n", __func__);
    DEBUG("  sizeof(int)=%d, sizeof(size_t)=%d, sizeof(int64)=%d, PRIu64=%s, PRId64=%s\n", sizeof(int), sizeof(size_t), sizeof(int64), PRIu64, PRId64);
    ASSERT(sizeof(int)==4, "expected int to be 4 bytes but was %d", sizeof(size_t));
    ASSERT(sizeof(size_t)==8 || sizeof(size_t)==4, "expected size_t to be 8 or 4 bytes but was %"PRIu64, sizeof(size_t));
    ASSERT(sizeof(int64)==8, "expected int64 to be 8 bytes but was %"PRIu64, sizeof(int64));
    ASSERT(sizeof(double)==8, "expected double to be 8 bytes but was %"PRIu64, sizeof(double));
    double dbl = -2; // in IEEE 745 it's hex(C000 0000 0000 0000)
    byte dbl0 = ((char*)&dbl)[0];
    byte dbl7 = ((char*)&dbl)[7];
    ASSERT(dbl0== 0x00, "expected double -2 [0] 0x00 but was %02X", dbl0);
    ASSERT(dbl7== 0xC0, "expected double -2 [7] 0xC0 but was %02X", dbl7);
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
    ASSERT(strcmp(buf, "9223372036854775807")==0, "expected x to be '9223372036854775807' but was '%s'", buf);
    INFO("TEST %s OK\n", __func__);
}

int main(int argc, char **argv) {
    char dbfile[128];
    strcpy(dbfile, ":memory:");
    for (int i=0 ; i<argc ; i++) {
        if (strcmp(argv[i], "help") == 0 || strcmp(argv[i], "--help") == 0) {
            printf(
                "Sqinn is SQLite over stdin/stdout\n"
                "\n"
                "Usage:\n"
                "\n"
                "       sqinn [options...] [command] \n"
                "\n"
                "Commands are:\n"
                "\n"
                "        help            show this help page\n"
                "        version         print Sqinn version\n"
                "        sqlite_version  print SQLite library version\n"
                "        test            execute built-in unit tests\n"
                "        bench           execute built-in benchmarks\n"
                "\n"
                "Options are:\n"
                "\n"
                "        -db             db file, used for test and bench\n"
                "                        commands. Default is \":memory:\"\n"
                "\n"
                "When invoked without a command, Sqinn will read (await) requests\n"
                "from stdin, print responses to stdout and output error messages\n"
                "on stderr.\n"
                "\n"
                "For more details see https://www.github.com/cvilsmeier/sqinn\n");
            return 0;
        } else if (strcmp(argv[i], "version") == 0) {
            printf("sqinn v%s\n", SQINN_VERSION);
            return 0;
        } else if (strcmp(argv[i], "sqlite_version") == 0) {
            printf("sqlite v%s\n", lib_version());
            return 0;
        } else if (strcmp(argv[i], "test") == 0) {
            test_platform();
            test_dbuf();
            test_dbuf_double();
            test_conn(dbfile);
            test_handler_versions();
            test_handler_functions(dbfile);
            test_handler_exec_query(dbfile);
            test_handler_exec_failure(dbfile);
            test_handler_errors();
            bench_dbuf(2);
            bench_conn_users(dbfile, 2);
            bench_conn_complex(dbfile, 2,2,2);
            ASSERT(mem_usage()==0, "mem_usage: %d\n", mem_usage());
            printf("All Tests OK\n");
            return 0;
        } else if (strcmp(argv[i], "bench") == 0) {
            bench_dbuf(100);
            bench_conn_users(dbfile, 1000*1000);
            bench_conn_complex(dbfile, 200,100,10);
            ASSERT(mem_usage()==0, "mem_usage: %d\n", mem_usage());
            return 0;
        } else if (strcmp(argv[i], "-db") == 0) {
            strncpy(dbfile, argv[i+1], sizeof(dbfile));
        }
    }
#if defined(_WIN32) || defined(__MINGW64__)
    setmode(fileno(stdin), O_BINARY);
    setmode(fileno(stdout), O_BINARY);
#endif
    loop();
    if (mem_usage() != 0) {
        INFO("mem_usage: %d\n", mem_usage());
    }
    return 0;
}
