#ifndef UTL_H
#define UTL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <inttypes.h>

#ifdef _WIN32
  // MSVC does not have unistd.h, it has io.h
  #include <io.h>
  #define STDIN_FILENO  0
  #define STDOUT_FILENO 1
  #define STDERR_FILENO 2
#else
  #include <unistd.h>
#endif

/* A BOOL is either FALSE (zero) or TRUE (not zero).*/
typedef char BOOL;

#define FALSE 0
#define TRUE  1


//
// Memory primitives: Wrap malloc() and free()
//

void initMem();
void printMem(FILE *fp);
void *memAlloc(size_t size, const char *file, int line);
void *memRealloc(void *ptr, size_t newSize);
void memFree(void *ptr);

/* hexdump writes a hexdump into a newly allocated buffer. The buffer must be memFree'd after use. */
char *hexdump(const char *data, size_t len);

/* Global memory counters. */
extern int mallocs;
extern int frees;


//
// Logging utilities
//

#define LOG_LEVEL_OFF 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_DEBUG 2

/* A Log writes log mesages. */
typedef struct log_s Log;
Log *newLog(int level, const char *filename, BOOL stdErr);  // TODO add maxFilesize parameter
void Log_free(Log *this);
int Log_level(Log *this);
void Log_print(Log *this, int level, const char *fmt, ...);

/* The (one and only) global Log instance. */
extern Log *theLog;

#define LOG_CAN_INFO (Log_level(theLog) >= LOG_LEVEL_INFO)

#define LOG_CAN_DEBUG (Log_level(theLog) >= LOG_LEVEL_DEBUG)

#define LOG_INFO0(msg)         Log_print(theLog, LOG_LEVEL_INFO, (msg))
#define LOG_INFO1(msg,a)       Log_print(theLog, LOG_LEVEL_INFO, (msg), (a))
#define LOG_INFO2(msg,a,b)     Log_print(theLog, LOG_LEVEL_INFO, (msg), (a), (b))
#define LOG_INFO3(msg,a,b,c)   Log_print(theLog, LOG_LEVEL_INFO, (msg), (a), (b), (c))
#define LOG_INFO4(msg,a,b,c,d) Log_print(theLog, LOG_LEVEL_INFO, (msg), (a), (b), (c), (d))

#define LOG_DEBUG0(msg)          Log_print(theLog, LOG_LEVEL_DEBUG, (msg))
#define LOG_DEBUG1(msg,a)        Log_print(theLog, LOG_LEVEL_DEBUG, (msg), (a))
#define LOG_DEBUG2(msg,a,b)      Log_print(theLog, LOG_LEVEL_DEBUG, (msg), (a), (b))
#define LOG_DEBUG3(msg,a,b,c)    Log_print(theLog, LOG_LEVEL_DEBUG, (msg), (a), (b), (c))
#define LOG_DEBUG4(msg,a,b,c,d)  Log_print(theLog, LOG_LEVEL_DEBUG, (msg), (a), (b), (c), (d))


//
// ASSERT macros
//

#define ASSERT(condition) if(!(condition)) { \
    Log_print(theLog, LOG_LEVEL_INFO, "%s:%d ASSERT FAIL: " #condition ""  , __FILE__, __LINE__); \
                      fprintf(stderr, "%s:%d ASSERT FAIL: " #condition "\n", __FILE__, __LINE__); \
    exit(1); \
}

#define ASSERTF(condition, fmt, ...) if(!(condition)) { \
    Log_print(theLog, LOG_LEVEL_INFO, "%s:%d ASSERT FAIL: " #condition ": " #fmt ""  , __FILE__, __LINE__, ##__VA_ARGS__); \
                      fprintf(stderr, "%s:%d ASSERT FAIL: " #condition ": " #fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    exit(1); \
}

#define ASSERT_FAIL(fmt, ...) { \
    Log_print(theLog, LOG_LEVEL_INFO, "%s:%d ASSERT FAIL: " #fmt ""  , __FILE__, __LINE__, ##__VA_ARGS__); \
                      fprintf(stderr, "%s:%d ASSERT FAIL: " #fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    exit(1); \
}

#define ASSERT_INT(want, have) { \
    int w = (want); \
    int h = (have); \
    if(w != h) { \
        Log_print(theLog, LOG_LEVEL_INFO, "%s:%d ASSERT FAIL: want %d but have %d"  , __FILE__, __LINE__, w, h); \
                          fprintf(stderr, "%s:%d ASSERT FAIL: want %d but have %d\n", __FILE__, __LINE__, w, h); \
        exit(1); \
    } \
}

#define ASSERT_INT64(want, have) { \
    int64_t w = (want); \
    int64_t h = (have); \
    if(w != h) { \
        Log_print(theLog, LOG_LEVEL_INFO, "%s:%d ASSERT FAIL: want %" PRId64 " but have %" PRId64 ""  , __FILE__, __LINE__, w, h); \
                          fprintf(stderr, "%s:%d ASSERT FAIL: want %" PRId64 " but have %" PRId64 "\n", __FILE__, __LINE__, w, h); \
        exit(1); \
    } \
}

#define ASSERT_DOUBLE(want, have) { \
    double w = (want); \
    double h = (have); \
    if(w != h) { \
        Log_print(theLog, LOG_LEVEL_INFO, "%s:%d ASSERT FAIL: want %f but have %f"  , __FILE__, __LINE__, w, h); \
                          fprintf(stderr, "%s:%d ASSERT FAIL: want %f but have %f\n", __FILE__, __LINE__, w, h); \
        exit(1); \
    } \
}

#define ASSERT_STR(want, have) { \
    const char* w = (want); \
    const char* h = (have); \
    if(strcmp(w,h) != 0) { \
        Log_print(theLog, LOG_LEVEL_INFO, "%s:%d ASSERT FAIL: want '%s' but have '%s'"  , __FILE__, __LINE__, w, h); \
                          fprintf(stderr, "%s:%d ASSERT FAIL: want '%s' but have '%s'\n", __FILE__, __LINE__, w, h); \
        exit(1); \
    } \
}

#endif  // UTL_H