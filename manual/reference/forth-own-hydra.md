## >z, zcount

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
>z       ( c-addr u -- z-addr )
zcount   ( z-addr -- c-addr u )
```

#### Description

The system's calls take a name as a **zero-terminated string**: its characters, and then a byte of 0. Forth's
strings are an address and a length. These two convert between them.

`>z` copies a string, puts a zero after it, and leaves the copy's address, ready for a `sys-` word. The copy
is in one of two buffers, used in turn, so two names can be held at once, for a call that takes two; a
third `>z` overwrites the first. A string can be 127 characters at most.

`zcount` goes the other way: given the address of a zero-terminated string, it leaves that address and the
number of characters before the zero.

#### See also

`sys-`, `count`, `s"`.

#### Example

```session
/> lib hydra
/> s" /rom/lib/shell" >z 0 sys-open throw value fd
/> fd sys-close .
0
/> s" hello" >z zcount type
hello
```

## sh

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
sh   ( c-addr u -- status )
```

#### Description

Runs a command line, given as a string, with the shell rc (`rc -c`), waits for it to finish, and leaves its
exit status: 0 for success. Pipes, patterns, redirection and variables in the line are all rc's to deal
with.

This is the way to run a command from inside a definition, where a line can't simply be typed. For a single
program with plain arguments, `run` does without the shell.

#### See also

`run`, `sh-out`, `%`, `status`.

#### Example

```session
/> lib hydra
/> s" echo hi | wc -c" sh .
      3
0
/> s" ls /nothere" sh .
ls: /nothere: not found
1
```

## run

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
run   ( c-addr u -- status )
```

#### Description

Runs a program, given as a string holding its name and its arguments with spaces between them, waits for it,
and leaves its exit code. The line is split at its spaces and nothing more: there is no quoting, no pattern
and no redirection. A name with no `/` in it is looked for in `/bin`.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −544 | `not found` | There is no such program |

#### See also

`sh`, `spawn`, `wait`.

#### Example

```session
/> lib hydra
/> s" echo one two" run .
one two
0
/> s" nothere" run .
not found
```

## ctl

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
ctl   ( c-addr1 u1 c-addr2 u2 -- )
```

#### Description

Writes the second string to the file named by the first, in a single write, and closes it. It is how a
command is sent to a device's control file: the same thing as `echo -n text >file` at the shell.

A failure is thrown, and reported with the file's name: the file isn't there, or the driver refused the
command.

#### See also

`write-file`, `sh`; Chapter 12.

#### Example

```session
/> lib hydra
/> s" /dev/gpio/ctl" s" out 2" ctl
/> s" /dev/gpio/ctl" s" in 2" ctl
/> s" /dev/gpio/ctl" s" fly" ctl
/dev/gpio/ctl: invalid argument
```

## bank-window

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
bank-window   ( -- addr )
```

#### Description

A constant: `$8000`, the address where the task's chosen RAM bank appears. The 8K from there to `$9FFF` is a
window on to one bank at a time, which `bank!` and `seg-bank!` choose.

#### See also

`bank!`, `seg-bank!`, `code-banks`; Appendix F.

## bank!, bank@

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
bank!   ( bank -- )
bank@   ( -- bank )
```

#### Description

`bank!` chooses which of the task's RAM banks is seen at `bank-window`, and `bank@` leaves the number of the
one chosen. A task has 16 banks of 8K for each RAM module fitted. A program asks for the ones it will use
with `sys-banks-alloc`, which leaves the number of the first of a run, and gives them back with
`sys-banks-free`.

With a bank chosen, the 8K at `bank-window` is ordinary memory, for `@`, `!`, `move` and the rest. This is
how a HyForth program keeps far more data than its dictionary has room for.

#### Notes

HyForth keeps the code of colon definitions in RAM banks too, at the same window. A definition that uses
`bank!` would switch away the very bank it is running from, so one compiled there is the error −21,
`unsupported operation`, when it runs. Say `false code-banks` before defining any word that uses `bank!` or
`seg-bank!`: such words are then compiled into the dictionary, and work. At the prompt `bank!` can be used
freely.

#### See also

`bank-window`, `code-banks`, `seg-bank!`, `allocate`.

#### Example

```session
/> lib hydra
/> 2 sys-banks-alloc throw value bk
/> false code-banks
/> : keep  ( n -- )  bk bank!  bank-window ! ;
/> : kept  ( -- n )  bk bank!  bank-window @ ;
/> true code-banks
/> 1234 keep kept .
1234
/> bk 2 sys-banks-free .
0
```

## code-banks

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
code-banks   ( flag -- )
```

#### Description

Says where the code of the colon definitions made from now on is to go. With a true flag it goes into RAM
banks of HyForth's own, as it does when HyForth starts: that leaves the dictionary for names and data, and
lets a program be much bigger than the dictionary. With a false flag it goes into the dictionary, in
ordinary memory.

Code in the dictionary is needed for words that change the bank window themselves, with `bank!` or
`seg-bank!`. Switch to false, define those words, and switch back.

#### See also

`bank!`, `here`, `unused`.

## seg-bank!

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
seg-bank!   ( seg n -- ior )
```

#### Description

Puts bank *n* of a shared segment at `bank-window`: the bank is then ordinary memory there, as one of the
task's own is after `bank!`. The segment must be one this task is attached to: `sys-seg-create` makes one,
and attaches its maker, and `sys-seg-attach` attaches another task that has its number.

Like `bank!`, it can't be used in a definition compiled into the code banks: see `code-banks`.

#### See also

`bank!`, `bank-window`, `code-banks`; section 17.9.

## argc, arg

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
argc   ( -- n )
arg    ( n -- c-addr u )
```

#### Description

The arguments HyForth was started with, for a program run as `forth file.fs a b`. `argc` leaves how many
there are, the file's name counting as the first: 3 in that case. `arg` leaves argument *n* as a string,
number 0 being the file's name; past the last, the string's length is 0.

At the prompt of an interactive HyForth there are none: `argc` is 0.

#### See also

`included`, `getenv`.

#### Example

```session
/> lib hydra
/> argc .
0
```

## get-dir, set-dir

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
get-dir   ( c-addr1 u1 -- c-addr2 u2 )
set-dir   ( c-addr u -- ior )
```

#### Description

`get-dir` puts the current directory's path into the buffer given, *c-addr1* and its size *u1*, and leaves
it as a string: as much of it as fits. `set-dir` makes the directory named the current one, and leaves 0, or
the reason it could not.

`set-dir` is the shell's `cd` in a form that works inside a definition. The current directory is where
names that don't begin with `/` are looked for.

#### See also

`cd`, `open-dir`, `=mkdir`.

#### Example

```session
/> lib hydra
/> s" /rom/lib" set-dir .
0
/rom/lib> pad 80 get-dir type
/rom/lib
/rom/lib> s" /nothere" set-dir .
-544
/rom/lib> s" /" set-dir drop
```

## =mkdir

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
=mkdir   ( c-addr u mode -- ior )
```

#### Description

Makes a directory. *mode* is the permission bits to give it; 0 leaves them to the system. The ior is 0, or
the reason: −545, `already exists`; −544 if the directory it is to go in isn't there.

The name has an `=` so that `mkdir` typed at the shell's prompt is still the `mkdir` program.

#### See also

`set-dir`, `open-dir`, `delete-file`.

#### Example

```session
/> lib hydra
/> s" /ram/newdir" 0 =mkdir .
0
/> s" /ram/newdir" 0 =mkdir .
-545
```

## ior>text

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
ior>text   ( ior -- c-addr u )
```

#### Description

Leaves the system's message for an ior, the result that the file words and the `sys-` words leave: for −544,
`not found`. For a number that is not a system error's, the standard's throw codes among them, the string
is empty.

`throw` prints the same message. `ior>text` is for a program that wants to word its own report.

#### See also

`throw`, `open-file`, `sys-`.

#### Example

```session
/> lib hydra
/> -544 ior>text type
not found
/> s" /ram/nothere" r/o open-file nip ior>text type
not found
```

## note, note-group

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
note         ( task n -- )
note-group   ( group n -- )
```

#### Description

Send note number *n* to a task, or to every task of a note group. The notes with meanings of their own are 1
(interrupt, which is what <kbd>CTRL</kbd>-<kbd>C</kbd> sends), 3 (hangup), 4 (alarm) and 9 (kill, which a
task cannot refuse); 16 to 31 are for programs to use between themselves.

What a note does is up to the task that gets it. Most programs end. A failure, such as a task that isn't
there, is thrown.

#### See also

`on-note`, `spawn`, `wait`; the command `kill`.

#### Example

```session
/> lib hydra
/> sleep 30 &
/> s" apid" getenv evaluate 9 note
/> 99 4 note
no such task
```

## on-note

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
on-note   ( xt -- )
```

#### Description

Names the word that is to be given the notes this HyForth receives. The word must take the note's number
and leave a flag, ( *n* -- *flag* ): true if it has dealt with the note and the program should go on, false
if the note should be treated as <kbd>CTRL</kbd>-<kbd>C</kbd> is, and stop what is running (the throw code
−28).

The word is not called at the instant a note arrives, but at the next word the interpreter runs or the next
turn of a loop, so it may do anything a word can.

<kbd>CTRL</kbd>-<kbd>C</kbd> itself and the kill note are not passed to it. `0 on-note` takes the word
away again: with none, any note but <kbd>CTRL</kbd>-<kbd>C</kbd> ends HyForth.

#### See also

`note`, `catch`.

#### Example

```session
/> lib hydra
/> : heard  ( n -- flag )  ." note " . true ;
/> ' heard on-note
/> sys-getpid 20 note
note 20
/> 0 on-note
```

## pause

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
pause   ( -- )
```

#### Description

Gives up the rest of HyForth's turn of the processor, so that any other task that is ready runs now. HyForth
goes on at its next turn. A loop that is only waiting for something another task will do should `pause`
each time round, or better, wait with `ms`.

#### See also

`ms`, `key?`.

## sys

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
sys   ( addr a x y -- a x y p )
```

#### Description

Calls machine code. The 65C02's A, X and Y registers are loaded from the low bytes of *a*, *x* and *y*, and
the code at *addr* is called as a subroutine. When it returns, with `RTS`, the three registers as it left
them are on the stack, and above them the processor's flags, *p*: the carry is bit 0.

The code can be bytes laid down with `c,`, a routine loaded from a file, or an entry of the system's.
HyForth's own data stack is kept in the zero page from `$22` up, indexed by X: code that uses zero page must
keep clear of it. (The system's calls are easier to make with the `sys-` words.)

#### See also

`sys-`, `c,`, `disasm`; Chapter 26.

#### Example

The three bytes are `LDA #$2A` and `RTS`:

```session
/> lib hydra
/> create code  $A9 c, $2A c, $60 c,
/> code 0 0 0 sys drop 2drop .
42
```

## %

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
%   ( "line" -- )
```

#### Description

Gives the rest of the line to rc as a command, whatever its first word is.

At HyForth's prompt a line is taken as Forth if its first word is a Forth word or a number, and as a command
for rc otherwise. So a program whose name is also a Forth word can't be run just by typing it: `words`,
`free` (once the memory library is loaded), or any word you have defined yourself. `%` settles it: what
follows is a command.

The command's exit code is then in `status`. A line that ends in `&` is not waited for, as in rc.

#### See also

`sh`, `status`, `wait`; section 8.14.

#### Example

```session
/> : free  ." my own word" ;
/> free
my own word
/> % free
ram     384 KB a task (3 modules)
shared  1024 KB, 512 KB in segments (1), 512 KB free
```

## status

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
status   ( -- n )
```

#### Description

Leaves the exit code of the last command that was run and waited for: a command line typed at the prompt, a
`%` line, or a task that `wait` waited for. 0 means it succeeded.

#### See also

`%`, `wait`, `sh`, `exits`.

#### Example

```session
/> ls /nothere
ls: /nothere: not found
/> status .
1
/> echo fine
fine
/> status .
0
```

## exits

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
exits   ( n -- )
```

#### Description

Ends HyForth, with *n* as its exit code: what the program that started it sees as its status. `bye` ends it
with 0. A script run as a command uses `exits` to say whether it succeeded.

#### See also

`bye`, `status`, `exit`.

## sh-out

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
sh-out   ( c-addr u -- c-addr2 u2 )
```

#### Description

Runs a command line with rc, as `sh` does, and leaves what it wrote to standard output as a string. Its exit
code is in `status`. What it wrote to standard error still goes to the screen.

The string is in the free space above `here`, and holds as much of the output as fits there. It is good
only until something is added to the dictionary: use it, or copy it, before defining anything.

#### See also

`sh`, `output-of`, `piped`, `status`.

#### Example

```session
/> s" echo hi" sh-out type
hi
/> s" ls /rom/lib/shell" sh-out nip .
15
```

## output-of

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
output-of   ( xt -- c-addr u )
```

#### Description

Runs a Forth word, and leaves what it printed as a string: nothing appears on the screen. It is `sh-out` for
HyForth's own output. The string is in the free space above `here`, as `sh-out`'s is, so the word run must
not add to the dictionary.

If the word throws an error, the error is thrown again when the output has been put back.

#### See also

`sh-out`, `piped`, `type`.

#### Example

```session
/> : say  ." from forth" ;
/> ' say output-of nip .
10
/> ' say output-of type
from forth
```

## spawn

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
spawn   ( c-addr u -- task )
```

#### Description

Starts a program, given as a string of its name and arguments as `run` takes them, and does not wait for
it: the task's number is left, and both go on at once. The program has a note group of its own, and its
task number is also put in the environment variable `apid`, as rc does for a command started with `&`.

`wait` waits for it to end.

#### See also

`wait`, `run`, `note`, `status`.

#### Example

```session
/> s" sleep 1" spawn wait status .
0
```

## wait

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
wait   ( task -- )
```

#### Description

Waits until a task that this HyForth started has ended: one from `spawn`, or from a command line that ended
in `&`. Its exit code is then in `status`.

#### See also

`spawn`, `status`.

## getenv, setenv, unsetenv

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
getenv     ( c-addr1 u1 -- c-addr2 u2 )
setenv     ( c-addr1 u1 c-addr2 u2 -- )
unsetenv   ( c-addr u -- )
```

#### Description

The environment's variables, which HyForth shares with the commands it runs (section 17.7).

`getenv` leaves the value of the variable named, as a string. A variable that is a list of several words
comes with a space between them. If there is no such variable, the length is 0.

`setenv` sets the variable named by the first string to the second, as one word of at most 126 characters.
`unsetenv` removes a variable; one that isn't there is not an error.

The commands HyForth starts get a copy of its environment, so a variable set here is `$name` in a command
line typed afterward.

#### See also

`sh`, `%`, `arg`.

#### Example

```session
/> s" colour" s" red" setenv
/> s" colour" getenv type
red
/> echo my colour is $colour
my colour is red
/> s" colour" unsetenv
/> s" colour" getenv nip .
0
```

## cd

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
cd   ( "dir" -- )
```

#### Description

Changes HyForth's current directory to the one named on the rest of the line. It is a Forth word because it
has to be: a `cd` run by rc as a command would change only that command's own directory.

The name is written as for rc's `cd`: `'`...`'` quotes one with spaces in it, and `$name` is a variable's
value. With no name, the directory is the one in `$home`, if that is set.

The prompt shows the current directory. Inside a definition, use `set-dir`.

#### Errors

| Message | Cause |
|---|---|
| *dir*`: not found` | There is no such directory |

#### See also

`set-dir`, `get-dir`, `prompt`.

#### Example

```session
/> cd /rom/lib
/rom/lib> cd /nothere
/nothere: not found
/rom/lib> cd /
```

## bind, mount, unmount, newns

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
bind      ( "[-a|-b] [-c] new old" -- )
mount     ( "[-a|-b] [-c] #x old [spec]" -- )
unmount   ( "[new] old" -- )
newns     ( -- )
```

#### Description

Change HyForth's own namespace. Each takes the rest of the line as its arguments, written exactly as for
rc's commands of the same names, whose entries in Chapter 2 of this book describe them in full: `bind` puts
one directory at another's place, or joins it to what is there (`-a` after, `-b` before, `-c` taking new
files); `mount` does the same with a device; `unmount` undoes them; `newns` builds the default namespace
again.

They are Forth words because a command run by rc would change only the namespace of that command's task.
Commands started afterward see the namespace as these leave it.

`newns` gives HyForth's `/ram` a fresh start too. In a HyForth that is a window's first shell, that removes
every file in `/ram`: copy what is wanted somewhere else first.

#### See also

`cd`; Chapter 9; `ns`, the command.

#### Example

```session
/> mkdir /ram/a /ram/b
/> echo 1 >/ram/a/one
/> bind /ram/a /ram/b
/> ls /ram/b
one
/> unmount /ram/b
/> ls /ram/b
```

## prompt, prompt2

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
prompt    ( c-addr u -- )
prompt2   ( c-addr u -- )
```

#### Description

Set the text of the prompt. `prompt` sets the one shown when HyForth is ready for a line, and `prompt2` the
one shown while a definition that was begun on an earlier line is still open. Each takes a string of at
most 31 characters, in which these stand for something that changes:

| Written | Shows |
|---|---|
| `%p` | The whole path of the current directory |
| `%d` | The current directory on its disk |
| `%v` | The SD card the directory is on, as `0:`; nothing for a directory that is not on a card |
| `%t` | HyForth's task number |
| `%w` | Its window's number |
| `%%` | A `%` |

HyForth starts with the prompt `%v%d> `, which shows `/ram> ` in the RAM disk and `0:/games> ` on a card, and
with a tab as the second prompt.

#### See also

`cd`, `:`.

#### Example

```text
/> s" [task %t] " prompt
[task 1] s" %v%d> " prompt
/> s" ... " prompt2
/> : unfinished
... 1 2 + ;
/> unfinished .
3
```

## send

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
send   ( "n line" -- )
```

#### Description

Types a line into another window. The first word of the rest of the line is a window's number, and what
follows it is given to that window's shell as if it had been typed there, with <kbd>ENTER</kbd> after it.
A line can be 126 characters at most.

With it one window can drive another: start a long job there, or a program whose output should be kept
apart.

#### See also

`spawn`; Chapter 10; the command `new-window`.
