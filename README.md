
![Sqinn](logo-200.png "Sqinn")

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

For performance considerations and benchmarks, see
[performance.md](performance.md).

For the Go (Golang) language binding, see
<https://github.com/cvilsmeier/sqinn-go>.


Compiling with gcc
-------------------------------------------------------------------------------

See the included `build.sh` script for compiling Sqinn with `gcc`. I have
tested it on the following platforms:

- Windows 10 amd64, using Mingw64 gcc
- Debian Linux 10 amd64
- Raspbian Linux arm (Raspberry Pi)
- Darwin amd64 (MacOS)

The releases page contains a tar file with pre-built binaries which you can try
out, see <https://github.com/cvilsmeier/sqinn/releases>.


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

