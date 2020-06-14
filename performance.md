
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

Here are some benchmarks using Sqinn with SQLite library version 3.32.2
(2020-06-04), compiled on Windows 10, Mingw64, with TDM-GCC-64.

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

The database is disk-based to simulate the real world. All inserts take place
in a single transaction. The results are (lower numbers are better):

    +----------------+---------+---------+
    |                | insert  | query   |
    +----------------+---------+---------+
    | C API          | 1.0 s   | 0.2 s   |
    | Sqinn          | 1.6 s   | 1.4 s   |
    +----------------+---------+---------+

Insert takes 60% longer than the C API, query takes 700% longer. That seems
reasonable: Inserting data is a lot of work for the database, so the
marshalling overhead is comparatively small. Querying however is fast at the
database level, therefore the marshalling overhead plays a big role here.


## Benchmark 2

The second benchmark uses a more complex schema: 3 tables, with foreign key
constraints and indices. For details see function `bench_conn_complex()` in
`src/conn_test.c`. The results are (lower numbers are better):

    +----------------+---------+---------+
    |                | insert  | query   |
    +----------------+---------+---------+
    | C API          | 1.4 s   | 0.5 s   |
    | Sqinn          | 1.7 s   | 1.3 s   |
    +----------------+---------+---------+

Insert takes 20% longer than the C API, query takes 260% longer. That seems
also reasonable: Compared to Benchmark 1, the database has more work to do,
therefore the relative performance of Sqinn goes up.


## Summary

Marshalling data over stdin/stdout takes time and therefore Sqinn is slower
than calling the SQLite C API directly. The performance penalty depends, as
shown, on the work the database has to do to execute a sql statement.
