
![Sqinn](logo-200.png "Sqinn")


> [!NOTE]
> This work is sponsored by Monibot - Website, Server and Application Monitoring.
> Try out Monibot at [https://monibot.io](https://monibot.io?ref=sqinn).


Sqinn is an alternative to the SQLite C API. Sqinn reads requests from stdin,
forwards the request to SQLite, and writes a response to stdout. It is used in
programming environments that do not allow calling C API functions.

The [SQLite database library](https://www.sqlite.org) is written in C and
provides an API for using it in C/C++. There are many language bindings. If you
cannot or do not want to use one of the available language bindings, and your
programming language allows the creation of subprocesses (fork/exec), an option
might be to communicate with SQLite over stdin/stdout, using Sqinn.

One example is [Go](https://golang.org): There exist a bunch of Go libraries
for reading and writing SQLite databases. Most of them use `cgo` to call the
SQLite C API functions. While this works (very well, indeed), it has drawbacks:
First, you have to have gcc installed on your development system. Second, cgo
slows down the Go compilation process. Third, cross compiling a cgo program to
another platform (say from Linux to MacOS) is hard to setup.

Sqinn provides functions that map to SQLite functions, like `sqlite3_open()`,
`sqlite3_prepare()`, `sqlite3_bind()`, and so on. If you have not read the
[Introduction to the SQLite C/C++
Interface](https://www.sqlite.org/cintro.html), now it's a good time. It's a
5-minute read and shows the basic workings of SQLite.

Marshalling requests and responses back and forth between process boundaries
is, of course, slow. To improve performance, Sqinn provides functions that lets
you call multiple SQLite functions in one request/response cycle.

All function calls and the binary IO protocol used for marshalling request and
response data is described in [io\_protocol.md](io_protocol.md).

For the Go (Golang) language binding, see
<https://github.com/cvilsmeier/sqinn-go>, for benchmarks, see
<https://github.com/cvilsmeier/sqinn-go-bench>.


Compiling with gcc
-------------------------------------------------------------------------------

See the included `build.sh` script for compiling Sqinn with `gcc`. I have
tested it on the following platforms:

- Windows 10 amd64, using Mingw64 gcc (tdm64-gcc-9.2.0 from https://jmeubank.github.io/tdm-gcc/)
- Windows 10 amd64, using MSVC
- Debian Linux 11/12 amd64

The releases page contains a tar file with pre-built binaries for Windows amd64
and Linux amd64, see <https://github.com/cvilsmeier/sqinn/releases>.

If you want to compile Sqinn, have gcc installed and follow the steps:

        $ git clone https://github.com/cvilsmeier/sqinn
        $ cd sqinn
        $ chmod a+x build.sh clean.sh
        $ ./build.sh

The build script creates a `bin` subdirectory that the build results go into.
Test it with:

        $ bin/sqinn test


Command line usage
-------------------------------------------------------------------------------

There isn't really one. Sqinn is not used by humans, it's used by other
programs. That said:

    $ sqinn help
    Sqinn is SQLite over stdin/stdout

    Usage:

           sqinn [options...] [command]

    Commands are:

            help            show this help page
            version         print Sqinn version
            sqlite_version  print SQLite library version
            test            execute built-in unit tests
            bench           execute built-in benchmarks

    Options are:

            -db             db file, used for test and bench
                            commands. Default is ":memory:"

    When invoked without a command, Sqinn will read (await) requests
    from stdin, print responses to stdout and output error messages
    on stderr.


Limitations
-------------------------------------------------------------------------------

### Single threaded

Sqinn is single threaded. It serves requests one after another.


### API subset

Sqinn supports only a subset of the many functions that the SQLite C/C++ API
provides. Interruption of SQL operations, backup functions, vfs and
extension functions are not supported, among others.


### Single statement

As of now, Sqinn does not support multiple active statements at a
time. If a caller tries to prepare a statement while another one is still
active (i.e. un-finalized), Sqinn will bark.


Changelog
-------------------------------------------------------------------------------

See version in src/util.h

### v1.1.42

- SQLite Version 3.49.2 (2025-05-07)

### v1.1.41

- SQLite Version 3.49.1 (2025-02-18)

### v1.1.40

- SQLite Version 3.49.0 (2025-02-06)

### v1.1.39

- SQLite Version 3.48.0 (2025-01-14)

### v1.1.38

- SQLite Version 3.47.2 (2024-12-07)

### v1.1.37

- SQLite Version 3.47.1 (2024-11-25)

### v1.1.36

- SQLite Version 3.47.0 (2024-10-21)

### v1.1.35

- SQLite Version 3.46.1 (2024-08-13)

### v1.1.34

- SQLite Version 3.46.0 (2024-05-23)

### v1.1.33

- SQLite Version 3.45.3 (2024-04-15)

### v1.1.32

- SQLite Version 3.45.2 (2024-03-12)

### v1.1.31

- SQLite Version 3.45.0 (2024-01-15)

### v1.1.30

- SQLite Version 3.44.2 (2023-11-24)

### v1.1.29

- SQLite Version 3.44.0 (2023-11-01)

### v1.1.28

- SQLite Version 3.43.2 (2023-10-10)

### v1.1.27

- SQLite Version 3.43.1 (2023-09-11)

### v1.1.26

- SQLite Version 3.43.0 (2023-08-24)

### v1.1.25

- SQLite Version 3.42.0 (2023-05-16)

### v1.1.24

- SQLite Version 3.41.2 (2023-03-22)

### v1.1.23

- SQLite Version 3.41.1 (2023-03-10)

### v1.1.22

- SQLite Version 3.41.0 (2023-02-21)

### v1.1.21

- SQLite Version 3.40.1 (2022-12-28)

### v1.1.20

- SQLite Version 3.40.0 (2022-11-16)

### v1.1.19

- SQLite Version 3.39.4 (2022-09-29)

### v1.1.18

- SQLite Version 3.39.2 (2022-07-21)

### v1.1.17

- SQLite Version 3.39.0 (2022-06-25)

### v1.1.16

- SQLite Version 3.38.5 (2022-05-06)

### v1.1.15

- SQLite Version 3.38.3 (2022-04-27)

### v1.1.14

- SQLite Version 3.38.2 (2022-03-26)

### v1.1.13

- SQLite Version 3.38.1 (2022-03-12)

### v1.1.12

- SQLite Version 3.38.0 (2022-02-22)

### v1.1.11

- SQLite Version 3.37.2 (2022-01-06)

### v1.1.10

- SQLite Version 3.37.0

### v1.1.9 (2021-06-29)

- sqlite 3.36.0 (2021-06-18)

### v1.1.8 (2021-04-23)

- sqlite 3.35.5 (2021-04-19)

### v1.1.7 (2021-04-03)

- sqlite 3.35.4 (2021-04-02)

### v1.1.6 (2021-03-20)

- fix version tests

### v1.1.5 (2021-03-18)

- sqlite 3.35.2 (2021-03-17)

### v1.1.4 (2021-01-29)

- fixed version number

### v1.1.3 (2021-01-25)

- sqlite v3.34.1 (2021-01-20)

### v1.1.2 (2020-11-06)

- sqlite v3.33.0

### v1.1.1 (2020-06-22)

- fix unsuccessful FC_EXEC/QUERY leaked prepared statement
- sqlite v3.32.3

### v1.1.0 (2020-06-14)

- fast IEEE 745 encoding for double values

### v1.0.0 (2020-06-10)

- first version


Contributing
-------------------------------------------------------------------------------

I will reject most PRs and feature requests. Why? Because I use sqinn for my
own projects, and I want it to be fast, secure, robust and easy to maintain.
I cannot include every feature under the sun.
I give it away for free, so everybody can adjust it to his or her own needs.


License
-------------------------------------------------------------------------------

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>

