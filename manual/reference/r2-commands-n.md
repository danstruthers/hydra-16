## new-window

A program: opens a window and runs a command in it.

#### Syntax

```text
new-window [-g] [command [argument ...]]
```

#### Arguments

*command*
: What to run in the window. With none, a shell: the one the system starts in each window.

`-g`
: Put the new window in a group of its own. Without it, the window joins the group of the window the command
  was typed in (Chapter 10).

#### Description

A new window is made and shown, and the command is started in it with the window as its standard input, output
and error. The command is run by a login shell (`rc -l -c`), so it has the default namespace and the profile's
settings, and `$window` in its environment is the new window's number.

`new-window` does not wait: it ends as soon as the command has been started, and the prompt returns in the old
window. The new window closes by itself when the command ends.

The command has a note group of its own, so <kbd>CTRL</kbd>-<kbd>C</kbd> typed in one window doesn't reach the
programs of another.

#### See also

Chapter 10; section 13.5.

## ns

A program: shows a task's namespace.

#### Syntax

```text
ns [task]
```

#### Arguments

*task*
: A task's number. This shell's own task if it is left out.

#### Description

The namespace is printed as the `bind` and `mount` commands that would build it, in the order they were made
(section 9.8). It is read from the task's file in `/proc`.

#### Exit status

Empty on success. `usage` for more than one argument, or a number that is no task's.

#### See also

`bind`, `mount`, `newns`; Chapter 9.

#### Example

```session
/> ns | head -3
bind '#/' /
bind '#/dev' /dev
bind -a '#n' /dev
```

## play

A program: plays a song, or a line of music.

#### Syntax

```text
play [-l] song [n]
play -o score.mml song.zsm
play [-lx] -m channel mml ...
play [-lx] -c channel note ...
```

#### Arguments

*song*
: A ZSM file, a score in MML (a name ending `.mml`), or on a Vera X a WAV file.

*n*
: Play the song's repeating part this many more times.

`-l`
: Play again and again, until <kbd>CTRL</kbd>-<kbd>C</kbd>.

`-o`
: Don't play: turn the MML score into a ZSM file.

`-m` *channel*
: Play the words that follow as one line of MML on that channel.

`-c` *channel*
: Play the notes that follow as a chord: one on each channel from *channel* on.

`-x`
: Read the MML in the Commander X16's dialect.

#### Description

`play` reads the music and sends the sound driver its notes at the times they fall due, and ends when the song
does. It is the program HyBASIC's `PLAY` statement runs. Chapter 15 describes the sound channels and MML.

#### Exit status

Empty when the song has been played. `1` if the file can't be read or the music has a mistake in it, which is
reported. `usage` for bad arguments.

#### See also

Chapter 15.

## ps

A program: lists the tasks.

#### Syntax

```text
ps [-a]
```

#### Arguments

`-a`
: Show each program's arguments after its name.

#### Description

One line is printed for each task in use, under a heading:

| Column | Meaning |
|---|---|
| `task` | The task's number, 0 to 15 |
| `state` | `ready` to run; `wait`ing for a child to end; `sleep`ing; waiting for an `event`; or `idle`, a driver with nothing asked of it |
| `parent` | The task that started it; `-` for those the kernel started |
| `cpu` | The processor time it has had, in seconds to a tenth |
| `group` | Its note group: the tasks a <kbd>CTRL</kbd>-<kbd>C</kbd> reaches together |
| `name` | The program it is running |

Task 0 is the kernel. The drivers are the tasks at the top of the range, and the programs a user starts are
given the lowest free numbers. `ps` typed at a prompt runs in the shell's own task (chapter 2's introduction),
so it isn't in its own list; in a pipeline it is.

Arguments other than `-a` are ignored.

#### Exit status

Empty.

#### See also

`top`, `kill`, `slay`, `ns`; section 17.8.

## pwd

A program: prints the current directory.

#### Syntax

```text
pwd
```

#### Description

The current directory's full path is written to standard output, with a line end. Arguments are ignored.

A program's current directory is its own, copied from the program that started it; `pwd` shows the one it was
started with, which is the shell's.

#### Exit status

Empty.

#### See also

`cd`, `ls`.

## rc

A program: the shell.

#### Syntax

```text
rc [-l] [-i] [file [argument ...]]
rc [-l] -c commands
```

#### Arguments

*file*
: A script: a file of commands to run. The arguments after it are the script's, `$1`, `$2` and so on, with the
  file's name as `$0`.

`-c` *commands*
: Run the commands in this one argument, then end.

`-l`
: A login shell: build the default namespace and run the profile, `/rom/lib/profile`, before anything else.

`-i`
: Interactive: print the prompt and carry on after errors, even when the commands come from a file.

#### Description

With no file and no `-c`, rc reads commands from standard input, printing its prompt first if that is a
console. It ends at the end of its input (<kbd>CTRL</kbd>-<kbd>D</kbd> at the keyboard) or at `exit`.

rc's language is described in Chapter 8, and its own commands at the end of this chapter.

#### Exit status

The status of the last command it ran, or the status given to `exit`. `1` if the file can't be read.

#### See also

`forth`, `hylang`, `basic`; Chapter 8.

#### Example

```session
/> rc -c 'echo one; exit 3'
one
```

## rm

A program: removes files and directories.

#### Syntax

```text
rm [-rf] name ...
```

#### Arguments

`-r`
: Remove a directory and everything in it, to any depth.

`-f`
: Say nothing about what can't be removed, and don't count it a failure.

#### Description

Each name is removed. A directory is removed only if it is empty, unless `-r` is given, in which case what is
in it is removed first, from the bottom up.

There is no question first and nothing can be brought back.

A file that is open, in any task, is not removed: the system refuses.

#### Exit status

Empty on success. `1` if anything could not be removed and `-f` was not given. `usage` for an unknown option.

#### Errors

Each is printed as `rm: name: reason`.

| Reason | Cause |
|---|---|
| `not found` | There is no such name |
| `directory not empty` | A directory with something in it, and no `-r` |
| `read-only` | The file's disk can't be written |
| `busy` | The file is open |

#### See also

`rmdir`, `mv`, `cp`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% mkdir d; echo x >d/f
% rm d; echo status $status
rm: d: directory not empty
status 1
% rm -r d; rm nothere; rm -f nothere; echo status $status
rm: nothere: not found
status
```

## rmdir

A program: removes empty directories.

#### Syntax

```text
rmdir dir ...
```

#### Description

Each directory is removed, if it has nothing in it. Unlike `rm`, it will not remove a file, which makes it the
safe way to say "this should be an empty directory".

#### Exit status

Empty on success. `1` if any could not be removed: `directory not empty`, `not a directory`, `not found`.

#### See also

`rm`, `mkdir`.

## slay

A program: ends tasks, by the name of the program they are running.

#### Syntax

```text
slay [-i] name ...
```

#### Arguments

*name*
: A program's name, as `ps` shows it.

`-i`
: Interrupt the tasks instead of ending them.

#### Description

Every task running a program of that name is sent the kill note, or with `-i` the interrupt note, as `kill`
sends them. `slay` never ends itself.

#### Exit status

Empty if each name matched a task. `1` if a name matched none: `no such task`. `usage` with no name.

#### See also

`kill`, `ps`.

#### Example

<!-- session
rc
-->

```session
% sleep 30 &
% slay sleep; echo status $status
status
% slay sleep
slay: sleep: no such task
```

## sleep

A program: waits.

#### Syntax

```text
sleep seconds
```

#### Arguments

*seconds*
: A whole number of seconds.

#### Description

`sleep` does nothing for that long, and ends. It uses no processor time while it waits.
<kbd>CTRL</kbd>-<kbd>C</kbd> ends it sooner.

#### Exit status

Empty when the time is up. `usage` if the argument is missing or is not a whole number: `sleep 0.5` is not
accepted.

#### See also

`date`.

## sort

A program: sorts lines.

#### Syntax

```text
sort [-bfnru] [file ...]
```

#### Arguments

| Option | Effect |
|---|---|
| `-b` | Ignore spaces and tabs at the start of each line when comparing |
| `-f` | Fold: treat capitals and small letters as the same |
| `-n` | Compare the numbers that the lines start with, not their text |
| `-r` | Reverse the order |
| `-u` | Unique: print one of each set of lines that compare equal |

#### Description

All the lines of all the files (or of standard input) are read into memory, sorted, and written to standard
output. The order is that of the bytes' values, so every capital comes before every small letter, and digits
before both, unless `-f` is given.

With `-n`, a line's number may have a sign and a decimal point; lines are in the order of their numbers'
values, so 9 comes before 10, where plain sorting puts `10` first.

#### Exit status

Empty on success. `1` if a file can't be read; the others are still sorted. `usage` for an unknown option.

#### Notes

The text must fit in memory: `sort` is for files of a size a person would edit, not for megabytes.

#### See also

`uniq`, `grep`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo 10 >n; echo 9 >>n; echo -5 >>n; echo 2.5 >>n
% sort n | tail -2; sort -n n | tail -2
2.5
9
9
10
```

## tail

A program: prints the last lines of a file.

#### Syntax

```text
tail [-N] [file]
```

#### Arguments

`-`*N*
: How many lines: a number. 10 if it is not given.

*file*
: One file. Standard input if it is left out.

#### Description

The file's last *N* lines are written to standard output, or all of it if it has fewer.

The file is read through to its end, with the most recent 16K of it kept in memory, and the lines are found
there. What `tail` prints can therefore never be more than the last 16K of the file, however many lines are
asked for.

#### Exit status

Empty on success. `1` if the file can't be read. `usage` for more than one file, or an option that isn't a
number.

#### See also

`head`, `more`.

## tee

A program: copies its input to files as well as to its output.

#### Syntax

```text
tee [-a] [file ...]
```

#### Arguments

`-a`
: Add to the end of each file. Without it, each file is made, or emptied if it exists.

#### Description

Standard input is copied to standard output and, at the same time, to each file. It is for the middle of a
pipe, to keep a copy of what is passing: `ls -l | tee listing | wc -l`.

A file that can't be opened or written is reported, and `tee` goes on with the others and with its output.

#### Exit status

Empty on success. `1` if any file could not be written.

#### See also

`cat`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo x | tee t1 /rom/no; echo status $status; cat t1
tee: /rom/no: read-only
x
status 1
x
```

## top

A program: shows which tasks are using the processor.

#### Syntax

```text
top
```

#### Description

Once a second, `top` clears the screen and lists the tasks, each with its number, its state, its name, and the
share of the processor it took in that second, as a percentage. It goes on until <kbd>CTRL</kbd>-<kbd>C</kbd>.

The kernel's share, task 0's, includes the time when nothing needed the processor.

#### See also

`ps`.

## touch

A program: makes empty files, or marks files as just changed.

#### Syntax

```text
touch name ...
```

#### Description

A name that doesn't exist is made, as an empty file. A name that does exist has its time of last change set to
now, and nothing else about it altered.

#### Exit status

Empty on success. `1` if any could not be made or changed.

#### See also

`ls`, `date`.

## uniq

A program: removes repeated lines that are next to each other.

#### Syntax

```text
uniq [-c] [file]
```

#### Arguments

`-c`
: Put before each line the number of times it came in a row.

#### Description

The file, or standard input, is copied to standard output with each run of identical lines shown once. Only
lines that follow one another are compared: the same line appearing again later, after a different one, is
printed again. To remove every repeat, sort first: `sort file | uniq`.

A line is compared as its first 255 bytes; a longer line is treated as more than one.

#### Exit status

Empty on success. `1` if the file can't be read. `usage` for more than one file or an unknown option.

#### See also

`sort`, `wc`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo a >u; echo a >>u; echo b >>u; echo a >>u
% uniq -c u
      2 a
      1 b
      1 a
```

## wc

A program: counts lines, words and bytes.

#### Syntax

```text
wc [-lwc] [file ...]
```

#### Arguments

`-l`, `-w`, `-c`
: Print only the lines, the words, or the bytes. They can be combined, and the counts always come in that
  order, whatever order the letters are in.

#### Description

For each file, or for standard input, `wc` prints its number of lines, of words and of bytes, each in a column
seven characters wide, and then the file's name. With more than one file, a line of totals follows.

A line is counted at each line end, so a last line with no line end is not counted. A word is a run of
anything but spaces, tabs and line ends.

#### Exit status

Empty on success. `1` if a file can't be read. `usage` for an unknown option.

#### See also

`grep`, `sort`, `uniq`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo one two three >a; echo -n 'no newline' >c
% wc a c
      1       3      14 a
      0       2      10 c
      1       5      24 total
% wc -l <a
      1
```

## xd

A program: shows a file's bytes in hexadecimal.

#### Syntax

```text
xd [file ...]
```

#### Description

Each file, or standard input, is printed 16 bytes to a line: the offset of the line's first byte, in hex; the
bytes, each as two hex digits; and the same bytes as text, with a `.` for any that isn't a printable character.
Each file's offsets start again at 0.

#### Exit status

Empty on success. `1` if a file can't be read.

#### See also

`cat`, `cmp`, `dis`.

#### Example

```session
/> echo hello | xd
0000000  68 65 6c 6c 6f 0a                                hello.
```

## xmodem

A program: sends or receives a file over the serial port, by the XMODEM protocol.

#### Syntax

```text
xmodem -r file
xmodem -s [-k] file
```

#### Arguments

`-r` *file*
: Receive: the terminal program at the other end sends, and what arrives is written to *file*.

`-s` *file*
: Send *file* to the terminal program.

`-k`
: Send in blocks of 1K, with a CRC. Without it, in blocks of 128 bytes.

#### Description

XMODEM is the simplest of the file transfer protocols that terminal programs offer, and any of them can be the
other end. Start `xmodem` on the Hydra, then start the transfer in the terminal program within a minute.

While the transfer runs, the serial line carries nothing else: the console holds back the screen's output
until it is over, and keys are not acted on.

Receiving, `xmodem` asks first for a CRC and, if the sender doesn't answer to that, for a plain checksum; it
takes blocks of 128 bytes or of 1K as they are sent. Each block is acknowledged if it arrived whole and asked
for again if it didn't. XMODEM pads the last block; the padding bytes are not written to the file.

When it has finished it prints the file's name and its length.

#### Exit status

Empty when the file has been transferred. `1` otherwise, after a message: `too many errors` (a block failed ten
times), `cancelled` (the other end, or <kbd>CTRL</kbd>-<kbd>C</kbd> before the first block), `no answer`
(nothing for a minute). `usage` for bad arguments.

#### Notes

`/pc` (section 9.9) is a far easier way to move files to and from a PC that runs the Hydra's own PC tool.
`xmodem` is for when there is only an ordinary terminal program.

#### See also

Sections 9.9 and 14.7.

## The system's own programs

One program is started by the system itself, and is not meant to be typed.

`init`
: The first program, and the first program of every window. The kernel starts it as the system comes up; again
  for each window the user opens (<kbd>CTRL</kbd>-<kbd>]</kbd> <kbd>C</kbd>), with the window's number as its
  argument; and again whenever the first window's shell ends. It puts its standard input, output and error on
  its window's console, builds its namespace, and then becomes the shell: it runs the shell in its own task, in
  its own place, as `exec` does, so that no task is left behind only to wait for the shell. The shell is the one
  `/lib/shell` names (`/bin/forth -l` in the ROM's), or `rc -l` with no such file. The first time, as the system
  starts, it also starts the RAM disks, sets the windows up from `/lib/windows`, lists the tasks and runs `hello`
  (section 7.4).

Because no task waits for a shell, a task whose parent has ended is simply let go by the kernel when it ends
itself: there is nothing for it to be handed to.

The keyboard and mouse of a Vera X card, the pins and the I²C bus are all served by `kdev`, the kernel's own
device driver, and are not programs or tasks of their own.

## The shell's own commands

These are built into rc. Each changes something about the shell itself, its directory, its variables or its
namespace, which a separate program could not do: a program is another task, and changes only its own.

A built-in command's failure sets `$status` to the reason, and prints it as `rc: command: reason`.

### . (dot)

```text
. file [argument ...]
```

Reads *file* and runs its commands in this shell, as if they had been typed. Unlike running the file as a
script, which starts another rc, anything the commands set stays set here: variables, functions, the current
directory. Inside the file, `$1`, `$2` and `$*` are the arguments given.

Status: that of the file's last command; `not found` if there is no such file.

### bind

```text
bind [-a | -b] [-c] new old
```

Makes the directory or file *new* appear at the name *old* in this shell's namespace (Chapter 9). With no
option *new* replaces what was at *old*; `-a` adds it after what is there, making a union directory, and `-b`
before. `-c` lets files be created in it through the union.

Status: `not found` if either name doesn't exist.

### builtin

```text
builtin command [argument ...]
```

Runs *command* as rc's built-in of that name, or as a program, passing over any function of the same name. It
is how a function that wraps a command calls the real one: `fn cd { builtin cd $*; echo now in `{pwd} }`.

### cd

```text
cd [dir]
```

Makes *dir* the shell's current directory. With no *dir*, it goes to the directory named by `$home`, or to `/`
if `$home` isn't set. Every program the shell starts afterwards begins there.

Status: `not found`; `not a directory`.

### eval

```text
eval [word ...]
```

Joins its words with spaces and runs the result as a command line. The line is read again from scratch, so
variables and quoting in the words are interpreted a second time: `eval echo '$'$v` prints the value of the
variable whose name is in `v`. With no words it does nothing.

### exec

```text
exec command [argument ...]
```

Runs *command* in place of the shell. The shell does not start another task and wait: its own task becomes the
program, with the same task number, files and environment, and there is no shell left to return to. Whatever
started the shell sees the program's status as the shell's.

### exit

```text
exit [status ...]
```

Ends the shell. The words given are its exit status; with none, the status is that of the last command.

### mount

```text
mount [-a | -b] [-c] '#x' old [spec]
```

Attaches a driver's files, named by its device letter, at *old* in this shell's namespace. *spec* chooses among
what the driver has: which disk, for the file system's driver. The options are `bind`'s. The device name is
quoted because `#` starts a comment in rc.

Status: `no such device` if there is no such driver; `not found` if *old* doesn't exist.

### newns

```text
newns
```

Throws away this shell's namespace and builds the default one again, from `/lib/namespace`, as a login shell
does when it starts. It undoes every `bind`, `mount` and `unmount` the shell has made.

The default namespace gives a task its own area of the RAM disk as `/ram`, named for the task's number. A
shell that the system started, the first shell of a window, has that area as its `/ram` already, and `newns`
leaves the files in it alone. A shell started from another shell shares its parent's `/ram` until it says
`newns`: from then on it has a `/ram` of its own, which starts empty (anything a task that had the same number
before it left there is cleared away), and the parent's files are untouched, though no longer at this
shell's `/ram`.

### shift

```text
shift [n]
```

Drops the first *n* words of `$*`, the arguments of the script or function it is used in, so that `$2` becomes
`$1`. *n* is 1 if it is left out. Shifting by more than there are leaves none.

### unmount

```text
unmount [new] old
```

Takes away what was bound or mounted at *old*. With *new*, only that one member of a union directory is taken
out, and the others stay.

Status: `not found` if nothing is bound there.

### wait

```text
wait [task]
```

Waits for a background command to end. With a task's number, such as `$apid`, it waits for that one; with
none, for all of them. A number that is not one of the shell's own children is not an error: there is nothing
to wait for.

### whatis

```text
whatis name ...
```

Says what each name means to the shell, in a form that could be typed back in: a variable as its assignment, a
function as its definition, a built-in as `builtin name`, and a program as the full path it would be run from.
A name that means nothing prints nothing.
