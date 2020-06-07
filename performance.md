
SQINN PERFORMANCE
===============================================================================

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


## Benchmark 1

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
`REAL` is time consuming: It takes a lot of CPU cycles to convert a floating
point number into a string and back. The same benchmark without that
conversion gives the following result:

    +----------------+---------+---------+
    |                | insert  | query   |
    +----------------+---------+---------+
    | SQLite C API   | 1.1 s   | 0.2 s   |
    | Sqinn          | 1.6 s   | 1.3 s   |
    +----------------+---------+---------+

Insert is getting par with the C API. That seems reasonable: Inserting data
is a lot of work for the database, so the marshalling overhead is
comparatively small. Querying however is fast, therefore the marshalling
overhead plays a big role here, Sqinn is 6 to 7 times slower that direct API
calls.


## Benchmark 2

The second benchmark uses a more complex schema: 3 tables, many foreign key
constraints, many indices. For details see function `bench_conn_complex()` in
`src/conn_test.c`. The results are (lower numbers are better):

    +----------------+---------+---------+
    |                | insert  | query   |
    +----------------+---------+---------+
    | SQLite C API   | 0.8 s   | 0.2 s   |
    | Sqinn          | 1.0 s   | 0.7 s   |
    +----------------+---------+---------+


## Summary

Marshalling data over stdin/stdout takes time and therefore Sqinn is slower
than calling the SQLite C API directly. The performance penalty depends, as
shown, on the work the database has to do to execute a sql statement.

