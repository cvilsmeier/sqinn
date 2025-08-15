#include "utl.h"

#define MAX_BLOCKS 0  // set 0 to disable memory tracking, set 8*1024 (e.g.) for debugging memory issues
#define MAX_FILE_LEN 64

typedef struct block_s {
    char file[MAX_FILE_LEN+1];
    int line;
    void *ptr;
    BOOL free;
} block_s;

block_s blocks[MAX_BLOCKS+1];
int nblocks = 0;
int mallocs = 0;
int frees = 0;

void initMem() {
    memset(blocks, 0, sizeof(blocks));
    nblocks = 0;
    mallocs = 0;
    frees = 0;
}

void printMem(FILE *fp) {
    fprintf(fp, "%d mallocs, %d frees, %d in use\n", mallocs, frees, mallocs-frees);
    if(MAX_BLOCKS) {
        fprintf(fp, "%d blocks\n", nblocks);
        for (int i=0 ; i<nblocks ; i++) {
            block_s b = blocks[i];
            fprintf(fp, "block %3d: %16s:%-4d %4s\n", i, b.file, b.line, b.free ? "" : "leak");
        }
    }
}

void *memAlloc(size_t size, const char *file, int line) {
    mallocs ++;
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "fatal: out of memory");
        exit(1);
    }
    if(MAX_BLOCKS) {
        if(nblocks < MAX_BLOCKS) {
            strncpy(blocks[nblocks].file, file, MAX_FILE_LEN);
            blocks[nblocks].line = line;
            blocks[nblocks].ptr = ptr;
            blocks[nblocks].free = FALSE;
            nblocks++;
        }
    }
    return ptr;
}

void *memRealloc(void *ptr, size_t newSize) {
    void *newPtr = realloc(ptr, newSize);
    if (!newPtr) {
        fprintf(stderr, "fatal: out of memory");
        exit(1);
    }
    if(MAX_BLOCKS) {
        for (int i=0 ; i<nblocks ; i++) {
            if (blocks[i].ptr == ptr ) {
                blocks[i].ptr = newPtr;
            }
        }
    }
    return newPtr;
}

void memFree(void *ptr) {
    if (!ptr) {
        return;
    }
    frees ++;
    free(ptr);
    if(MAX_BLOCKS) {
        for (int i=0 ; i<nblocks ; i++) {
            if (blocks[i].ptr == ptr ) {
                blocks[i].free = TRUE;
            }
        }
    }
}

char *hexdump(const char *data, size_t len) {
    if(!data) {
        char *buf = (char*)memAlloc(8, __FILE__, __LINE__);
        strcpy(buf, "NULL");
        return buf;
    }
    if (len > 1024 ) {
        len = 1024;
    }
    char *buf = (char*)memAlloc(4*len, __FILE__, __LINE__);
    buf[0] = 0;
    char tmp[4];
    for (size_t i=0 ; i<len ; i++) {
        snprintf(tmp, 4, "%02X ", (unsigned char)data[i]);
        strcat(buf, tmp);
    }
    size_t ln = strlen(buf);
    if (buf[ln-1] == ' ') {
        buf[ln-1] = 0;
    }
    return buf;    
}


static void _vfprintf(FILE *fp, int level, const char *fmt, va_list args) {
    time_t now = time(NULL);
    struct tm *pt = localtime(&now);
    char tstamp[64];
    strftime(tstamp, sizeof(tstamp), "%Y-%m-%d %H:%M:%S", pt);
    fprintf(fp, "%s ", tstamp);
    switch (level) {
        case LOG_LEVEL_INFO:
            fprintf(fp, "INFO  ");
            break;
        case LOG_LEVEL_DEBUG:
            fprintf(fp, "DEBUG ");
            break;
        default:
            fprintf(fp, "LEVEL%d ", level);
            break;
    }
    vfprintf(fp, fmt, args);
    fprintf(fp, "\n");
    fflush(fp);
}

struct log_s {
    int level;  // 0, 1, ...
    FILE *fp;   // NULL for no file output
    BOOL stdErr;
};

Log *newLog(int level, const char *filename, BOOL stdErr) {
    Log *this = (Log *)memAlloc(sizeof(Log), __FILE__, __LINE__);
    this->level = level;
    this->fp = NULL;
    this->stdErr = stdErr;
    if (strlen(filename) > 0) {
        this->fp = fopen(filename, "a");
        if (!this->fp) {
            fprintf(stderr, "cannot open logfile\n");
        }
    }
    return this;
}

void Log_free(Log *this) {
    if (!this) {
        return;
    }
    if (this->fp) {
        fclose(this->fp);
    }
    if (this->stdErr) {
        fflush(stderr);
    }    
    memFree(this);
}

int Log_level(Log * this) {
    if (!this) {
        return LOG_LEVEL_OFF;
    }
    return this->level;
}

void Log_print(Log *this, int level, const char *fmt, ...) {
    if (!this) {
        return;
    }
    if (this->level < level) {
        return;
    }
    if (this->fp) {
        va_list args;
        va_start(args, fmt);
        _vfprintf(this->fp, level, fmt, args);
        va_end(args);
        fflush(this->fp);
    }
    if (this->stdErr) {
        va_list args;
        va_start(args, fmt);
        _vfprintf(stderr, level, fmt, args);
        va_end(args);
        fflush(stderr);
    }
}

Log *theLog = NULL;
