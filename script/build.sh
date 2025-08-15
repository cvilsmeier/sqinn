#!/bin/sh

set -e
stat lib/ > /dev/null  # must be in correct dir
mkdir -p bin # in case it does not exist

# compile the thing
CC="gcc"
CFLAGS="-std=c99 -Wall -Werror -O2"
if ! test -f bin/sqlite3.o; then
    $CC $CFLAGS -c lib/sqlite3.c -o bin/sqlite3.o -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_THREADSAFE=0
fi
$CC $CFLAGS -c lib/utl.c  -o bin/utl.o
$CC $CFLAGS -c lib/io.c   -o bin/io.o
$CC $CFLAGS -c lib/db.c   -o bin/db.o
$CC $CFLAGS -c lib/app.c  -o bin/app.o
$CC $CFLAGS -c lib/main.c -o bin/main.o

$CC -static \
    bin/sqlite3.o \
    bin/utl.o \
    bin/io.o \
    bin/db.o \
    bin/app.o \
    bin/main.o \
    -o bin/sqinn

