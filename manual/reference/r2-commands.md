# Chapter 2. Commands

A command is a program in `/bin`, or one of the shell's own, which are at the end of the chapter. This chapter
describes each one's command line: what it takes, what it does with it, what it prints when it can't, and the
status it ends with. The languages and the larger tools have chapters of their own in the manual, to which their
entries here point.

**The exit status.** Every program ends with a status, which rc keeps in `$status` (section 8.9). It is empty
for success. For failure it is a short text: most often `1`, and `usage` when the arguments made no sense, in
which case the program has printed its usage line. A few programs have statuses of their own, and their entries
say so.

**Messages.** What a program has to say about a failure goes to standard error, not into a pipe's data, as
`name: what: why`. The *why* is usually the system's own text for an error (Appendix G): `not found`,
`read-only`, `is a directory`.

**Options** are single letters after a `-`, and letters can be joined: `-ld` is `-l -d`. They come before the
other arguments. `-` alone is not an option, and unless an entry says otherwise it is not standard input either:
it is a file called `-`.

**Paths.** A path in an argument can be at most 63 characters long once it has been made absolute.

**Standard input.** The programs that read files read standard input when they are given none, which is how
they are used in a pipe.

## as

A program: the 65C02 assembler.

#### Syntax

```text
as [-bl] {file.s | -} [out | -]
```

#### Arguments

*file.s*
: The source. `-` reads the source from standard input.

*out*
: The file to write. If it is left out, the source's name without its `.s`; for a source read from standard
  input, standard output. `-` writes to standard output.

`-l`
: Write a labels file too, *out*`.lbl`: every label and its address, which `db` and `dis -l` read. It needs a
  named output.

`-b`
: Write the bytes alone, with no program header, assembled from the address `.org` gives, or $0800 if there is
  none.

#### Description

The source is in ca65's assembly language (Chapter 27). Without `-b` the output is a program in the form the
system loads and runs: `as hi.s` makes `hi`, which `./hi` runs. The assembler is the same code that assembles
the `ASM` blocks of a HyBASIC program.

#### Exit status

Empty on success. `1` if the source can't be read, or has a mistake, which is reported as `as: file:line:
message`. `usage` for bad arguments.

#### See also

`dis`, `db`; Chapter 27.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo ' lda #' >bad.s
% as bad.s; echo status $status
as: bad.s:1: a bad expression
status 1
```

## basic

A program: HyBASIC.

#### Syntax

```text
basic [-l] [file [argument ...]]
```

#### Arguments

*file*
: A program to load and run. When it ends, HyBASIC ends. The arguments after it are the program's own, which it
  reads with `ARG$` and `COMMAND$`.

`-l`
: Start as a login shell would: make the default namespace and run the profile first (section 8.16).

#### Description

With no file, HyBASIC starts at its prompt, where lines are run as they are typed and numbered lines build a
program. `SYSTEM` or `BYE` leaves it.

#### Exit status

At the prompt: the status given to `SYSTEM` or `BYE`, empty if none. Running a file: empty if the program ran
to its end; the status given to `SYSTEM`; `1` if the file can't be read or the program stopped with an error,
which is printed as `file:line: message`.

#### See also

Chapter 1 of this book; Chapters 23 and 24.

## calc

A program: works out an expression in the Hydra's numbers.

#### Syntax

```text
calc [-b base] [-d digits] [expression ...]
```

#### Arguments

*expression*
: The arguments, joined with spaces, are one expression. With none, `calc` reads standard input and works out
  each line, until the end of the input.

`-b` *base*
: Print answers in this base: a base string (section 18.4).

`-d` *digits*
: The number of significant digits for results that can't be exact, 1 to 100. 12 if it is not given.

#### Description

The numbers are those of Chapter 18: integers of any length, exact fractions, decimals and complex numbers. The
operators are `+`, `-`, `*`, `/`, `%` (the remainder of dividing integers) and `^` (a power), with parentheses.
The functions, each applied to what follows it, are `sqrt`, `exp`, `log`, `sin`, `cos`, `tan`, `atan`, `abs`,
`floor`, `round`, `truncate`, `fib`, `numerator`, `denominator`, `re`, `im`, `rational` and `random`; and with
two arguments in parentheses, `gcd`, `pow`, `complex` and `fixed`. The constants are `pi`, `e` and `i`. The
functions and powers take complex numbers too, and give their principal values (`sqrt(3+4i)` is `2+1i`); `abs` of a
complex number is its modulus (`abs(3-4i)` is 5).

Each answer is printed on a line of its own. A mistake in one expression is reported and, when lines are being
read, the next line is gone on with.

The shell reads the command line first. `^`, `(`, `)`, `#`, `*` and `'` all mean something to rc, so an
expression that uses them goes in quotes: `calc '2^10'`.

#### Exit status

Empty on success. The message itself for an expression that failed: `division by zero`, `not a base`. `usage`
for a bad option.

#### See also

Chapter 18.

#### Example

```session
/> calc 1/3 + 1/6
1/2
/> calc -b x 255
FF
/> calc -d 5 sqrt 2
1.4142
/> calc 'abs(3-4i)'
5
/> calc 1 / 0
calc: division by zero
```

## cat

A program: copies files to standard output, one after another.

#### Syntax

```text
cat [file ...]
```

#### Description

Each file is read and written to standard output, 512 bytes at a time, in the order given. Nothing is added
between them and nothing is changed: `cat` copies bytes, and is as good for a program or a picture as for text.
With no files it copies standard input.

A file that can't be read is reported, and `cat` goes on with the next.

Anything that can be read can be given: a device (`cat /dev/time`), or a directory, which reads as its files'
64-byte records.

#### Exit status

Empty if every file was copied. `1` if any could not be read. `write error` if standard output could not be
written: a full disk, or a pipe whose reader has gone.

#### Notes

`-` is not standard input here: `cat -` looks for a file named `-`.

#### See also

`more`, `head`, `tail`, `xd`, `cp`, `tee`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo one >a; echo two >b
% cat a nothere b; echo status $status
one
cat: nothere: not found
two
status 1
```

## cmp

A program: compares two files, byte by byte.

#### Syntax

```text
cmp file1 file2
```

#### Description

The two files are read together. If every byte is the same and they end together, `cmp` prints nothing. At the
first byte that differs, it prints the files' names and the byte's position, counted from 1, and stops. If one
file ends while they still agree, it says which.

#### Exit status

Empty if the files are the same. `1` if they differ, or if either can't be read. `usage` unless there are
exactly two names.

#### See also

`cp`, `xd`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo one >a; echo two >b; echo onetwo >ab
% cmp a a; echo status $status
status
% cmp a b; echo status $status
a b differ: byte 1
status 1
% cmp a ab
a ab differ: byte 4
```

## cp

A program: copies files, and with `-r` directories.

#### Syntax

```text
cp [-r] from to
cp [-r] from ... dir
```

#### Arguments

`-r`
: Copy directories too: each one's whole tree.

*from*
: A file to copy, or with `-r` a directory. There can be several if the last name is a directory.

*to*
: The copy's name. If it is an existing directory, the copy goes into it under *from*'s own name.

*dir*
: An existing directory. Each *from* is copied into it under its own name.

#### Description

Each *from* is handled in turn, and one that fails doesn't stop the rest.

A file is copied by opening it for reading, creating the destination, and reading and writing 512 bytes at a
time until a read returns nothing. Creating the destination empties a file that is already there. The copy is a
new file: it has its own date, the time of copying, and is not read-only even if the original is.

Before copying, `cp` compares the two files' device, disk and file identifier. If they are the same file, by
whatever two names, it refuses to copy it onto itself, which would otherwise empty it.

Anything that can be opened and read can be a *from*, a device included: `cp /dev/time now` copies what one read
of the clock gives.

Without `-r`, a *from* that is a directory is reported and skipped. With `-r`, the directory is made at the
destination (one already there is used as it is), and then everything in it is copied in the same way, down to
16 directories deep.

#### Exit status

Empty if every file was copied. `1` if any was not. `usage`, after a line saying how `cp` is used, if there are
fewer than two names or an option other than `-r`.

#### Errors

Each is printed as `cp: name: reason`, where *name* is the file that could not be read or written.

| Reason | Cause |
|---|---|
| `not found` | *from* isn't there, or a directory in *to*'s path isn't |
| `is a directory` | *from* is a directory, and there's no `-r` |
| `not a directory` | Several *from*s, and the last name isn't a directory |
| `the same file` | *from* and *to* are one file |
| `read-only` | The destination's disk can't be written (`/rom`) |
| `name too long` | The destination's path would pass 63 characters |

Any other reason is the system's own error message for what failed (Appendix G): a full disk, for one.

#### Notes

If a read or a write fails part-way, the destination is left holding what was written up to then.

`cp` doesn't ask before it overwrites.

#### See also

`mv`, `rm`, `mkdir`, `cat`, `cmp`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo hello >a.txt
% cp a.txt b.txt; cat b.txt
hello
% mkdir d; cp a.txt b.txt d; ls d
a.txt
b.txt
% cp d e; echo $status
cp: d: is a directory
1
% cp -r d e; ls e
a.txt
b.txt
% cp a.txt a.txt
cp: a.txt: the same file
% cp a.txt /rom/x
cp: /rom/x: read-only
% cp a.txt; echo $status
usage: cp [-r] from to, or cp [-r] from ... dir
usage
```

## date

A program: prints the date and the time.

#### Syntax

```text
date [-n]
```

#### Arguments

`-n`
: Print the time as one number: the seconds since the start of 1 January 2000.

#### Description

Without `-n`, the date and time are printed as `yyyy-mm-dd hh:mm:ss`, which is what `/dev/time` reads as. The
clock is the system's; on a Hydra with no clock chip it starts at 2000-01-01 00:00:00 when the system does.

`date` doesn't set the clock. Write the time to it: `echo 2026-10-03 15:04:05 >/dev/time`.

#### Exit status

Empty on success. `usage` for any argument but `-n`.

#### See also

`sleep`; section 17.5.

## db

A program: the debugger.

#### Syntax

```text
db program [argument ...]
db -p task
```

#### Arguments

*program*
: A program to start under the debugger, stopped before its first instruction, with its arguments.

`-p` *task*
: Attach to a task that is already running, by its number.

#### Description

`db` shows a program's registers and memory, runs it an instruction or a call at a time, and stops it at
breakpoints. Its own commands are typed at its `db>` prompt (Chapter 29).

#### Exit status

`usage` with no arguments.

#### See also

`as`, `dis`, `ps`; Chapter 29.

## df

A program: lists the disks, with their sizes and the space free.

#### Syntax

```text
df
```

#### Description

One line is printed for each disk the storage driver has started, under a heading: its name (the name it has
under `/sd`, or the letter of a built-in disk), its kind, the size of its file system, how much of that is
free, and its label. A disk that has no file system is listed with its own size.

Arguments are ignored.

#### Exit status

Empty.

#### See also

`du`, `fsck`, `label`, `mkfs`; Chapter 14.

#### Example

```session
/> df
disk  kind   size        free        label
0     sdhc   16380 KB    15612 KB    HYDRA-16
x     rom    671 KB      2 KB        ROM
r     ram    255 KB      253 KB      RAM
s     sram   511 KB      510 KB      SRAM
```

## dis

A program: the disassembler.

#### Syntax

```text
dis [-cnw] [-l labels] [-o addr] file
```

#### Arguments

*file*
: A program, as `as` makes one, or with `-o` a file of raw bytes.

`-l` *labels*
: Use the names in a labels file (`as -l`'s) for the addresses they belong to.

`-n`
: Don't name the system calls and the call registers: show their addresses.

`-w`
: Wide: set the instructions out in the columns the system's own sources use.

`-c`
: Treat every byte as code. Without it, `dis` follows the program from its entry and takes what is never
  reached as data.

`-o` *addr*
: The file has no program header: it is bytes to be loaded at *addr*, given in hex.

#### Description

The program is written to standard output as assembly language that `as` accepts: `dis prog | as - copy` makes
a copy that is byte for byte the same (Chapter 28).

#### Exit status

Empty on success. `1` if the file can't be read, and for bad arguments, after the usage line.

#### See also

`as`, `db`, `xd`; Chapter 28.

## du

A program: reports how much space files take.

#### Syntax

```text
du [-a] [name ...]
```

#### Arguments

*name*
: A file or a directory. The current directory if none is given.

`-a`
: Print a line for every file as well as for every directory.

#### Description

For each name, `du` adds up the lengths of everything in its tree, each file's length rounded up to a whole
kilobyte, and prints a line for each directory with the total of what is in and under it: the number of
kilobytes, a tab, and the path. The deepest directories come first and the name itself last. A name that is a
file gets its own line.

The figure is the files' lengths, not the space the file system gives them, which is in whole clusters and is
a little more.

#### Exit status

Empty on success. `1` if a name isn't there. `usage` for an unknown option.

#### See also

`df`, `ls`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo 12345 >s1; mkdir t; echo x >t/s2
% du -a s1 t
1       s1
1       t/s2
1       t
```

## echo

A program: prints its arguments.

#### Syntax

```text
echo [-n] [word ...]
```

#### Arguments

`-n`
: Don't end the line.

#### Description

The words are written to standard output with one space between each, and then a line end. The spaces are
`echo`'s: however many there were between the words on the command line, the shell has already made them
separate arguments. To keep spaces, quote them.

`-n` is the only option, and only as the first argument. Anything else that starts with `-` is a word and is
printed.

The whole line is written in one piece (up to 256 bytes at a time), so two programs writing lines to the same
place don't mix their lines' halves.

#### Exit status

Empty on success. `write error` if the output can't be written.

#### See also

`cat`; section 8.3.

#### Example

```session
/> echo a  b   c; echo -n x; echo y; echo -x z
a b c
xy
-x z
```
