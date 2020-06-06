SQINN
===============================================================================

Sqinn is an alternative to the SQLite C API. Sqinn reads requests from stdin,
forwards the request to SQLite, and writes a response to stdout. It is used in
programming environments that do not allow to call C API functions directly.

    !!!

    Not production-ready. 

    Preliminary version, everything may change.

    !!!

[SQLite](https://www.sqlite.org/index.html) is written in C and provides a API
for using it in C/C++. There are many language bindings. If you cannot or do
not want to use one of the available language bindings, and your programming
language allows the creation of subprocesses (fork/exec), an option might be
to communicate with SQLite over stdin/stdout, using Sqinn.

Sqinn provides functions that map to SQLite functions, like `sqlite3_open()`,
`sqlite3_prepare()`, `sqlite3_column()`, and so on. If you have not read the
[Introduction to the SQLite C/C++
Interface](https://www.sqlite.org/cintro.html), now it's a good time. It's a
5-minute read and shows the basic workings of SQLite. All of the functions
described that document are provided by Sqinn.

Marshalling every function call and every result value back and forth between
process boundaries if, of course, horribly slow. To gain performance, Sqinn
provides functions that lets you call multiple SQLite functions in one
request/response cycle.

The provided function calls and the binary protocol used for marshalling
request and response data is described in
[binary\_protocol.md](binary_protocol.md).


Compiling with gcc
-------------------------------------------------------------------------------

See the included `build.sh` script for compiling Sqinn with `gcc`. I have
tested it on Windows 10 (64bit) using Mingw and Cygwin, and Debian Linux 10.3
amd64. Alternatively, you can use the pre-built binaries from the releases
page
[github.com/cvilsmeier/sqinn/releases](https://github.com/cvilsmeier/sqinn/releases).


Performance
-------------------------------------------------------------------------------

Marshalling request and response data over stdin/stdout takes time.
Therefore, reading and writing a SQLite database with Sqinn is considerably
slower than accessing the database file directly with SQLite C API calls.

The performance loss depends to a great degree on the amount of work the
database has to do to execute a sql statement. For simple statements that do
not impose a lot of work on the database, the marshalling overhead becomes
significat. For complex queries that take a long time to execute, the
marshalling overhead becomes negligible.

Here are some benchmarks using Sqinn with SQLite library version 3.32.1
(2020-05-25), compiled on Windows 10, Mingw64, with TDM-GCC-64.

- OS: Windows 10 Home x64 Version 1909 Build 18363
- CPU: Intel(R) Core(TM) i7-6700HQ CPU @ 2.60GHz, 2592 MHz, 4 Cores
- RAM: 16GB
- Disk: 256GB SSD

The driver program that's invoking Sqinn is written in Go 1.14, also on
Windows 10 x64.


### Benchmark 1

The first benchmark inserts 1 million rows into a table, and queries them
afterwards. The table schema is as follows:

    CREATE TABLE users (
        id     INTEGER PRIMARY KEY NOT NULL,
        name   VARCHAR,
        age    INTEGER,
        rating REAL
    );

The database is disk-based to simulate the real world. The database is
initially empty. All inserts take place in a single transaction. The results
are (lower numbers are better):

    +----------------+---------+---------+
    |                | insert  | query   |
    +----------------+---------+---------+
    | SQLite C API   | 1.2 s   | 0.2 s   |
    | Sqinn          | 2.6 s   | 2.4 s   |
    +----------------+---------+---------+

Inserting 1 million rows takes twice as long, querying them takes 10 times
as long. The time is mostly lost in marshalling, especially the column type
`REAL` is problematic: It takes a lot of CPU cycles to convert a floating
point number into a string and back. The same benchmark without that
conversion gives the following result:

    +----------------+---------+---------+
    |                | insert  | query   |
    +----------------+---------+---------+
    | SQLite C API   | 1.1 s   | 0.2 s   |
    | Sqinn          | 1.6 s   | 1.3 s   |
    +----------------+---------+---------+

Inserting is getting par with the C API. That seems reasonable: Inserting data
is a lot of work for the database, so the marshalling overhead is
comparatively small. Querying however, is fast, therefore the marshalling
overhead plays a big role here. After all: 1 million rows if a lot of data.


### Benchmark 2

The second benchmark uses a more complex schema: 3 tables, many foreign key
constraints, many indices. For details see `src/conn_test.c`. The results are
(lower numbers are better):

    +----------------+---------+---------+
    |                | insert  | query   |
    +----------------+---------+---------+
    | SQLite C API   | 0.8 s   | 0.2 s   |
    | Sqinn          | 1.0 s   | 0.7 s   |
    +----------------+---------+---------+


### Summary

Marshalling data over stdin/stdout takes time and therefore Sqinn is slower
than calling the SQLite C API directly. The performance penalty depends, as
shown, on the work the database has to do to execute a sql statement.


Limitations and Shortcomings
-------------------------------------------------------------------------------

### Single threaded

Sqinn is single threaded. It serves requests only one-after-another.


### Single statement

As of now, Sqinn does not support multiple active prepared statements at a
time. If a caller tries to prepare a statement while another one is still
active (i.e. not finalized), Sqinn will bark.


