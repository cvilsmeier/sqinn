#ifndef IO_H
#define IO_H

typedef struct reader_s Reader;
Reader *newStdinReader();
Reader *newMemReader(char *buf, size_t bufsz);
void Reader_free(Reader* this);
char Reader_readByte(Reader* this);
int Reader_readInt32(Reader* this);
int64_t Reader_readInt64(Reader* this);
double Reader_readDouble(Reader* this);
const char* Reader_readString(Reader* this);
const char* Reader_readBlob(Reader* this, size_t *plen);

typedef struct writer_s Writer;
Writer *newStdoutWriter();
Writer *newMemWriter(char *buf, size_t bufsz);
void Writer_free(Writer* this);
void Writer_markFrame(Writer* this);
void Writer_flush(Writer* this);
void Writer_writeByte(Writer* this, char value);
void Writer_writeInt32(Writer* this, int value);
void Writer_writeInt64(Writer* this, int64_t value);
void Writer_writeDouble(Writer* this, double value);
void Writer_writeString(Writer* this, const char* str);
void Writer_writeBlob(Writer* this, const char* data, size_t len);

void testIo();

#endif // IO_H
