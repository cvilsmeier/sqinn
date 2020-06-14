#include <time.h>
#include <stdarg.h>
#include "util.h"

void log_info(const char *file, int line, const char *fmt, ...) {
    // fprintf(stderr, "INFO  %25s:%-3d ", file, line); 
    fprintf(stderr, "INFO  [%s:%d] ", file, line); 
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args); 
    va_end(args);
    fflush(stderr);
}

void log_debug(const char *file, int line, const char *fmt, ...) {
    // fprintf(stderr, "DEBUG %25s:%-3d ", file, line); 
    fprintf(stderr, "DEBUG [%s:%d] ", file, line); 
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
    clock_t now = clock();
    double sec = ((double)now) / CLOCKS_PER_SEC;
    return sec;
}

double mono_since(double then) {
    return mono_time() - then;
}
