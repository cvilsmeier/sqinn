#!/bin/sh

if [ ! -f "src/main.c" ]; then
    echo src/main.c not found - cannot build
    exit 1
fi

mkdir -p bin

if [ ! -f "bin/sqlite3.o" ]; then
    echo build sqlite3.o
    gcc -std=c99 -O1 -o bin/sqlite3.o -c src/sqlite3.c -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION
fi

FLAGS="-std=c99 -Wall -O1"
case `uname -s` in 
    *Linux*)
        FLAGS="$FLAGS -static"
        ;;
    *MINGW*)
        FLAGS="$FLAGS -static"
        ;;
    
esac

echo "build sqinn using FLAGS $FLAGS"
gcc $FLAGS -o bin/sqinn \
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

mkdir -p dist

cp bin/sqinn dist/

