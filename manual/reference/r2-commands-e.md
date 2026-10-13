## ed

A program: the line editor.

#### Syntax

```text
ed [file]
```

#### Arguments

*file*
: The file to edit. One that doesn't exist is not an error: `ed` says `new file` and starts with nothing, and
  the file is made when the text is first written with `w`.

#### Description

`ed` holds the file's lines in memory and changes them by commands typed at its `*` prompt, each naming the
lines it is about by their numbers. It needs nothing of the terminal but lines in and lines out, so it works
over any connection, in a script, and with its input from a pipe. Its commands are in Chapter 11.

Nothing is written to the file until a `w` command.

#### See also

`edit`; Chapter 11, under `ed`.

## edit

A program: the screen editor.

#### Syntax

```text
edit [+n] [file ...]
```

#### Arguments

*file*
: A file to edit. Several can be given, each in a buffer of its own; one that doesn't exist starts empty. With
  none, `edit` starts with an empty, unnamed buffer.

`+`*n*
: Start with the cursor on line *n*.

#### Description

`edit` shows the text on the whole screen and the cursor moves through it; what is typed goes in where the
cursor is. Its commands are control keys, listed along the bottom of its screen, in the manner of `nano`
(Chapter 11). It needs a terminal that understands the cursor's escape sequences (section 13.7), which HydraOS's
own screen and any terminal emulator do.

The text is kept in the task's RAM banks, so a file's size is limited by those, not by the 32K of ordinary
memory.

HyBASIC's `EDIT` command runs this program on the program in memory.

#### See also

`ed`, `more`; Chapter 11, under `edit`.

## forth

A program: HyForth.

#### Syntax

```text
forth [-l] [file [argument ...]]
```

#### Arguments

*file*
: A file of Forth to run. When it has been read to its end, HyForth ends. The arguments after it are the
  script's own (`argc` and `arg`, section 20.9).

`-l`
: Start as a login shell: build the namespace and run the profile, `/lib/forth/profile.fs`, first (section
  8.16).

#### Description

With no file, HyForth starts at its prompt, which shows the current directory. It is the shell a Hydra starts
with: besides Forth's words it runs programs by name (section 8.15). `bye` leaves it.

#### Exit status

`1` if the file can't be read.

#### See also

Chapter 4 of this book; Chapters 19 and 20.

## free

A program: reports how much memory there is.

#### Syntax

```text
free
```

#### Description

Two lines are printed. The first is the RAM every task has for itself in banks: 128K for each RAM module
fitted. The second is the shared RAM: its size, how much of it is in shared segments and how many of those
there are, and how much is free.

The figures are for the machine, not for one program. To see what a task is using, look in `/proc` (section
17.8).

Arguments are ignored.

#### Exit status

Empty.

#### See also

`ps`, `df`; Appendix F.

#### Example

```session
/> free
ram     384 KB a task (3 modules)
shared  1024 KB, 512 KB in segments (1), 512 KB free
```

## fsck

A program: checks a disk's file system, and can repair it.

#### Syntax

```text
fsck [-f] disk
```

#### Arguments

*disk*
: The disk's name: the name it has under `/sd`, or the letter of a built-in disk (`r` is the RAM disk).

`-f`
: Fix what is found wrong.

#### Description

The check is done by the storage driver, which `fsck` asks through the disk's control file. It reads every
directory and every file's list of clusters, and compares what the files use with the free map. It then prints
the disk's label, its free space, and what it found:

* **lost**: clusters marked as used that no file has. They are wasted space.
* **unmarked**: clusters a file has that are marked free. Something else could be given them.
* **twice**: clusters that two files both claim.

All three are 0 on a sound disk. `-f` puts the free map right, which cures the first two.

#### Exit status

`1` if there is no such disk. `usage` with no disk.

#### See also

`df`, `mkfs`, `label`; section 14.6.

#### Example

```session
/> fsck r
hydrafs label=RAM
free 252 KB of 255 KB
check: lost 0, unmarked 0, twice 0
banks $00-$1f
```

## grep

A program: prints the lines that match a pattern.

#### Syntax

```text
grep [-chilnsv] [-e] pattern [file ...]
```

#### Arguments

*pattern*
: A regular expression.

*file*
: The files to search. Standard input if there are none.

| Option | Effect |
|---|---|
| `-c` | Print a count of the matching lines, not the lines |
| `-h` | Don't put the file's name before each line |
| `-i` | Treat capitals and small letters as the same |
| `-l` | Print only the names of the files that have a matching line |
| `-n` | Put each line's number before it |
| `-s` | Say nothing about files that can't be read |
| `-v` | Print the lines that don't match |
| `-e` | The next argument is the pattern, even if it starts with `-` |

#### Description

Each line of each file is tested, and printed if some part of it matches the pattern. When more than one file
is named, each line printed has its file's name and a colon before it.

The pattern's language:

| Pattern | Matches |
|---|---|
| *c* | The character *c*, if it is not one of those below |
| `\`*c* | The character *c*, whatever it is |
| `.` | Any one character |
| `[abc]`, `[a-z]` | One character of those listed, or in the range |
| `[^abc]` | One character not listed |
| `^`, `$` | The start of the line, and its end |
| *e*`*` | *e* any number of times, none included |
| *e*`+` | *e* once or more |
| *e*`?` | *e* once or not at all |
| *e1*`|`*e2* | Either |
| `(`*e*`)` | *e*, as a group |

A pattern matches anywhere in a line unless `^` and `$` tie it down. The shell reads the command line first, so
a pattern with any of rc's own characters in it goes in quotes.

#### Exit status

Empty if a line matched. `no matches` if none did. `1` if a file could not be read. `bad expression`, after a
message saying what is wrong, for a pattern that isn't one. `usage` for bad arguments.

A script can therefore test for a match directly: `if(grep -s pattern file >/dev/null) ...`.

#### See also

`sort`, `uniq`, `wc`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo one >a; echo two >b
% grep o a b; grep -c o a; grep -l t a b
a:one
b:two
1
b
% grep z a; echo status $status
status no matches
% grep '[' a
grep: bad expression: no ]
```

## head

A program: prints the first lines of files.

#### Syntax

```text
head [-N] [file ...]
```

#### Arguments

`-`*N*
: How many lines: a number. 10 if it is not given.

#### Description

The first *N* lines of each file are written to standard output, or all of a file that has fewer. With no
files, standard input's. Several files are printed one after another with nothing between them.

#### Exit status

Empty on success. `1` if a file can't be read. `usage` for an option that isn't a number.

#### See also

`tail`, `more`, `cat`.

## hello

A program: a test program, which greets its arguments.

#### Syntax

```text
hello [word ...]
```

#### Description

`hello` is the program `init` runs first when the system starts, to show that programs load and run. It prints
`hello, ` and its first argument, and ends on purpose with the exit code 7 and the status `bye`.

#### Exit status

`bye`, always.

## hwtest

A program: restarts the Hydra into its hardware test.

#### Syntax

```text
hwtest
```

#### Description

The machine is reset and the hardware test, which is in the ROM, runs in place of HydraOS (section 5.7).
Everything that was running stops, as at any reset.

#### See also

Section 5.7.

## hylang

A program: hylang.

#### Syntax

```text
hylang [-l] [file]
```

#### Arguments

*file*
: A file of hylang to run. When it has been read to its end, hylang ends.

`-l`
: Start as a login shell: build the namespace and run the profile, `/lib/hylang/profile.hl`, first (section
  8.16).

#### Description

With no file, hylang starts at its `hylang>` prompt, where each expression typed is evaluated and its value
printed after `=>`. `exit` leaves it.

#### Exit status

`1` if the file can't be read.

#### See also

Chapter 5 of this book; Chapters 21 and 22.

## kill

A program: ends tasks, by their numbers.

#### Syntax

```text
kill [-i] task ...
```

#### Arguments

*task*
: A task's number, as `ps` shows it.

`-i`
: Interrupt the task instead of ending it.

#### Description

Each task is sent a note. Without `-i` it is the kill note, which no program can catch or ignore: the task
ends at once, its files are closed and its memory is freed. With `-i` it is the interrupt note, the one
<kbd>CTRL</kbd>-<kbd>C</kbd> sends: a program that handles it can tidy up, or carry on.

#### Exit status

Empty if every task was sent its note. `1` if any is not a task: `no such task`, or `invalid argument` for
something that is not a number. `usage` with no task.

#### See also

`slay`, `ps`.

#### Example

<!-- session
rc
-->

```session
% sleep 30 &
% kill $apid; echo status $status
status
% kill 99
kill: 99: no such task
```

## label

A program: shows or sets a disk's label.

#### Syntax

```text
label disk [word ...]
```

#### Arguments

*disk*
: The disk's name: the name it has under `/sd`, or the letter of a built-in disk.

*word*
: The new label: the words, joined with one space between each.

#### Description

With no words, the disk's label is printed. With words, it is set. The label is a name kept in the file system
itself, which `df` shows; it travels with a card.

#### Exit status

Empty on success. `1` if the disk isn't there or can't be written. `usage` with no disk.

#### See also

`df`, `mkfs`.

## ls

A program: lists files.

#### Syntax

```text
ls [-ld] [name ...]
```

#### Arguments

*name*
: A file or a directory. The current directory if none is given.

`-l`
: The long form: a line of details for each.

`-d`
: List a directory itself, not what is in it.

#### Description

For a directory, each entry in it is listed, one to a line, in the directory's own order, which is the order
the entries were made in and not alphabetical. A directory's name has a `/` after it. For a file, its own name
is listed.

The long form's line has five parts: the mode, the device, the length in bytes, the date and time the file was
last changed, and the name.

```text
d-rw-rw-rw- fr        0 2000-01-01 00:00 lib
```

The mode's first character is `d` for a directory and `-` otherwise; the second is `a` for an append-only file.
Then come three groups of `r`, `w` and `x`, for reading, writing and running, as Plan 9 lists them; HydraOS has
no users, so the three groups are always alike. A file that can't be written, such as anything on the ROM disk,
has no `w`. The device is the letter of the driver the file belongs to, and the letter or digit of its disk if
it has one: `fr` is the file system on the RAM disk, `fx` on the ROM disk, and `c-` the console.

#### Exit status

Empty on success. `1` if a name isn't there; the others are still listed. `usage` for an unknown option.

#### See also

`du`, `df`, `pwd`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo one >a; mkdir d
% ls
bin/
lib/
a
d/
% ls -ld d nothere
d-rw-rw-rw- fr        0 2000-01-01 00:00 d
ls: nothere: not found
```

## mkdir

A program: makes directories.

#### Syntax

```text
mkdir [-p] dir ...
```

#### Arguments

`-p`
: Make the directories on the way too, as needed, and don't complain about one that exists.

#### Description

Each directory is made, empty. Without `-p`, the directory it goes in must exist, and the directory itself must
not.

#### Exit status

Empty on success. `1` if any could not be made: `already exists`, `not found` (the directory it would go in),
`read-only`.

#### See also

`rmdir`, `rm`, `ls`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% mkdir d; mkdir d; echo status $status
mkdir: d: already exists
status 1
% mkdir p/q
mkdir: p/q: not found
% mkdir -p d p/q; ls p
q/
```

## mkfs

A program: makes a new, empty file system on a disk.

#### Syntax

```text
mkfs [-fp] disk [word ...]
```

#### Arguments

*disk*
: The disk's name: the name it has under `/sd`, or the letter of a built-in disk.

*word*
: The new file system's label.

`-f`
: A full format: write the whole of the free map. Without it only what is needed is written, which is much
  quicker on a large card.

`-p`
: Make the file system in a new partition of its own, after the partitions a card already has, leaving them
  alone.

#### Description

**Everything on the disk is lost.** `mkfs` does not ask first. The disk gets a HydraFS file system with nothing
in it, which the storage driver makes.

#### Exit status

Empty on success. `1` if there is no such disk, or it can't be done. `usage` with no disk.

#### See also

`fsck`, `label`, `df`; Chapter 14.

## mods

A program: lists the modules in the paged ROM.

#### Syntax

```text
mods
```

#### Description

One line is printed for each module, under a heading: the ROM bank or banks it is in, its type, and its name. A
module that shares a bank with others is shown as the bank, a `+`, and the page in the bank where it starts.

The types are `program` (run by name), `driver`, `library` (code that programs call), and `boot`: a driver that
the kernel starts itself as the system comes up.

Programs that are not modules, the ones in `/rom/bin`, are files on the ROM disk and are not listed here: `ls
/rom/bin` shows those.

#### Exit status

Empty.

#### See also

`ps`; section 17.10.

## more

A program: shows text a screen at a time.

#### Syntax

```text
more [file ...]
```

#### Description

Each file, or standard input if there are none, is shown 22 lines at a time. After each 22 lines `more` prints
`--more--` and waits for a line from the console: Return alone shows the next 22, and `q` then Return stops.

The answer is read from the console itself, not from standard input, so `more` works at the end of a pipe: `ls
/rom/bin | more`.

Text that ends before 22 lines is printed and `more` ends without asking.

#### Exit status

Empty on success. `1` if a file can't be read.

#### See also

`head`, `tail`, `cat`, `edit`.

## mv

A program: moves or renames files.

#### Syntax

```text
mv from to
mv from ... dir
```

#### Arguments

*from*
: A file or directory to move. There can be several if the last name is a directory.

*to*
: The new name. If it is an existing directory, *from* goes into it under its own name.

#### Description

**Within one directory**, a move is a rename: the entry's name is changed, and nothing is copied. A file
already at the new name is removed first. Files and directories can both be renamed.

**To another directory**, a file is copied there and the original is then removed. A directory can't be moved
to another directory: `mv` refuses, as Plan 9's does. Copy it with `cp -r` and remove the original with `rm
-r`.

Moving a file to itself does nothing.

#### Exit status

Empty on success. `1` if anything could not be moved. `usage` with fewer than two names.

#### Errors

Each is printed as `mv: name: reason`.

| Reason | Cause |
|---|---|
| `not found` | *from* isn't there, or a directory in *to*'s path isn't |
| `is a directory` | *from* is a directory, and *to* is in another directory |
| `read-only` | A disk that can't be written |

#### See also

`cp`, `rm`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo one >a; echo two >b; mkdir d e
% mv a c; mv c b; cat b
one
% mv b d; ls d
b
% mv d e; echo status $status
mv: d: is a directory
status 1
```
