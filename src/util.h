#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// versions

#define SQINN_VERSION "1.1.25" // sync version in README
#define IO_VERSION    1

// types

typedef unsigned char byte;
typedef long long int int64;
typedef char bool;
#define TRUE   1
#define FALSE  0

// printf int64
#if defined(_WIN32) || defined(__MINGW64__)
    #define FMT_PRId64 "I64d"
    #define FMT_PRIu64 "I64u"
#else
    #define FMT_PRId64 "lld"
    #define FMT_PRIu64 "llu"
#endif

// logging

void log_info(const char *file, int line, const char *fmt, ...);
void log_debug(const char *file, int line, const char *fmt, ...);

#define INFO(fmt, ...)  log_info(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define DEBUG(fmt, ...) log_debug(__FILE__, __LINE__, fmt, __VA_ARGS__)

// assert

void assert(const char *file, int line, bool condition, const char *fmt, ...);

#define ASSERT(c, fmt, ...) assert(__FILE__, __LINE__, !!(c), fmt, __VA_ARGS__)
#define ASSERT0(c, fmt)       assert(__FILE__, __LINE__, !!(c), fmt)

// clock

double mono_time();
double mono_since(double then);

#endif // _UTIL_H

