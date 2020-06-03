#include <time.h>
#include <stdarg.h>
#include "util.h"

void log_info(const char *file, int line, const char *fmt, ...) {
    fprintf(stderr, "INFO  %25s:%-3d ", file, line); 
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args); 
    va_end(args);
    fflush(stderr);
}

void log_debug(const char *file, int line, const char *fmt, ...) {
    fprintf(stderr, "DEBUG %25s:%-3d ", file, line); 
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args); 
    va_end(args);
    fflush(stderr);
}

void assert(const char *file, int line, bool condition, const char *fmt, ...) {
    if (condition) {
        return;
    }
    fprintf(stderr, "ASSERTION FAILED %s:%d ", file, line); 
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args); 
    va_end(args);
    fflush(stderr);
    exit(1);
}

double mono_time() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    // DEBUG("sec=%d, nsec=%d\n", (int)t.tv_sec, (int)t.tv_nsec);
    double sec = (double)t.tv_sec;
    double nsec = (double)t.tv_nsec / (double)1000000000.0;
    // DEBUG("sec=%f, nsec=%f\n", sec, nsec);
    double mt = sec + nsec;
    // DEBUG("mt=%f\n", mt);
    return mt;
}

double mono_diff_sec(double a, double b) {
    if (a < b) {
        return b-a;
    }
    return a-b;
}
