
SQINN BINARY PROTOCOL
===============================================================================

Sqinn reads a request telegram from stdin, processes the request, outputs a
response telegram to stdout, and waits for the next request telegram. A
telegram consists of a 4-byte size N, followed by a payload of N bytes. The
payload is either a request or a response. If N is zero, or a read error occurs
while reading the 4-byte size N or the payload, sqinn terminates.

A requests starts with a one-byte function code, followed by a variable number
of argument values. The function code tells sqinn which sqlite function to
execute. Function Codes are described in section "Function Codes". Data types
used to encode the argumen values are described in section "Data Types".

A response starts with a byte that is nonzero (true) if the function was
successful and zero (false) if not. If the function was successful, a variable
number of return values follows. If the function was not successful, a error
message (string value) follows, describing the error that occurred.

Example:

For opening a sqlite database file, function code 1 (FC\_OPEN) is used. The
single argument to FC_\OPEN is the filename. A sample request looks like this:

    [ 0] 0x0A        FC_OPEN
    [ 1] 0x00        4-byte size of filename plus terminating null (8 in this case)
    [ 2] 0x00
    [ 3] 0x00
    [ 4] 0x08
    [ 5] 0x74        7 bytes filename "test.db"
    [ 6] 0x65
    [ 7] 0x73
    [ 8] 0x74
    [ 9] 0x2e
    [10] 0x64
    [11] 0x62
    [12] 0x00        1 null byte

Byte 0 is set to 10, which is the numerical code for FC\_OPEN. Bytes 1 to 12
contain the only argument to FC\_OPEN, which is the filename to be opened.

For sending the request to Sqinn's stdin, the request has to be preceded by its
4-byte size N, so that Sqinn knows how many bytes it must read before it can
start processing the request. In our example N is decimal 13 (0x0C), so
the complete telegram will look like this:

    [ 0] 0x00        4-byte size of subsequent request payload (13 in this case)
    [ 1] 0x00
    [ 2] 0x00
    [ 3] 0x0C
    [ 4] 0x0A        FC_OPEN
    [ 5] 0x00        4-byte size of filename plus terminating null (8 in this case)
    [ 6] 0x00
    [ 7] 0x00
    [ 8] 0x08
    [ 9] 0x74        7 bytes filename "test.db"
    [10] 0x65
    [11] 0x73
    [12] 0x74
    [13] 0x2e
    [14] 0x64
    [15] 0x62
    [16] 0x00        1 null byte

If the file could be opened, sqinn will output the following response payload:

    [0] 0x01         bool 1 "ok"

If a error occurred, sqinn will output an error:

    [ 0] 0x00        bool 0 "not ok"
    [ 1] 0x00        4 bytes size of error message plus terminating null byte (10 i this case)
    [ 2] 0x00
    [ 3] 0x00
    [ 4] 0x0A
    [ 5] 'n'         9 bytes error message "not found"
    [ 5] 'o'
    [ 6] 't'
    [ 7] ' '
    [ 8] 'f'
    [ 9] 'o'
    [10] 'u'
    [11] 'n'
    [12] 'd'
    [13] 0x00        1 null byte

Before writing the reponse to stdout, Sqinn will write the 4-byte size N of the
response, so that consumers of Sqinn's stdout know how many bytes to read to
have a complete response.

In the following sections, we describe the data types used to encode and
decode function arguments and response values. Then we describe the function
codes provided by sqinn.


Data Types
------------------------------------------------------------------------------

### bool

A bool is encoded as a single byte. A value of zero means false, a
non-zero value means true.

Examples:

    (bool)1   [0x01]
    (bool)0   [0x00]

### int32

A int32 is a 32-bit (4-byte) signed integer value. It is encoded as 4
bytes, MSB first (big endian).

Examples:

    (int32)1    [0x00, 0x00, 0x00, 0x01]
    (int32)2    [0x00, 0x00, 0x00, 0x02]
    (int32)128  [0x00, 0x00, 0x00, 0x80]
    (int32)256  [0x00, 0x00, 0x01, 0x00]
    (int32)-1   [0xFF, 0xFF, 0xFF, 0xFF]

### int64

A int64 is a 64-bit (8-byte) signed integer value. It is encoded as 8
bytes, MSB first (big endian).

Examples:

    (int64)1    [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01]
    (int64)-1   [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]

### string

A string is a size-prefixed C-type UTF-8 string. The size is calculated as
the number of bytes (not characters) plus the terminating null byte. The
empty string has a size of 1, counting only the terminating null byte. The
size is encoded as a int32, then follow the content bytes, including
the terminating null byte. The maximum string size is 2,147,483,647
(2^31-1) bytes.

Examples:

    (string)""      [0x00, 0x00, 0x00, 0x01, 0x00]
    (string)"A"     [0x00, 0x00, 0x00, 0x02, 0x41, 0x00]
    (string)"123"   [0x00, 0x00, 0x00, 0x04, 0x31, 0x32, 0x33, 0x00]


### double

A double is a 64-bit floating point value. Since not all programming
environments use the IEEE-745 format, sqinn encodes a double value as a
string. The string encoding rules above apply for double values also.

Examples:

    (double)1.2   [0x00, 0x00, 0x00, 0x04, 0x31, 0x2E, 0x320, 0x00]


### blob

A blob is a size-prefixed array of bytes. The size is encoded as a int32,
then follow the content bytes. The maximum blob size is 2,147,483,647
(2^31-1) bytes. Blobs may, other than strings, contain null bytes.

Examples:

    (blob)[0x61, 0x00, 0x10]     [0x00, 0x00, 0x00, 0x03, 0x61, 0x00, 0x10]
    (blob)[]                     [0x00, 0x00, 0x00, 0x00]


Functions
------------------------------------------------------------------------------

This section lists all functions understood by sqinn, with arguments and
return values. The numerical values of all function codes and value types are
enumerated in `src/handler.h`. For a better understanding of the provided
functions, we recommend reading SQLite's Introduction to its C/C++ interface:
[https://www.sqlite.org/cintro.html](https://www.sqlite.org/cintro.html)

The first byte of each request is a function code. The rest of the request
encodes the function arguments, if any.

A response starts with a bool that indicates success (true) or failure (false).
For success, the rest of the response encodes the result values, if any. For
failure, the rest of the response is a string that holds an error message.
Since error reponses all look the same, we leave them out in the following
function descriptions.


### FC\_SQINN\_VERSION

This function returns the version of sqinn as a string, e.g. "1.0.0".

    Request:

        byte     FC_SQINN_VERSION

    Response (Success):

        bool     true                  true means "success"
        string   version_string        e.g. "1.0.0"


### FC\_SQLITE\_VERSION

This function returns the version of the sqlite library as a string, e.g.
"3.32.1".

    Request:

        byte     FC_SQLITE_VERSION

    Response (Success):

        bool     true
        int32    version                   e.g. "3.32.1"


### FC\_OPEN

This function opens a database.

    Request:

        byte     FC_OPEN
        string   filename              e.g. "/home/alice/my.db" or ":memory:"

    Response (Success):

        bool     true


### FC\_PREPARE

This function prepares a statement.

    Request:

        byte     FC_PREPARE
        string   sql                   e.g. "SELECT id,name FROM users"

    Response (Success):

        bool     true


### FC\_BIND

This function binds parameter values.

    Request:

        byte     FC_BIND
        int32    iparam                The parameter number (1,2,...)
        byte     value_type            VAL_NULL, VAL_INT, VAL_STRING, ...
        any      value                 any can be an int32, int64, double,
                                       string or blob value, depending on the
                                       given value_type. If value_type is
                                       VAL_NULL, value is omitted.

    Response (Success):

        bool     true


### FC\_STEP

This advances a prepared statement to its next row or to completion.

    Request:

        byte     FC_STEP
        int32    iparam                The parameter number (1,2,...)
        byte     value_type            VAL_NULL, VAL_INT, VAL_STRING, ...
        any      value                 any can be an int32, int64, double,
                                       string or blob value, depending on the
                                       given value_type. If value_type is
                                       VAL_NULL, value is omitted.

    Response (Success):

        bool     true
        bool     more                  true if another row is available. In
                                       that case, column values can be
                                       fetched with FC_COLUMN.


### FC\_RESET

Reset a prepared statement before reusing it.

    Request:

        byte     FC_RESET

    Response (Success):

        bool     true


### FC\_CHANGES

Count the number of rows modified by the last executed statement.

    Request:

        byte     FC_CHANGES

    Response (Success):

        bool     true
        int32    count                 the number of rows modified


### FC\_COLUMN

Retrieve a column value. When retrieving a column, the caller has to specify
the type of the value to retrieve. If the value type matches the column type,
no type conversion is done. Otherwise, sqlite will convert the column value to
the specified value type. See the SQLite documentation for more information
how sqlite does type conversion.

    Request:

        byte     FC_COLUMN
        int32    icol                  the column index (0,1,...)
        byte     val_type             the type of the column value

    Response (Success):

        bool     true
        bool     set                   true if the column value was set (not
                                       NULL), false if it was NULL.
        any      value                 An int23, int64, double, string or
                                       blob value, depending on the val_type
                                       argument.

Please note: If `set` is false, the value result is omitted.


### FC\_FINALIZE

Finalizes a statement.

    Request:

        byte     FC_FINALIZE

    Response (Success):

        bool     true


### FC\_CLOSE

Closes a database.

    Request:

        byte     FC_CLOSE

    Response (Success):

        bool     true


### FC\_EXEC

Executes a statement multiple times.

    Request:

        byte   FC_EXEC
        string sql
        int32  niterations   number of iterations
        int32  nparams       number of bind parameters per iteration
        for 0 to (niterations * nparams):
            byte val_type    type of parameter value
            any value

    Response (Success):

        bool     true
        for 0 to niterations:
            int32 changes      the change counter for the n'th iteration

Exec is used to combine invocations of prepare, bind, step, changes, reset and
finalize in one request/response cycle. Let's make an example. Say you want to
insert three users. The user table has two columns: `id` and `name`. The
request would then look like this:

     1: byte   FC_EXEC
     2: string "INSERT INTO users (id,name) VALUES(?,?)"
     3: int32  3
     4: int32  2
     5: byte   VAL_INT
     6: int32  1
     7: byte   VAL_TEXT
     8: string "Alice"
     9: byte   VAL_INT
    10: int32  2
    11: byte   VAL_TEXT
    12: string "Bob"
    13: byte   VAL_INT
    14: int32  3
    15: byte   VAL_NULL

- Line 1 is the function code.
- Line 2 is the sql needed for inserting users.
- Line 3 is `niteration`, the number of iterations, in this case the given
  INSERT statement will be executed three times. 
- Line 4 is `nparams`, the number of bind parameters per iteration. In this
  case, each INSERT iteration needs 2 values bound.
- Lines 5-8 contain the values for the first INSERT call: A user with id 1 and
  name Alice.
- Lines 9-12 contain the values for the next INSERT call: A user with id 2 and
  name Bob.
- Lines 13-15 contain the values for the third INSERT call: A user with id 3
  and no name (name NULL).

If `niterations` is zero, the statement is not called at all. This is allowed.
However, preparing a statement, running it zero times, and the finalizing that
statement doesn't make no sense. Therefore, FC\_EXEC should not be called in
the first place in this case.

The `nparams` argument is allowed to be zero. In that case, no parameters are
bound, and the statement is executed as-is. This is often the case if you want
to execute statements other than INSERT, UPDATE or DELETE, for example "BEGIN
TRANSACTION" or "COMMIT", or you have statements without bind parameters, e.g.
"DELETE FROM users".

For the above request, the response might look like this:


     1: bool   1
     2: int32  1
     3: int32  1
     4: int32  1

- Line 1 indicates success (true)
- Line 2 is the change counter for the first iteration: 1 row was updated by
  that sql operation.
- Line 3 is the change counter for the second iteration: 1 row was updated by
  that sql operation.
- Line 4 is the change counter for the third iteration: 1 row was updated by
  that sql operation.


### FC\_QUERY

Execute a query and fetch all row values in one go.

    Request:

        byte FC_QUERY
        string sql
        int32 nparams          number of bind parameters
        for 0 to nparams:
            byte val_type      type of parameter value
            any value          the parameter value
        int32 ncols            number of columns per row
        for 0 to ncols:
            byte val_type      type of column value

    Response (Success):

        bool   true
        int32  nrows         number of result rows
        for 0 to (nrows * ncols):
            byte val_type    type of column value
            any value

Query is used to combine invocations of prepare, bind, step, column and
finalize into one function call. Let's make an example. Say you want to query
all users with id greater than 42. The user table has two columns, an integer
column `id` and a text column `name`. The request would then look like this:

    1: byte   FC_QUERY
    2: string "SELECT id,name FROM users WHERE id > ? ORDER BY id"
    3: int32  1
    4: byte   VAL_INT
    5: int32  42
    6: int32  2
    7: byte   VAL_INT
    8: byte   VAL_TEXT

- Line 1 is the function code.
- Line 2 is the query sql.
- Line 3 is `nparams`, the number of bind paramters. In this case we have one
  bind parameter.
- Line 4 is the type of the bind parameter.
- Line 5 holds its value.
- Line 6 is the `ncols`, the number of columns per result row.
- Lines 7-8 contain the expected column types.

Let's assume the user table contained the following values:

    +---------+--------------+
    | id      | name         |
    +---------+--------------+
    | 13      | Thirteen     |
    | 37      | Thirtyseven  |
    | 42      | Fourtytwo    |
    | 51      | Fiftyone     |
    | 73      | Seventythree |
    | 81      | (NULL)       |
    +---------+--------------+

The response would then look like this:

     1: bool   true
     2: int32  3
     3: byte   VAL_INT
     4: int32  51
     5: byte   VAL_TEXT
     6: string "Fiftyone"
     7: byte   VAL_INT
     8: int32  73
     9: byte   VAL_TEXT
    10: string "Seventythree"
    11: byte   VAL_INT
    12: int32  81
    13: byte   VAL_NULL

- Line 1 contains the success flag, true in this case.
- Line 2 `nrows` indicates that the query yielded 3 result rows.
- Line 3 contains the type of the first column of the first row.
- Line 4 contains the value of the first column of the first row.
- Lines 5-6 contains the type and value of the second column of the first row.
- Lines 7-8 contains the type and value of the first column of the second row.
- Lines 9-10 contains the type and value of the second column of the second
  row.
- Lines 11-12 contains the type and value of the first column of the third
  row.
- Line 14 contains the type of the second column of the third row. Since the
  type is NULL, no value follows.

