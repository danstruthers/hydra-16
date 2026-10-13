## OPEN

A statement: opens a file, and gives it a number for the statements that read and write it.

#### Syntax

```text
OPEN path$ FOR {INPUT | OUTPUT | APPEND | BINARY} AS [#]f [LEN = n]
```

#### Arguments

*path$*
: A string expression: the file's name. A name that doesn't start with `/` is taken from the current directory
  (`CHDIR`). It can be any name in the program's namespace: a file on a disk, or a device such as `/dev/cons`.

*f*
: A numeric expression from 1 to 255: the number the file is known by until it is closed. The `#` is optional.

*n*
: Accepted, evaluated and ignored. QuickBASIC's record length has no meaning here: `GET` and `PUT` take a
  position in bytes.

#### Description

The mode decides how the file is opened, and which statements may then use it:

| Mode | A file that exists | A file that doesn't | Starts at | May then be used by |
|---|---|---|---|---|
| `INPUT` | Opened for reading | Error 53 | Its first byte | `INPUT #`, `LINE INPUT #`, `INPUT$`, `EOF`, `LOF`, `LOC`, `SEEK` |
| `OUTPUT` | Emptied, and opened for writing | Made | Its first byte | `PRINT #`, `WRITE #`, `LOF`, `LOC`, `SEEK` |
| `APPEND` | Opened for writing, its contents kept | Made | Its end | `PRINT #`, `WRITE #`, `LOF`, `LOC`, `SEEK` |
| `BINARY` | Opened for reading and writing | Made | Its first byte | All of the above, and `GET` and `PUT` |

Up to 8 files can be open at once. A file stays open until `CLOSE` names it, `CLOSE` with no number closes them
all, or a `RUN` starts (which closes every file first). Leaving HyBASIC closes them too.

`EOF(f)` is always true (−1) for a file opened `FOR OUTPUT` or `FOR APPEND`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 52 | `bad file number` | *f* is less than 1 or more than 255 |
| 55 | `file already open` | Number *f* is in use |
| 67 | `too many files` | 8 files are open already |
| 53 | `file not found` | `FOR INPUT`: there's no such file. Any mode: a directory in the path isn't there |
| 70 | `permission denied` | The file, or its disk (`/rom`), can't be written |
| 13 | `type mismatch` | *path$* is a number, or *f* a string |

The system's other refusals come through as its own errors (a full disk is 61, `disk full`).

#### Notes

The limit of 8 is HyBASIC's own. Each open file also takes one of the task's 16 file descriptors, three of which
are the console's.

One file can be open under two numbers at once, each with its own position: opening it `FOR INPUT` as `#2` while
it is open `FOR OUTPUT` as `#1` is allowed, and reads what has been written so far.

A file written with `PRINT #` holds exactly the bytes `PRINT` would have sent to the screen, each line ended by a
line feed (byte 10).

#### See also

`CLOSE`, `PRINT`, `WRITE`, `INPUT`, `LINE INPUT`, `GET`, `PUT`, `SEEK`, `EOF`, `LOF`, `LOC`, `KILL`, `NAME`.

#### Example

```session
> OPEN "/ram/log.txt" FOR OUTPUT AS #1 : PRINT #1, "one" : PRINT LOF(1); EOF(1) : CLOSE #1
 4 -1
> OPEN "/ram/log.txt" FOR APPEND AS #1 : PRINT #1, "two" : PRINT LOF(1) : CLOSE #1
 8
> OPEN "/ram/log.txt" FOR INPUT AS #1 : LINE INPUT #1, L$ : PRINT L$; EOF(1)
one 0
> PRINT #1, "three"
bad file mode
> OPEN "/ram/other.txt" FOR OUTPUT AS #1
file already open
> CLOSE : OPEN "/ram/nothere.txt" FOR INPUT AS #1
file not found
> PRINT ERR
 53
```
