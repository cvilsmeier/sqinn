#!/bin/sh

set -e
stat lib/ > /dev/null  # we must be in correct directory
mkdir -p bin    # bin directory will hold all buld artefacts

# setup
CC="gcc"
CFLAGS="-std=c99 -Wall -Werror -O2"
LDFLAGS="-static"
if test "$(uname)" = "Darwin"; then
    CC="clang"
    LDFLAGS=""   # apple does not support -static
fi

# compile
if ! test -f bin/sqlite3.o; then
    $CC $CFLAGS -c lib/sqlite3.c -o bin/sqlite3.o -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_THREADSAFE=0
fi
$CC $CFLAGS -c lib/utl.c  -o bin/utl.o
$CC $CFLAGS -c lib/io.c   -o bin/io.o
$CC $CFLAGS -c lib/db.c   -o bin/db.o
$CC $CFLAGS -c lib/app.c  -o bin/app.o
$CC $CFLAGS -c lib/main.c -o bin/main.o

# link
$CC $LDFLAGS \
    bin/sqlite3.o \
    bin/utl.o \
    bin/io.o \
    bin/db.o \
    bin/app.o \
    bin/main.o \
    -o bin/sqinn

