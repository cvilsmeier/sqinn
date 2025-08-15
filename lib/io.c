#include "utl.h"
#include "io.h"

#define MAX_LEN 0x7FFFFFFF

void _readStdin(char *buf, size_t len) {
    size_t c = 0;
    while(c < len) {
        size_t n = read(STDIN_FILENO, buf+c, len-c);
        if (n<=0) {
            ASSERT_FAIL("_readStdin: n=%zd", n);
        }
        c += n;
    }
    ASSERT(c == len);
}

void _writeStdout(char *buf, size_t len) {
    size_t c = 0;
    while(c < len) {
        size_t n = write(STDOUT_FILENO, buf+c, len-c);
        if (n<=0) {
            ASSERT_FAIL("_writeStdout: n=%zd", n);
        }
        c += n;
    }
    ASSERT(c == len);
}

// class Reader

struct reader_s {
    // for stdin reader
    BOOL std;
    char* buf;
    size_t bufsz;
    size_t rp; // read pointer
};

Reader *newStdinReader(){
    Reader* this = (Reader*)memAlloc(sizeof(Reader), __FILE__, __LINE__);
    this->std = TRUE;
    this->buf = NULL;
    this->bufsz = 0;
    this->rp = 0;
    return this;
}

Reader *newMemReader(char *buf, size_t bufsz) {
    ASSERT(buf);
    ASSERT(bufsz);
    Reader* this = (Reader*)memAlloc(sizeof(Reader), __FILE__, __LINE__);
    this->std = FALSE;
    this->buf = buf;
    this->bufsz = bufsz;
    this->rp = 0;
    return this;
}

void Reader_free(Reader* this) {
    if(this->std && this->buf) {
        memFree(this->buf);
    }
    memFree(this);
}

void _readNextFrameIfNeeded(Reader* this) {
    ASSERT(this->std);
    ASSERT(this->rp <= this->bufsz);
    if(this->rp == this->bufsz) {
        char tmp[4];
        _readStdin(tmp, 4);
        size_t len0 = (size_t)(unsigned char)tmp[0] << 24;
        size_t len1 = (size_t)(unsigned char)tmp[1] << 16;
        size_t len2 = (size_t)(unsigned char)tmp[2] <<  8;
        size_t len3 = (size_t)(unsigned char)tmp[3] <<  0;
        size_t len = len0 + len1 + len2 + len3;
        ASSERT(1 <= len && len <= MAX_LEN);
        if (this->buf) {
            this->buf = memRealloc(this->buf, len);
        } else {
            this->buf = memAlloc(len, __FILE__, __LINE__);
        }
        _readStdin(this->buf, len);
        this->bufsz = len;
        this->rp = 0;
        if (LOG_CAN_DEBUG) {
            char * hx = hexdump(this->buf, this->bufsz);
            LOG_DEBUG2("_readNextFrameIfNeeded: %d bytes: %s", this->bufsz, hx);
            memFree(hx);
        }
    }
}

char Reader_readByte(Reader* this) {
    if(this->std) {
        _readNextFrameIfNeeded(this);
    }
    ASSERT(this->bufsz - this->rp >= 1);
    char v = this->buf[this->rp];
    this->rp += 1;
    return v;
}

int Reader_readInt32(Reader* this) {
    if(this->std) {
        _readNextFrameIfNeeded(this);
    }
    ASSERT(this->bufsz - this->rp >= 4);
    uint32_t v0 = (uint32_t)(unsigned char)this->buf[this->rp + 0] << 24;
    uint32_t v1 = (uint32_t)(unsigned char)this->buf[this->rp + 1] << 16;
    uint32_t v2 = (uint32_t)(unsigned char)this->buf[this->rp + 2] <<  8;
    uint32_t v3 = (uint32_t)(unsigned char)this->buf[this->rp + 3] <<  0;
    this->rp += 4;
    return (int)(v0 + v1 + v2 + v3);
}

int64_t Reader_readInt64(Reader* this) {
    if(this->std) {
        _readNextFrameIfNeeded(this);
    }
    ASSERT(this->bufsz - this->rp >= 8);
    uint64_t v0 = (uint64_t)(unsigned char)this->buf[this->rp + 0] << 56;
    uint64_t v1 = (uint64_t)(unsigned char)this->buf[this->rp + 1] << 48;
    uint64_t v2 = (uint64_t)(unsigned char)this->buf[this->rp + 2] << 40;
    uint64_t v3 = (uint64_t)(unsigned char)this->buf[this->rp + 3] << 32;
    uint64_t v4 = (uint64_t)(unsigned char)this->buf[this->rp + 4] << 24;
    uint64_t v5 = (uint64_t)(unsigned char)this->buf[this->rp + 5] << 16;
    uint64_t v6 = (uint64_t)(unsigned char)this->buf[this->rp + 6] << 8;
    uint64_t v7 = (uint64_t)(unsigned char)this->buf[this->rp + 7];
    this->rp += 8;
    return (int64_t)(v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7);
}

double Reader_readDouble(Reader* this) {
    if(this->std) {
        _readNextFrameIfNeeded(this);
    }
    ASSERT(this->bufsz - this->rp >= 8);
    double v = 0.0;
    char *p = (char *)(&v);
    p[7] = this->buf[this->rp + 0];
    p[6] = this->buf[this->rp + 1];
    p[5] = this->buf[this->rp + 2];
    p[4] = this->buf[this->rp + 3];
    p[3] = this->buf[this->rp + 4];
    p[2] = this->buf[this->rp + 5];
    p[1] = this->buf[this->rp + 6];
    p[0] = this->buf[this->rp + 7];
    this->rp += 8;
    return v;
}

const char* Reader_readString(Reader* this) {
    size_t len = 1;
    const char *p = Reader_readBlob(this, &len);
    ASSERTF(p[len-1] == '\0', "Reader_readString: string is not null-terminated");
    return p;
}

const char* Reader_readBlob(Reader* this, size_t *plen) {
    if(this->std) {
        _readNextFrameIfNeeded(this);
    }
    size_t len = Reader_readInt32(this);
    ASSERT(len <= 0x7FFFFFFF);
    ASSERT(this->bufsz - this->rp >= len);
    *plen = len;
    const char *p = this->buf + this->rp;
    this->rp += len;
    return p;
}

// class Writer

struct writer_s {
    BOOL std;
    char* buf;
    size_t bufsz;
    size_t wp; // write pointer
};

void _validateWriter(Writer *this) {
    ASSERT(this);
    ASSERT(this->buf);
    ASSERT(this->bufsz);
    ASSERT(this->wp <= this->bufsz);
}

Writer *newStdoutWriter() {
    Writer *this = (Writer *)memAlloc(sizeof(Writer), __FILE__, __LINE__);
    this->std = TRUE;
    this->bufsz = 1024*1024;
    this->buf = memAlloc(this->bufsz, __FILE__, __LINE__);
    this->wp = 0;
    _validateWriter(this);
    return this;
}

Writer *newMemWriter(char *buf, size_t bufsz) {
    Writer *this = (Writer *)memAlloc(sizeof(Writer), __FILE__, __LINE__);
    this->std = FALSE;
    this->buf = buf;
    this->bufsz = bufsz;
    this->wp = 0;
    _validateWriter(this);
    return this;
}

void Writer_free(Writer* this) {
    _validateWriter(this);
    if (this->std) {
        memFree(this->buf);
    }
    memFree(this);
}

static void _growWriter(Writer* this, size_t minSize) {
    LOG_DEBUG2("_growWriter: this->bufsz %ld, minSize %ld", this->bufsz, minSize);
    if (this->bufsz < minSize) {
        size_t newSize = this->bufsz;
        while( newSize < minSize ) {
            newSize = 2 * newSize;
        }
        LOG_DEBUG1("_growWriter: newSize %ld ", newSize);
        if (newSize > MAX_LEN) {
            ASSERT_FAIL("_growWriter: newSize %zd > MAX_LEN", newSize)
        }   
        this->buf = memRealloc(this->buf, newSize);
        this->bufsz = newSize;
    }    
}

void Writer_markFrame(Writer* this) {
    _validateWriter(this);
    if (this->std && this->wp > 1024*1024) {
        Writer_flush(this);
    }
}

void Writer_flush(Writer* this) {
    _validateWriter(this);
    if (this->std && this->wp) {
        if (LOG_CAN_DEBUG) {
            char * hx = hexdump(this->buf, this->wp);
            LOG_DEBUG2("Writer_flush: %d bytes: %s", this->wp, hx);
            memFree(hx);
        }
        char tmp[4];
        tmp[0] = (char)(this->wp >> 24);
        tmp[1] = (char)(this->wp >> 16);
        tmp[2] = (char)(this->wp >> 8);
        tmp[3] = (char)(this->wp);
        _writeStdout(tmp, 4);
        _writeStdout(this->buf, this->wp);
        this->wp = 0;
    }
}


void Writer_writeByte(Writer* this, char value) {
    _validateWriter(this);
    if (this->std) {
        _growWriter(this, this->wp + 1);
    }
    ASSERT(this->bufsz - this->wp >= 1);
    this->buf[this->wp] = value;
    this->wp += 1;
}

void Writer_writeInt32(Writer* this, int value) {
    _validateWriter(this);
    if (this->std) {
        _growWriter(this, this->wp + 4);
    }
    ASSERT(this->bufsz - this->wp >= 4);
    this->buf[this->wp + 0] = (char)(value >> 24);
    this->buf[this->wp + 1] = (char)(value >> 16);
    this->buf[this->wp + 2] = (char)(value >> 8);
    this->buf[this->wp + 3] = (char)(value);
    this->wp += 4;
}

void Writer_writeInt64(Writer* this, int64_t value) {
    _validateWriter(this);
    if (this->std) {
        _growWriter(this, this->wp + 8);
    }
    ASSERTF(this->bufsz - this->wp >= 8, "this->bufsz %zd - this->wp %zd = %zd", this->bufsz, this->wp, this->bufsz - this->wp);
    this->buf[this->wp + 0] = (char)(value >> 56);
    this->buf[this->wp + 1] = (char)(value >> 48);
    this->buf[this->wp + 2] = (char)(value >> 40);
    this->buf[this->wp + 3] = (char)(value >> 32);
    this->buf[this->wp + 4] = (char)(value >> 24);
    this->buf[this->wp + 5] = (char)(value >> 16);
    this->buf[this->wp + 6] = (char)(value >> 8);
    this->buf[this->wp + 7] = (char)(value);
    this->wp += 8;
}

void Writer_writeDouble(Writer* this, double value) {
    _validateWriter(this);
    if (this->std) {
        _growWriter(this, this->wp + 8);
    }
    ASSERT(this->bufsz - this->wp >= 8);
    char* p = (char*)(&value);
    this->buf[this->wp + 0] = p[7];
    this->buf[this->wp + 1] = p[6];
    this->buf[this->wp + 2] = p[5];
    this->buf[this->wp + 3] = p[4];
    this->buf[this->wp + 4] = p[3];
    this->buf[this->wp + 5] = p[2];
    this->buf[this->wp + 6] = p[1];
    this->buf[this->wp + 7] = p[0];
    this->wp += 8;
}

void Writer_writeString(Writer* this, const char* str) {
    _validateWriter(this);
    size_t len = strlen(str);
    Writer_writeBlob(this, str, len + 1);
}

void Writer_writeBlob(Writer* this, const char* data, size_t len) {
    ASSERT(data);
    ASSERT(len < MAX_LEN);
    _validateWriter(this);
    if (this->std) {
        _growWriter(this, this->wp + 4 + len);
    }
    Writer_writeInt32(this, (int)len);
    ASSERT(this->bufsz - this->wp >= len);
    memcpy(this->buf + this->wp, data, len);
    this->wp += len;
}

//
// Test
//

static void testWriteAndRead() {
    char buf[256];
    // write
    Writer *w = newMemWriter(buf, sizeof(buf));
    Writer_writeByte(w, 0);
    Writer_writeByte(w, 42);
    Writer_writeByte(w, 255);                  // same as -1
    Writer_writeInt32(w, 0x10203040);          // 4 byte
    Writer_writeInt32(w, 0xF0E0D0C0);          // 4 byte
    Writer_writeInt64(w, 0x1020304050607080);  // 8 byte
    Writer_writeInt64(w, 0xF0E0D0C0B0A09080);  // 8 byte
    Writer_writeDouble(w, 128.5);              // 8 byte // double 128.5 = hex(40 60 10 00 00 00 00 00)
    Writer_writeString(w, "Alice");            // 4 byte + 5 data + 1 null-termination
    Writer_writeBlob(w, "12345678", 8);            // 4 byte + 8 byte data
    // check data
    int i=0;
    // Writer_writeByte(w, 0);
    ASSERT_INT(0, (unsigned char)buf[i++]);
    // Writer_writeByte(w, 42);
    ASSERT_INT(42, (unsigned char)buf[i++]);
    // Writer_writeByte(w, 255);                  // same as -1
    ASSERT_INT(255, (unsigned char)buf[i++]);
    // Writer_writeInt32(w, 0x10203040);          // 4 byte
    ASSERT_INT(0x10, (unsigned char)buf[i++]);
    ASSERT_INT(0x20, (unsigned char)buf[i++]);
    ASSERT_INT(0x30, (unsigned char)buf[i++]);
    ASSERT_INT(0x40, (unsigned char)buf[i++]);
    // Writer_writeInt32(w, 0xF0E0D0C0);          // 4 byte
    ASSERT_INT(0xF0, (unsigned char)buf[i++]);
    ASSERT_INT(0xE0, (unsigned char)buf[i++]);
    ASSERT_INT(0xD0, (unsigned char)buf[i++]);
    ASSERT_INT(0xC0, (unsigned char)buf[i++]);
    // Writer_writeInt64(w, 0x1020304050607080);  // 8 byte
    ASSERT_INT(0x10, (unsigned char)buf[i++]);
    ASSERT_INT(0x20, (unsigned char)buf[i++]);
    ASSERT_INT(0x30, (unsigned char)buf[i++]);
    ASSERT_INT(0x40, (unsigned char)buf[i++]);
    ASSERT_INT(0x50, (unsigned char)buf[i++]);
    ASSERT_INT(0x60, (unsigned char)buf[i++]);
    ASSERT_INT(0x70, (unsigned char)buf[i++]);
    ASSERT_INT(0x80, (unsigned char)buf[i++]);
    // Writer_writeInt64(w, 0xF0E0D0C0B0A09080);  // 8 byte
    ASSERT_INT(0xF0, (unsigned char)buf[i++]);
    ASSERT_INT(0xE0, (unsigned char)buf[i++]);
    ASSERT_INT(0xD0, (unsigned char)buf[i++]);
    ASSERT_INT(0xC0, (unsigned char)buf[i++]);
    ASSERT_INT(0xB0, (unsigned char)buf[i++]);
    ASSERT_INT(0xA0, (unsigned char)buf[i++]);
    ASSERT_INT(0x90, (unsigned char)buf[i++]);
    ASSERT_INT(0x80, (unsigned char)buf[i++]);
    // Writer_writeDouble(w, 128.5);              // 8 byte // double 128.5 = hex(40 60 10 00 00 00 00 00)
    ASSERT_INT(0x40, (unsigned char)buf[i++]);
    ASSERT_INT(0x60, (unsigned char)buf[i++]);
    ASSERT_INT(0x10, (unsigned char)buf[i++]);
    ASSERT_INT(0x00, (unsigned char)buf[i++]);
    ASSERT_INT(0x00, (unsigned char)buf[i++]);
    ASSERT_INT(0x00, (unsigned char)buf[i++]);
    ASSERT_INT(0x00, (unsigned char)buf[i++]);
    ASSERT_INT(0x00, (unsigned char)buf[i++]);
    // Writer_writeString(w, "Alice");            // 4 byte + 5 data + 1 null-termination
    ASSERT_INT(0x00, (unsigned char)buf[i++]);
    ASSERT_INT(0x00, (unsigned char)buf[i++]);
    ASSERT_INT(0x00, (unsigned char)buf[i++]);
    ASSERT_INT(0x06, (unsigned char)buf[i++]);
    ASSERT_INT('A', (unsigned char)buf[i++]);
    ASSERT_INT('l', (unsigned char)buf[i++]);
    ASSERT_INT('i', (unsigned char)buf[i++]);
    ASSERT_INT('c', (unsigned char)buf[i++]);
    ASSERT_INT('e', (unsigned char)buf[i++]);
    ASSERT_INT('\0', (unsigned char)buf[i++]);
    // Writer_writeBlob(w, "12345678", 8);            // 4 byte + 8 byte data
    ASSERT_INT(0x00, (unsigned char)buf[i++]);
    ASSERT_INT(0x00, (unsigned char)buf[i++]);
    ASSERT_INT(0x00, (unsigned char)buf[i++]);
    ASSERT_INT(0x08, (unsigned char)buf[i++]);
    ASSERT_INT('1', (unsigned char)buf[i++]);
    ASSERT_INT('2', (unsigned char)buf[i++]);
    ASSERT_INT('3', (unsigned char)buf[i++]);
    ASSERT_INT('4', (unsigned char)buf[i++]);
    ASSERT_INT('5', (unsigned char)buf[i++]);
    ASSERT_INT('6', (unsigned char)buf[i++]);
    ASSERT_INT('7', (unsigned char)buf[i++]);
    ASSERT_INT('8', (unsigned char)buf[i++]);
    ASSERT_INT(57,i);
    // read
    Reader *r = newMemReader(buf, sizeof(buf));
    ASSERT_INT(0, Reader_readByte(r));
    ASSERT_INT(42, Reader_readByte(r));
    ASSERT_INT(-1, Reader_readByte(r));
    ASSERT_INT(0x10203040, Reader_readInt32(r));
    ASSERT_INT(-253701952, Reader_readInt32(r));
    ASSERT_INT(-253701952, 0xF0E0D0C0);
    ASSERT_INT64(0x1020304050607080, Reader_readInt64(r));
    ASSERT_INT64(-1089641583808049024, Reader_readInt64(r));
    ASSERT_INT64(-1089641583808049024, 0xF0E0D0C0B0A09080);
    ASSERT_DOUBLE(128.5, Reader_readDouble(r));
    const char *str = Reader_readString(r); // no need to free
    ASSERT_STR("Alice", str);
    // free
    Reader_free(r);
    Writer_free(w);
}

void testIo() {
    LOG_INFO0("testIo testWriteAndRead");
    testWriteAndRead();
}
