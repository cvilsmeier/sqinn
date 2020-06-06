#!/bin/sh

if [ ! -d "bin" ]; then
    mkdir bin
fi

if [ ! -f "bin/sqlite3.o" ]; then
    echo build sqlite3.o
    gcc -O1 -o bin/sqlite3.o -c src/sqlite3.c -DSQLITE_THREADSAFE=0
fi

LIBS=

case `uname -s` in 
    *Linux*)
        LIBS=-ldl
        ;;
esac

echo build sqinn
gcc $LIBS -Wall -O1 -o bin/sqinn \
    src/util.c \
    src/mem.c \
    src/mem_test.c \
    src/conn.c \
    src/conn_test.c \
    src/handler.c \
    src/handler_test.c \
    src/loop.c \
    src/main.c \
    bin/sqlite3.o

