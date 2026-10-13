## error

A function: makes an error.

#### Syntax

```text
(error message [code])
```

#### Arguments

*message*
: The error's message: a string, or any value, which is turned to text as `print` shows it.

*code*
: An atom that names the kind of error, for the program that catches it. Without it the error has no code.

#### Result

An error value. It prints as `Error: ` and the message.

#### Description

An error is a value, but one that most functions will not accept: a function given an error as an argument
does not run, and the error is its result. So the error passes outward through every enclosing call until it
is caught, or reaches the prompt, which prints it. `error` is how a program reports that it cannot go on;
`try` is how another part of the program catches the report.

The few forms that do accept an error are `try`; `def`, `set` and `set!`, so that an error can be kept in a
variable; `error?`, `type-of`, `error-message` and `error-code`; and `print`.

The errors that the system's calls give have codes too. Each is the name of the system's error code (Chapter
3 lists them) in small letters, without its `E_`: `:noent` for `E_NOENT`, a file that is not found.

#### Errors

| Message | Cause |
|---|---|
| `An error's code must be an atom` | *code* is not an atom |

#### See also

`try`, `error-message`, `error?`.

#### Example

```session
hylang> (error "out of cheese")
=> Error: out of cheese
hylang> (+ 1 (error "inside"))
=> Error: inside
hylang> (fun {half n} {if (odd? n) (error (format "{} is odd" n) :odd) (/ n 2)})
=> NIL
hylang> (half 10)
=> 5
hylang> (half 7)
=> Error: 7 is odd
hylang> (try (half 7) (list &err &code))
=> {"7 is odd" :odd}
```

## error-message, error-code

Functions: the message and the code of an error.

#### Syntax

```text
(error-message e)
(error-code e)
```

#### Arguments

*e*
: An error, usually one kept in a variable, or the value of an expression that may fail.

#### Result

`error-message` gives the message, as a string, without the `Error: ` that is printed before it.
`error-code` gives the code, an atom, or `NIL` for an error that has none. For a value that is not an error,
both give `NIL`.

#### Description

These two are given errors without passing them on, so `(error-code (open path))` is a way to ask why a
call failed without `try`. For an error from the system the code is the system's name for it: `:noent` (not
found), `:exist` (already exists), `:isdir` (is a directory), `:rofs` (read-only), and so on.

#### See also

`error`, `try`, `error?`, `errstr`.

#### Example

```session
hylang> (def {e} (error "out of cheese" :cheese))
=> NIL
hylang> (error-message e)
=> "out of cheese"
hylang> (error-code e)
=> :cheese
hylang> (error-code (open "/nothere"))
=> :noent
hylang> (error-message (open "/nothere"))
=> "/nothere: not found"
hylang> (error-code 5)
=> NIL
```

## print

A function: prints values, and ends the line.

#### Syntax

```text
(print x ...)
```

#### Arguments

*x*
: Any values. There may be none.

#### Result

`NIL`. The values are written to standard output with one space between them, and then a new line. With no
arguments, only the new line.

#### Description

Each value is written as text meant for a person. A string is written as its characters, without quotes, and
a character as itself. A number is written in the current base. A list is written in braces, and the strings
and characters *inside* a list keep their quotes and backslashes, so that the items can be told apart.

`print` accepts an error, and prints it as the prompt would.

To write values as they would be typed, so that they can be read back, use `save`. To write without the new
line or the spaces, use `write`. To write to a file, use `print-to`.

#### See also

`write`, `save`, `print-to`, `format`, `output-of`.

#### Example

```session
hylang> (print "one" 2 {3})
one 2 {3}
=> NIL
hylang> (print "a" \b :c 1/2)
a b :c 1/2
=> NIL
hylang> (print {1 "two" \3})
{1 "two" \3}
=> NIL
```

## write

A function: prints values, with nothing added.

#### Syntax

```text
(write x ...)
```

#### Arguments

*x*
: Any values.

#### Result

`NIL`. The values are written to standard output as `print` writes each, but with nothing between them and
no new line after them.

#### Description

`write` is for building a line a piece at a time, and for prompts that the answer should follow on the same
line. A new line is written when the program asks for one, with `"\n"` in a string or with `(print)`.

#### See also

`print`, `write-to`, `format`.

#### Example

```session
hylang> (write "no new line")
no new line=> NIL
hylang> (write 1 2)
12=> NIL
hylang> (do (write "a" "b") (print))
ab
=> NIL
```

## save

A function: writes a value as it would be typed, to the screen or to a file.

#### Syntax

```text
(save x)
(save x path [mode])
```

#### Arguments

*x*
: Any value.

*path*
: A file to write to. Without it, the value is printed.

*mode*
: What to do if the file is already there: `:overwrite`, to replace its contents, or `:append`, to add to
  them.

#### Result

`NIL`.

#### Description

The value is written in the form that reads back as the same value: a string with its quotes and escapes, a
character with its backslash, a hash as `#(...)`, and a new line after it. This is the opposite of `read` and
`load`: a file of saved values is a file of hylang source, and `load` gives the value of the last one in it.

For a file that does not exist, no mode is needed. For one that does, a mode must be given, so that a file is
never replaced by accident.

#### Errors

| Message | Cause |
|---|---|
| `File exists but neither ':append' nor ':overwrite' were specified` | The file is there, and no mode was given |
| The system's message, after the path | The file could not be made or written |

#### See also

`print`, `repr`, `load`, `read`, `write-file`.

#### Example

```session
hylang> (save "a string")
"a string"
=> NIL
hylang> (save {1 "two" \3 :four 5/6})
{1 "two" \3 :four 5/6}
=> NIL
hylang> (save {1 2} "/ram/saved.hl")
=> NIL
hylang> (save {3 4} "/ram/saved.hl" :append)
=> NIL
hylang> (read-file "/ram/saved.hl")
=> "{1 2}\n{3 4}\n"
hylang> (load "/ram/saved.hl")
=> {3 4}
```

## read-line, read-byte, read-all

Functions: read from the keyboard, or from a stream.

#### Syntax

```text
(read-line [s])
(read-byte [s])
(read-all [s])
```

#### Arguments

*s*
: A stream open for reading (see `open`). Without it, standard input: the keyboard, unless hylang's input
  was redirected.

#### Result

| Function | Result | At the end |
|---|---|---|
| `read-line` | The next line, as a string, without the line end that closed it | `NIL` |
| `read-byte` | The next byte, as a number from 0 to 255 | `NIL` |
| `read-all` | Everything from the current place to the end, as one string | `""` |

#### Description

A line ends at a line feed, and a carriage return before it is dropped. The last line of a file is given
whether or not it ends with one.

Because `read-line` gives `NIL` only at the end, and an empty line is `""`, which is true, a loop can read
until `NIL`. From the keyboard, the end is <kbd>CTRL</kbd>-<kbd>D</kbd> at the start of a line.

The keyboard gives its characters a line at a time, when <kbd>ENTER</kbd> is pressed, so `read-byte` from
the keyboard waits for a whole line and then gives its bytes one by one, the line feed last. For keys one at
a time as they are pressed, use `key`.

#### Errors

| Message | Cause |
|---|---|
| `Cannot read from this stream` | The stream was opened for writing only, or has been closed |

#### See also

`open`, `read-file`, `read-lines`, `read-bytes`, `key`.

#### Example

```session
hylang> (write-file "/ram/a.txt" "line one\nline two\n")
=> NIL
hylang> (def {s} (open "/ram/a.txt"))
=> NIL
hylang> (read-line s)
=> "line one"
hylang> (read-byte s)
=> 108
hylang> (read-all s)
=> "ine two\n"
hylang> (read-line s)
=> NIL
hylang> (close s)
=> NIL
```

## key, key?

Functions: the next key pressed, without waiting for a line; and whether one is waiting.

#### Syntax

```text
(key)
(key?)
```

#### Result

`key` waits for a key and gives it: a character for an ordinary key, and an atom for one of the terminal's
named keys: `:up`, `:down`, `:left`, `:right`, `:home`, `:end`, `:ins`, `:del`, `:pgup`, `:pgdn`, and `:f1` to
`:f12`. <kbd>ENTER</kbd> is the character `\cr`. At the end of the input it gives `NIL`.

`key?` gives `T` if a key has been pressed and not yet read, and `NIL` if not. It does not wait.

#### Description

Keys are read raw: as they are pressed, without being shown on the screen, and without the line editing
that `read-line` and the prompt give. The keyboard goes back to lines when the next line is read.

Together they make the loop of a game or an editor: `(if (key?) (key))` takes a key when there is one, and
lets the program go on when there isn't.

<kbd>CTRL</kbd>-<kbd>C</kbd> pressed while a program waits in `key` is the error `interrupted`.

#### See also

`read-line`, `on-note`, `sleep`.

#### Example

Here <kbd>x</kbd> and then the up arrow were pressed:

```text
hylang> (key)
=> \x
hylang> (key)
=> :up
```

## open

A function: opens a file, giving a stream.

#### Syntax

```text
(open path [mode])
```

#### Arguments

*path*
: The file's name, a string. A name that doesn't start with `/` is looked for from the current directory.

*mode*
: An atom: how the file will be used. Without it, `:read`.

| Mode | The file | The stream |
|---|---|---|
| `:read` | Must be there | Is read from its start |
| `:write` | Is made, or emptied if it is there | Is written from its start |
| `:append` | Is made if it isn't there, and nothing in it is lost | Is written at its end |
| `:update` | Must be there. Nothing is emptied | Is read and written in place, from its start |

#### Result

A stream: a value that stands for the open file and its current place. It prints as `<stream>`.

#### Description

A stream is passed to the reading functions (`read-line`, `read-byte`, `read-all`, `read-bytes`,
`read-buffer`), the writing functions (`print-to`, `write-to`, `write-bytes`), `seek` and `tell`, and then to
`close`. Each read or write moves the stream's place on by what was read or written.

Any file can be opened, a device's too: `(open "/dev/time")` is a stream on the clock.

A program should close what it opens: a task can have only so many files open at once. For a whole file at a
time, `read-file`, `read-lines` and `write-file` open and close it themselves.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | `:read` or `:update` of a file that isn't there, or a directory in the path that isn't |
| *path*`: is a directory` | The path names a directory. Use `ls` or `dir` |
| `'open' mode must be :read, :write, :append or :update, not :bogus` | Another atom was given as the mode |
| `'open' mode must be :read, :write, :append or :update` | The mode is not an atom |
| `'open' expects a String` | *path* is not a string |

Other failures give the system's message after the path (Chapter 3 lists them), and its code as the error's.

#### See also

`close`, `read-line`, `print-to`, `seek`, `read-file`, `write-file`, `exists?`.

#### Example

```session
hylang> (def {w} (open "/ram/d.txt" :write))
=> NIL
hylang> (print-to w "hello" 42)
=> NIL
hylang> (close w)
=> NIL
hylang> (def {s} (open "/ram/d.txt"))
=> NIL
hylang> (read-line s)
=> "hello 42"
hylang> (close s)
=> NIL
hylang> (open "/ram/nothere")
=> Error: /ram/nothere: not found
```

## close

A function: closes a stream.

#### Syntax

```text
(close s)
```

#### Arguments

*s*
: A stream.

#### Result

`NIL`. The file is closed, and what was written to it is safely in it.

#### Description

Closing a stream that is already closed does nothing, and is not an error. Reading or writing a closed
stream is one: `Cannot read from this stream`, `Cannot write this stream`.

#### Errors

| Message | Cause |
|---|---|
| `'close' expects a stream` | *s* is not a stream |

#### See also

`open`.

#### Example

```session
hylang> (def {s} (open "/rom/lib/shell"))
=> NIL
hylang> (close s)
=> NIL
hylang> (read-line s)
=> Error: Cannot read from this stream
```

## print-to, write-to

Functions: `print` and `write`, to a stream.

#### Syntax

```text
(print-to s x ...)
(write-to s x ...)
```

#### Arguments

*s*
: A stream open for writing: `:write`, `:append` or `:update`.

*x*
: Any values.

#### Result

`NIL`. `print-to` writes what `print` would show: the values with spaces between, then a line feed.
`write-to` writes what `write` would: the values with nothing between or after them.

#### Errors

| Message | Cause |
|---|---|
| `'print-to' expects a stream first` | *s* is not a stream (and `'write-to'`) |
| `Cannot write this stream` | The stream was opened with `:read`, or has been closed |

#### See also

`print`, `write`, `write-bytes`, `write-file`, `open`.

#### Example

```session
hylang> (def {w} (open "/ram/d.txt" :write))
=> NIL
hylang> (print-to w "hello" 42)
=> NIL
hylang> (write-to w "no" "nl")
=> NIL
hylang> (close w)
=> NIL
hylang> (read-file "/ram/d.txt")
=> "hello 42\nnonl"
```

## read-bytes, write-bytes

Functions: read and write a stream's bytes as numbers.

#### Syntax

```text
(read-bytes s n)
(write-bytes s bytes)
(write-bytes s buffer [i [n]])
```

#### Arguments

*s*
: A stream, open for reading (`read-bytes`) or for writing (`write-bytes`).

*n*
: How many bytes: a whole number, 0 or more.

*bytes*
: A list of whole numbers from 0 to 255.

*buffer*
: A buffer. With *i*, its bytes from position *i* on are written; with *n* too, only *n* of them.

#### Result

`read-bytes` gives a list of the next *n* bytes. Near the end of the file the list is shorter than was asked
for, and at the end it is `NIL`.

`write-bytes` gives `NIL`. The bytes are written as they are, with nothing added: this is how a file that is
not text is made.

#### Errors

| Message | Cause |
|---|---|
| `A count must be an integer` | *n* is negative, or not a whole number |
| `'write-bytes' expects a list of bytes` | The second argument is neither a list nor a buffer. For a string's bytes, use `write-to`, or `(bytes s)` |
| `256 isn't a byte` | An item of the list is not a whole number from 0 to 255 |
| `'write-bytes' takes a part (i n) only of a buffer` | *i* was given with a list |
| `Cannot write this stream` | The stream is not open for writing |

#### See also

`read-byte`, `read-buffer`, `bytes`, `buffer`, `number-bytes`.

#### Example

```session
hylang> (def {w} (open "/ram/data" :write))
=> NIL
hylang> (write-bytes w {72 105 33})
=> NIL
hylang> (close w)
=> NIL
hylang> (def {s} (open "/ram/data"))
=> NIL
hylang> (read-bytes s 2)
=> {72 105}
hylang> (read-bytes s 100)
=> {33}
hylang> (read-bytes s 100)
=> NIL
hylang> (close s)
=> NIL
```

## seek, tell

Functions: set, and ask, a stream's place in its file.

#### Syntax

```text
(seek s n)
(tell s)
```

#### Arguments

*s*
: A stream.

*n*
: The place to move to: a number of bytes from the start of the file, from 0 to the file's length.

#### Result

Both give the stream's place, as a number of bytes from the start: for `seek`, the place it has just set.
The next read or write happens there.

#### Description

A place is counted in bytes, the first being 0. `(seek s 0)` goes back to the start. To find a file's
length, ask `stat`.

With a stream opened `:update`, `seek` is how a part of a file is changed in place: writing there replaces
the bytes that were there, and does not move the ones after.

#### Errors

| Message | Cause |
|---|---|
| `Cannot seek past end of stream` | *n* is more than the file's length |
| `Cannot seek before beginning of stream` | *n* is negative |
| `'seek' expects a stream` | *s* is not a stream |

#### See also

`open`, `stat`, `read-bytes`.

#### Example

```session
hylang> (write-file "/ram/d.txt" "hello 42\n")
=> NIL
hylang> (def {u} (open "/ram/d.txt" :update))
=> NIL
hylang> (seek u 6)
=> 6
hylang> (read-line u)
=> "42"
hylang> (tell u)
=> 9
hylang> (seek u 0)
=> 0
hylang> (write-to u "J")
=> NIL
hylang> (close u)
=> NIL
hylang> (read-file "/ram/d.txt")
=> "Jello 42\n"
```

## read-file, read-lines

Functions: a whole file, as one string or as a list of lines.

#### Syntax

```text
(read-file path)
(read-lines path)
```

#### Arguments

*path*
: The file's name, a string.

#### Result

`read-file` gives the file's contents as one string, line ends and all. `read-lines` gives a list of its
lines, each a string without its line end; a carriage return before a line feed is dropped, and a last line
with no line end still counts. An empty file is `""`, and `NIL`.

#### Description

The file is opened, read and closed in the one call. The whole of it must fit in hylang's memory: for a
large file, `open` it and read it a line at a time.

Devices are files, so these read them too: `(read-file "/dev/time")` is the date and time.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | There is no such file (and the system's other messages, for its other errors) |
| `'read-file' expects a path` | *path* is not a string |

#### See also

`write-file`, `open`, `read-line`, `str-split`, `load`.

#### Example

```session
hylang> (write-file "/ram/a.txt" "line one\nline two\n")
=> NIL
hylang> (read-file "/ram/a.txt")
=> "line one\nline two\n"
hylang> (read-lines "/ram/a.txt")
=> {"line one" "line two"}
hylang> (read-lines "/rom/lib/shell")
=> {"/bin/forth -l"}
hylang> (read-file "/ram/nothere")
=> Error: /ram/nothere: not found
```

## write-file, append-file

Functions: write values to a file, replacing it or adding to it.

#### Syntax

```text
(write-file path x ...)
(append-file path x ...)
```

#### Arguments

*path*
: The file's name, a string.

*x*
: Any values. Each is written as `print` shows it, with nothing between them and no line end added: put
  `"\n"` where a line should end.

#### Result

`NIL`. `write-file` makes the file, or empties it if it is there, and writes the values. `append-file` makes
it if it isn't there, and writes the values after what it holds. With no values, `write-file` leaves an
empty file.

#### Description

The file is opened, written and closed in the one call. Devices are files, so this is also how a command is
sent to a device's control file (Chapter 12 of the manual).

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | A directory in the path isn't there (and the system's other messages, for its other errors) |

#### See also

`read-file`, `save`, `open`, `print-to`.

#### Example

```session
hylang> (write-file "/ram/b.txt" "x = " 42 "\n")
=> NIL
hylang> (append-file "/ram/b.txt" "y = " {1 2} "\n")
=> NIL
hylang> (read-file "/ram/b.txt")
=> "x = 42\ny = {1 2}\n"
```

## ls, dir, stat, glob

Functions: the names in a directory; their details; one file's details; and the paths that match a pattern.

#### Syntax

```text
(ls [path])
(dir [path])
(stat path)
(glob pattern)
```

#### Arguments

*path*
: A directory, or a file. For `ls` and `dir`, the current directory if it isn't given.

*pattern*
: A path in which `*` stands for any run of characters, `?` for any one character, and `[...]` for any one of
  the characters in the brackets (`a-z` is a range, and `~` first means any character *not* listed). Each
  part of the path between slashes can be a pattern.

#### Result

`ls` gives a list of the names in the directory, as strings, sorted. Given a file, it gives a list of that
one name.

`stat` gives a hash of the file's details:

| Key | Value |
|---|---|
| `:name` | Its name, without its directory |
| `:length` | Its length in bytes (0 for a directory) |
| `:dir` | `T`, for a directory. A file has no such entry |
| `:mtime` | When it was last changed, in seconds since the start of the year 2000, as `time` counts |
| `:mode` | Its permission bits, as a number (438 is octal 666: it can be read and written) |
| `:qid` | The file system's own number for the file |
| `:dev` | The letter of the device it is on, a character |

`dir` gives a list of such hashes, one for each name `ls` would give.

`glob` gives a sorted list of the paths that match, each written as the pattern was (with the directory it
gave), and `NIL` if none does.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | There is no such file or directory |

#### See also

`exists?`, `date`, `cd`.

#### Example

```session
hylang> (mkdir "/ram/sub")
=> NIL
hylang> (write-file "/ram/sub/f1" "1")
=> NIL
hylang> (write-file "/ram/sub/f2.hl" "22")
=> NIL
hylang> (ls "/ram/sub")
=> {"f1" "f2.hl"}
hylang> (glob "/ram/sub/*.hl")
=> {"/ram/sub/f2.hl"}
hylang> ((stat "/ram/sub/f2.hl") :length)
=> 2
hylang> ((stat "/ram/sub") :dir)
=> T
hylang> (map (fn {d} {d :name}) (dir "/ram/sub"))
=> {"f1" "f2.hl"}
```

## exists?, dir?, file?

Functions: is a path there, and is it a directory or a file?

#### Syntax

```text
(exists? path)
(dir? path)
(file? path)
```

#### Arguments

*path*
: A string.

#### Result

`T` or `NIL`. `exists?` is `T` if the path names anything; `dir?` if it names a directory; `file?` if it
names something that is not a directory. For a path that names nothing, all three are `NIL`: none of them
gives an error for a missing file.

#### See also

`stat`, `ls`, `open`.

#### Example

```session
hylang> (exists? "/rom/lib/shell")
=> T
hylang> (file? "/rom/lib/shell")
=> T
hylang> (dir? "/rom/lib")
=> T
hylang> (exists? "/rom/nothere")
=> NIL
```

## mkdir, remove, rename, copy-file

Functions: make a directory; remove a file or a directory; rename a file; copy a file.

#### Syntax

```text
(mkdir path)
(remove path)
(rename path name)
(copy-file from to)
```

#### Arguments

*path*
: A string: what to make, remove or rename.

*name*
: The new name.

*from*, *to*
: The file to copy, and the file to make.

#### Result

Each gives `NIL` when it has done its work.

#### Description

`mkdir` makes one directory. The directory it is to be in must be there already: it doesn't make a path of
several.

`remove` takes away a file, or a directory that is empty.

`rename` changes a file's name, in the directory it is in. It does not move a file: only the last part of
*name* is used, and any directory written before it is ignored. To move a file to another directory, copy it
and remove the original.

`copy-file` makes *to*, or empties it if it is there, and writes *from*'s bytes into it. *to* must be a
file's name, not a directory's.

#### Errors

| Message | Cause |
|---|---|
| *path*`: already exists` | `mkdir` of something that is there |
| *path*`: not found` | The file isn't there; or, for `mkdir`, the directory it is to go in |
| *path*`: directory not empty` | `remove` of a directory that still holds something |
| *path*`: is a directory` | `copy-file` to a directory |

#### See also

`ls`, `exists?`, `write-file`.

#### Example

```session
hylang> (mkdir "/ram/t1")
=> NIL
hylang> (write-file "/ram/t1/a" "hello")
=> NIL
hylang> (rename "/ram/t1/a" "b")
=> NIL
hylang> (copy-file "/ram/t1/b" "/ram/t1/c")
=> NIL
hylang> (ls "/ram/t1")
=> {"b" "c"}
hylang> (remove "/ram/t1")
=> Error: /ram/t1: directory not empty
hylang> (remove "/ram/t1/b")
=> NIL
hylang> (remove "/ram/t1/c")
=> NIL
hylang> (remove "/ram/t1")
=> NIL
```

## cd, cwd

Functions: set, and ask, the current directory.

#### Syntax

```text
(cd [path])
(cwd)
```

#### Arguments

*path*
: A directory. Without it, `/`.

#### Result

`cd` gives `NIL`, and makes *path* the current directory: the one that names not starting with `/` are
looked for in. `cwd` gives the current directory's full path, as a string.

#### Description

The current directory is this task's. Programs that hylang starts afterward begin in it; the shell that
started hylang is not affected.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | There is no such directory |
| `'cd' expects a path` | *path* is not a string |

#### Example

```session
hylang> (cd "/rom/lib")
=> NIL
hylang> (cwd)
=> "/rom/lib"
hylang> (exists? "shell")
=> T
hylang> (cd)
=> NIL
hylang> (cwd)
=> "/"
```
