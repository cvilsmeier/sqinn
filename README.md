![Sqinn](logo.png "Sqinn")

[![Build Status](https://github.com/cvilsmeier/sqinn/actions/workflows/linux.yml/badge.svg)](https://github.com/cvilsmeier/sqinn/actions/workflows/linux.yml)
[![Build Status](https://github.com/cvilsmeier/sqinn/actions/workflows/windows.yml/badge.svg)](https://github.com/cvilsmeier/sqinn/actions/workflows/windows.yml)
[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](http://unlicense.org/)


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

Sqinn provides functions to execute SQL statements and query database rows.

All function calls and the binary protocol used for sending request and
receiving responses is described in [rfx.txt](rfx.txt).

For the Go (Golang) language binding, see <https://github.com/cvilsmeier/sqinn-go>,
for benchmarks, see <https://github.com/cvilsmeier/sqinn-go-bench>.


Compiling
-------------------------------------------------------------------------------

See the included `build.sh` and `build.bat` script for compiling Sqinn. 
I have tested it on the following platforms:

- Windows 10 amd64, using MSVC Build Tools (cl.exe)
- Debian Linux 12 amd64 (gcc)

The releases page contains a tar file with pre-built binaries for Windows amd64
and Linux amd64, see <https://github.com/cvilsmeier/sqinn/releases>.

If you want to compile Sqinn, have gcc installed and follow the steps:

```bash
git clone https://github.com/cvilsmeier/sqinn
cd sqinn
chmod a+x script/*.sh
script/build.sh
```

The build script creates a `bin` subdirectory that the build results go into.
Test it with:

```bash
bin/sqinn test
```

See also <https://github.com/cvilsmeier/sqinn/actions> for build actions for
Linux and Windows.


Command line usage
-------------------------------------------------------------------------------

There isn't really one. Sqinn is not used by humans, it's used by other
programs. That said:

    $ sqinn
    sqinn v2.0.0 - SQLite over stdin/stdout.

    Usage:
            sqinn <command> [options...]

    The commands are:

            run               Listen to stdin and write to stdout.
            test              Execute selftest and exit.
            version           Print version and exit.
            sqlite            Print SQLite library version and exit.
            help              Print help page and exit.

    The options are:

            -db <dbname>      Database name. Default is ":memory:"
            -loglevel <level> Log level: 0=off, 1=info, 2=debug. Default is 0 (off).
            -logfile <file>   Log to a file. Default is empty (no file logging).
            -logstderr        Log to stderr. Default is off (no stderr logging).


Limitations
-------------------------------------------------------------------------------

### Single threaded

Sqinn is single threaded. It serves requests one after another.


### API subset

Sqinn supports only a subset of the many functions that the SQLite C/C++ API
provides. Interruption of SQL operations, incremental blob i/o,
vfs and extension functions are not supported, among others.


Contributing
-------------------------------------------------------------------------------

I have to reject most PRs and feature requests. Why? Because I use sqinn for my
own projects, and I need it to be fast, secure, reliable and easy to maintain.
I cannot include every feature under the sun.
I give it away for free, so everybody can adjust it to his or her own needs.


Changelog
-------------------------------------------------------------------------------

### v2.0.0

- New I/O protocol
- SQLite Version 3.50.4 (2025-07-30)
