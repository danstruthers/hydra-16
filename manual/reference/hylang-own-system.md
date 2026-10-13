## run, spawn, wait, kill

Functions: run a program and wait for it; start one without waiting; wait for one that was started; end one.

#### Syntax

```text
(run prog x ...)
(spawn prog x ...)
(wait task)
(kill task)
```

#### Arguments

*prog*
: The program: a path, or a bare name, which is looked for in `/bin`.

*x*
: The program's arguments. Each value is turned to text as `print` shows it, and is one argument, whatever
  spaces it holds.

*task*
: A task's number, as `spawn` gave it.

#### Result

`run` starts the program, waits until it ends, and gives its exit code: 0 for success, by the system's
custom, and something else for a failure.

`spawn` starts the program and returns at once with its task number. The program runs alongside hylang, with
the same standard input and output, so what it prints appears on the same screen.

`wait` waits until the task ends, and gives its exit code. Each task can be waited for once, and only by the
program that started it.

`kill` sends the task the kill note, which ends it, and gives `NIL`. The exit code that `wait` then gives is
137 (128 + 9, the kill note's number).

#### Description

The program is started directly, with no shell: no pattern is expanded and nothing is redirected. For a
command line with pipes, patterns or redirection, use `sh`.

A program that reports its own failure still ran, so `run` gives its exit code. `run` gives an *error* only
when the program could not be started.

#### Errors

| Message | Cause |
|---|---|
| *prog*`: not found` | There is no such program |
| `'run' expects a path` | *prog* is not a string |
| `A task must be an integer` | *task* is not a whole number |
| *task*`: no such child` | `wait` for a task that this program did not start, or has already waited for |
| *task*`: no such task` | `kill` of a task that isn't there |

#### See also

`sh`, `sh-out`, `note`, `pid`, `ps`.

#### Example

```session
hylang> (run "echo" "hello" "there")
hello there
=> 0
hylang> (run "ls" "/nothere")
ls: /nothere: not found
=> 1
hylang> (run "nothere")
=> Error: nothere: not found
hylang> (def {tk} (spawn "sleep" "60"))
=> NIL
hylang> (kill tk)
=> NIL
hylang> (wait tk)
=> 137
```

## sh, sh-out

Functions: run a command line with the shell; and the same, giving what it printed.

#### Syntax

```text
(sh line [input])
(sh-out line [input])
```

#### Arguments

*line*
: A command line, as it would be typed at rc's prompt: pipes, patterns, redirection and variables all work.

*input*
: Text to give the command as its standard input. Without it, the command reads what hylang itself would:
  the keyboard.

#### Result

`sh` runs the line (with `rc -c`) and gives its exit status, a number: 0 for success. What the command
prints goes to the screen.

`sh-out` runs it the same way but collects what it writes to standard output, and gives that as a string,
line ends included. What it writes to standard error still goes to the screen, and its exit status is not
given.

#### Description

These are the bridge to everything else on the Hydra: any program, and any pipeline of them, can do part of
a hylang program's work. `str-split` with `"\n"`, or `read-lines` on a file the command wrote, turns the
output into a list.

The command gets a copy of hylang's environment, so a variable set with `setenv` is `$name` in the line.

#### See also

`run`, `output-of`, `setenv`, `str-split`.

#### Example

```session
hylang> (sh "echo hi | wc -c")
      3
=> 0
hylang> (sh "ls /nothere")
ls: /nothere: not found
=> 1
hylang> (sh-out "echo hi")
=> "hi\n"
hylang> (sh-out "cat" "from input")
=> "from input"
hylang> (str-split (str-trim (sh-out "ls /rom/lib | head -3")) "\n")
=> {"as/" "basic/" "edit/"}
```

## pid

A function: this task's number.

#### Syntax

```text
(pid)
```

#### Result

The number of the task hylang is running in, 1 to 15. It is the number `ps` shows for it, and the name of
its directory in `/proc`.

#### See also

`task-info`, `ps`, `note`.

#### Example

```session
hylang> (pid)
=> 2
hylang> ((task-info (pid)) :name)
=> "hylang"
```

## env, setenv, unsetenv

Functions: read, set and remove an environment variable.

#### Syntax

```text
(env [name])
(setenv name value)
(unsetenv name)
$name
```

#### Arguments

*name*
: The variable's name, a string.

*value*
: A string; a list of strings, for a variable of several words; or a number, which is turned to text.

#### Result

`(env name)` gives the variable's value: a string, or a list of strings if it holds several words, as rc's
`$path` does. If there is no such variable, `NIL`. `$name` is another way to write `(env "name")`.

`(env)`, with no name, gives a hash of all the variables, with their names, as strings, for keys.

`setenv` and `unsetenv` give `NIL`. Removing a variable that isn't there is not an error.

#### Description

The environment is this task's own set of variables (section 17.7). hylang starts with a copy of the
environment of the shell that started it, and the programs it starts get a copy of its own: so `setenv` is
how a setting is passed down to a program run with `run` or `sh`. Nothing set here goes back up to the
shell.

Environment values are text. A number put in comes back as a string: use `val` to make it a number again.

#### Errors

| Message | Cause |
|---|---|
| `'setenv' expects a path` | *name* is not a string |

#### See also

`sh`, `run`, `val`.

#### Example

```session
hylang> (env "path")
=> {"." "/bin"}
hylang> (setenv "colour" "red")
=> NIL
hylang> $colour
=> "red"
hylang> (sh "echo my colour is $colour")
my colour is red
=> 0
hylang> (unsetenv "colour")
=> NIL
hylang> (env "colour")
=> NIL
```

## time, date, date-parts, seconds-of

Functions: the clock, and conversions between a count of seconds and a date.

#### Syntax

```text
(time)
(date [t])
(date-parts [t])
(seconds-of year month day [hour minute second])
```

#### Arguments

*t*
: A time, as a number of seconds since the start of 1 January 2000. Without it, now.

*year*, *month*, *day*
: A date: the year 2000 or later, the month 1 to 12, the day 1 to the month's last.

*hour*, *minute*, *second*
: A time of day. Without them, midnight.

#### Result

| Function | Result |
|---|---|
| `time` | The clock, now: the number of seconds since 2000-01-01 00:00:00 |
| `date` | The time as a string, `"2026-10-13 09:41:00"`: the form `/dev/time` uses |
| `date-parts` | The time as a hash: `:year`, `:month`, `:day`, `:hour`, `:minute`, `:second`, and `:weekday`, which is 0 for Sunday to 6 for Saturday |
| `seconds-of` | The number of seconds since 2000 of the date and time given |

#### Description

A time is a plain number, so times are compared with `<` and subtracted with `-`: the difference is in
seconds. A file's `:mtime`, from `stat`, is a time of the same kind.

The Hydra's clock starts at the beginning of 2000 each time it is switched on, unless it has a clock chip or
its time has been set (section 17.5): so on a board without one, `(time)` is the number of seconds since it
was started.

#### Errors

| Message | Cause |
|---|---|
| *year*`: invalid argument` | `seconds-of` given a date that doesn't exist, or a year before 2000 |
| `a time must be an integer` | *t* is not a whole number |

#### See also

`ticks`, `clock`, `sleep`, `stat`.

#### Example

```session
hylang> (date 0)
=> "2000-01-01 00:00:00"
hylang> (seconds-of 2026 10 13 9 41 0)
=> 845199660
hylang> (date 845199660)
=> "2026-10-13 09:41:00"
hylang> ((date-parts 845199660) :weekday)
=> 2
hylang> (- (seconds-of 2026 12 25) (seconds-of 2026 10 13))
=> 6307200
```

## ticks, tick-rate, clock

Functions: the system's tick count, how many ticks there are in a second, and the time since hylang started.

#### Syntax

```text
(ticks)
(tick-rate)
(clock)
```

#### Result

`ticks` gives the system's tick count: a number from 0 to 32,767, which goes up by one at each tick of the
system's timer and starts again from 0 when it passes 32,767.

`tick-rate` gives the number of ticks in a second: 200.

`clock` gives the number of seconds since this hylang started, as a decimal, to the nearest tick (a two
hundredth of a second).

#### Description

For timing a piece of a program, take `(clock)` before and after, and subtract.

`ticks` is the raw count, which a few system calls and `sleep-until` use. It wraps every 164 seconds, so the
difference of two readings must be taken with that in mind: `clock` does that work, and is the one to use
for anything longer than a moment.

#### See also

`time`, `sleep`, `sleep-until`.

#### Example

The numbers depend on the moment:

```text
hylang> (tick-rate)
=> 200
hylang> (def {t0} (clock))
=> NIL
hylang> (fib 5000)
=> 3878968454388325633701916308325905312082127714646245106160597214895550139044037097010822916462210669
...
hylang> (- (clock) t0)
=> 1.385
```

## sleep, sleep-until, yield

Functions: wait for a time; wait until a tick; and give other tasks a turn.

#### Syntax

```text
(sleep s)
(sleep-until tick)
(yield)
```

#### Arguments

*s*
: A number of seconds, 0 or more. It may be a fraction or a decimal: `(sleep 1/10)`.

*tick*
: A value of the tick count, as `ticks` gives it, at most 32,767 ticks ahead.

#### Result

Each gives `NIL`, when the wait is over. While hylang sleeps it uses none of the processor's time, and the
other tasks run.

`sleep-until` waits until the tick count reaches *tick*. A loop that adds a fixed step to a tick value and
sleeps until it keeps an even beat, however long each turn of the loop takes: `sleep`, which waits from when
it is called, would drift.

`yield` does not wait: it ends hylang's turn at once, so that any other task that is ready runs before
hylang goes on.

#### Description

A note that arrives during a sleep ends it early. <kbd>CTRL</kbd>-<kbd>C</kbd> does: the sleep's result is
then the error `interrupted`.

#### Errors

| Message | Cause |
|---|---|
| `'sleep' expects a number of seconds` | *s* is negative, or not a number |

#### See also

`ticks`, `clock`, `on-note`, `key?`.

#### Example

```session
hylang> (sleep 1/2)
=> NIL
hylang> (dotimes {i 3} (do (write i " ") (sleep 0.1)))
0 1 2 => NIL
```

## platform, hydra?

Functions: which system hylang is running on.

#### Syntax

```text
(platform)
(hydra?)
```

#### Result

On the Hydra-16, `platform` gives the atom `:hydra`, and `hydra?` gives `T`.

#### Description

hylang is the Hydra's version of a language that also runs on a PC (danlang). A program meant for both asks
these before it uses anything that only the Hydra has: its devices, its RAM banks, its system calls.

#### Example

```session
hylang> (platform)
=> :hydra
hylang> (hydra?)
=> T
```

## sysinfo, mods, ps, task-info, errstr, free

Functions: facts about the system: the machine, the ROM's modules, the tasks, an error's message, and the
memory.

#### Syntax

```text
(sysinfo)
(mods)
(ps)
(task-info task)
(errstr code)
(free)
```

#### Arguments

*task*
: A task's number, 0 to 15.

*code*
: A system error code, as a number, 0 to 255.

#### Result

| Function | Result |
|---|---|
| `sysinfo` | A hash: `:abi`, the version of the system's calls; `:ram-modules`, how many RAM modules are fitted; `:free-tasks`, a list of the task numbers not in use |
| `mods` | A list of hashes, one for each module in the paged ROM: `:name`, `:type` (`:program`, `:driver` or `:library`), `:bank` (its first ROM bank) and `:banks` (how many it has) |
| `ps` | A list of hashes, one for each task in use, as `task-info` gives |
| `task-info` | A hash for one task: `:task`, its number; `:name`; `:state`; `:parent`, the task that started it (no entry if none did); `:cpu`, the processor time it has had, in ticks; `:group`, its note group; `:args`, its arguments, a list of strings (no entry if it has none) |
| `errstr` | The system's message for the error code, a string |
| `free` | A hash of memory sizes, in kilobytes: `:ram`, this task's own RAM banks; `:shared`, the shared RAM that segments can use; `:used` and `:free`, how much of that is and isn't in segments; `:segments`, how many segments there are |

A task's `:state` is one of `:ready`, `:wait` (for a child to end), `:call` (in a driver's call), `:idle` (a
driver with nothing to do), `:new`, `:sleep`, `:blocked` and `:event`.

#### Description

These give, as values a program can use, what the commands `mods`, `ps` and `free` print. Being lists of
hashes, they are searched and sorted with the list functions.

#### Errors

| Message | Cause |
|---|---|
| *task*`: no such task` | `task-info` of a task number that isn't in use |
| `'task-info' was given a number out of its range` | *task* is not 0 to 15 |
| `'errstr' expects an error's code` | *code* is not 0 to 255 |

#### See also

`pid`, `banks`, `error-code`; the commands `ps`, `mods` and `free` (Chapter 2 of this book).

#### Example

```session
hylang> ((sysinfo) :ram-modules)
=> 3
hylang> (errstr 32)
=> "not found"
hylang> ((task-info 0) :name)
=> "kernel"
hylang> (map (fn {p} {p :name}) (ps))
=> {"kernel" "forth" "hylang" "snd" "kdev" "storage" "cons"}
hylang> (map (fn {m} {m :name}) (filter (fn {m} {== (m :type) :driver}) (mods)))
=> {"cons" "storage" "kdev" "snd" "vid"}
hylang> (free)
=> <hash>{{:ram 384} {:shared 1024} {:used 512} {:free 512} {:segments 1}}
```

## peek, poke, peek-word, poke-word

Functions: read and write the task's memory.

#### Syntax

```text
(peek addr)
(poke addr byte)
(peek-word addr)
(poke-word addr n)
```

#### Arguments

*addr*
: An address, 0 to 65,535 (`#xFFFF`).

*byte*
: A whole number from 0 to 255.

*n*
: A whole number from 0 to 65,535.

#### Result

`peek` gives the byte at the address, and `peek-word` the 16-bit word there: the byte at *addr* is its low
half and the byte at *addr* + 1 its high half, as the 65C02 keeps a word. `poke` and `poke-word` store a
byte, or a word in the same order, and give `NIL`.

#### Description

The memory is this task's own 64K, as it is while hylang runs: the task's RAM from `$0000` to `$7FFF`, the
RAM bank it has chosen at `$8000`, a bank of hylang's own code at `$A000`, and the kernel's ROM and the I/O
ports from `$E000` up (Appendix F). Other tasks' memory can't be reached.

Reading is always safe. Writing is not: hylang itself lives in this memory, with its stack, its variables
and its values, and a `poke` into any of them is likely to stop it. `poke` is for the places a program
knows to be its own, such as a block it has set aside, and for the hardware's registers. For a block of
bytes to work in, a buffer, or a RAM bank from `bank-alloc`, is the safe choice.

#### Errors

| Message | Cause |
|---|---|
| `'peek' was given a number out of its range` | The address, or the value, is too big (each function with its own name) |
| `'peek' expects a whole number` | An argument is negative, or not a whole number |

#### See also

`buffer`, `bank-read`, `lo`, `hi`, `word`, `hex`.

#### Example

```session
hylang> (hex (peek-word #xFFFC) 4)
=> "E000"
hylang> (hex (peek #xFFFC) 2)
=> "00"
hylang> (hex (peek #xFFFD) 2)
=> "E0"
```

(`$FFFC` holds the address the 65C02 starts from when it is reset.)

## banks, bank-alloc, bank-free, bank-read, bank-write

Functions: the task's RAM banks: how many it has, taking some, giving them back, and reading and writing
them.

#### Syntax

```text
(banks)
(bank-alloc n)
(bank-free bank n)
(bank-read bank offset n)
(bank-write bank offset bytes)
```

#### Arguments

*n*
: For `bank-alloc` and `bank-free`, a number of banks, 1 or more. For `bank-read`, a number of bytes.

*bank*
: A bank's number: the one `bank-alloc` gave, or one of those after it in the same run.

*offset*
: A position in the bank, 0 to 8,191.

*bytes*
: A list of whole numbers from 0 to 255.

#### Result

`banks` gives the number of RAM banks the task has in all: 16 for each RAM module fitted, each bank 8K.

`bank-alloc` takes *n* banks that are next to one another and gives the number of the first: the others are
that number plus 1, plus 2 and so on. The banks are the task's until it gives them back or ends.

`bank-free` gives *n* banks back, from *bank* on, and gives `NIL`.

`bank-read` gives a list of *n* bytes from the bank, starting at *offset*. `bank-write` stores the bytes in
the bank from *offset* on, and gives `NIL`. The bytes read or written must all be inside the one bank:
*offset* plus the count can't pass 8,192.

#### Description

A task has far more RAM in its banks than the 64K it can see at once (384K, with three modules). These
functions let a hylang program keep data there, outside hylang's own memory: a large table, a picture, a
file's contents. hylang reaches the bank only for the moment of the read or the write, so nothing a program
keeps in a bank is disturbed by hylang, or disturbs it.

hylang keeps some of the task's banks for its own values. `bank-alloc` gives only banks that are free.

#### Errors

| Message | Cause |
|---|---|
| `'bank-read' was given a number out of its range` | The offset is past the bank's end, or the bytes would run past it |
| `'bank-write' expects a list of bytes (0-255)` | *bytes* is not such a list: a string or a buffer must be made a list first, with `bytes` |
| *n*`: invalid argument` | `bank-alloc` of no banks; `bank-free` of banks that aren't taken |
| `'bank-alloc' was given a number out of its range` | More banks than a task can have |

#### See also

`free`, `buffer`, `seg-create`, `peek`.

#### Example

The bank's number depends on what hylang has taken already:

```text
hylang> (banks)
=> 48
hylang> (def {bk} (bank-alloc 2))
=> NIL
hylang> bk
=> 7
hylang> (bank-write bk 0 {1 2 3})
=> NIL
hylang> (bank-read bk 0 4)
=> {1 2 3 0}
hylang> (bank-free bk 2)
=> NIL
```

## seg-create, seg-attach, seg-detach, seg-read, seg-write

Functions: shared memory segments, which several tasks can use together.

#### Syntax

```text
(seg-create banks)
(seg-attach seg)
(seg-detach seg)
(seg-read seg bank offset n)
(seg-write seg bank offset bytes)
```

#### Arguments

*banks*
: The segment's size, in 8K banks: 1 to 128.

*seg*
: A segment's number, as `seg-create` gave it.

*bank*
: Which of the segment's banks, the first being 0.

*offset*
: A position in that bank, 0 to 8,191.

*n*
: A number of bytes.

*bytes*
: A list of whole numbers from 0 to 255.

#### Result

`seg-create` makes a segment of the shared RAM and gives its number. `seg-attach` and `seg-detach` join the
task to a segment and take it away again, and give `NIL`. `seg-read` gives a list of *n* bytes from one of
the segment's banks, and `seg-write` stores bytes there and gives `NIL`. As with a task's own banks, a read
or a write must stay inside one bank.

#### Description

A **segment** is a piece of the Hydra's shared RAM (section 17.9). Unlike a task's own banks, a segment can
be attached by any task that knows its number, so it is how two programs share a large amount of data
without copying it through a pipe: one creates the segment and passes its number, as an argument or in a
file, and both attach it. A segment lasts while any task has it attached, or while it has a name in
`/dev/seg`.

A task must be attached to a segment to read or write it. The task that creates one is attached to it
already; any other attaches it with `seg-attach`.

#### Errors

| Message | Cause |
|---|---|
| *seg*`: invalid argument` | A read or a write of a segment that isn't attached |
| *seg*`: out of range` | *bank* is not one of the segment's banks |
| `'seg-read' was given a number out of its range` | The bytes would run past the bank's end; or a number is too big for what it is |
| *banks*`: invalid argument` | `seg-create` of no banks |

#### See also

`bank-alloc`, `free`; section 17.9.

#### Example

The segment's number depends on what is already in use:

```text
hylang> (def {sg} (seg-create 1))
=> NIL
hylang> sg
=> 14
hylang> (seg-attach sg)
=> NIL
hylang> (seg-write sg 0 0 {1 2 3})
=> NIL
hylang> (seg-read sg 0 0 3)
=> {1 2 3}
hylang> (seg-detach sg)
=> NIL
```

## bind, mount, unmount, ns

Functions: change, and list, the task's namespace.

#### Syntax

```text
(bind new old [:before | :after] [:create])
(mount dev old [spec] [:before | :after] [:create])
(unmount old [new])
(ns)
```

#### Arguments

*new*
: The directory, or file, to make appear at *old*.

*old*
: The place in the tree that is changed. It must be there already.

*dev*
: A device's name, such as `"#f"`, with *spec* choosing which of the device's trees: `"x"` is the ROM disk.

`:before`, `:after`
: Make a union: *old* shows what it showed before and *new*'s contents too, with *new* looked in first
  (`:before`) or last (`:after`). With neither, *new* replaces what *old* showed.

`:create`
: In a union, a file created at *old* is made in this member.

#### Result

`bind`, `mount` and `unmount` give `NIL`.

`unmount` undoes what was bound or mounted at *old*: all of it, or with *new*, only that member of a union.

`ns` gives the namespace as a list of hashes, one for each bind and mount, in the kernel's order: `:old`, the
place; `:new`, what is there, as a device's path such as `"#fx/lib"`; and `:create`, `T`, on a member that
takes creates.

#### Description

These are the system's `BIND`, `MOUNT` and `UNMOUNT` calls, and work as rc's `bind`, `mount` and `unmount`
commands do (Chapter 9 of the manual explains namespaces, and Chapter 2 of this book has the commands'
entries).

The namespace is this task's. Programs that hylang starts afterward see the same tree; the shell that
started hylang does not see the changes, and they are gone when hylang ends. `(newns)` puts the default
namespace back.

In a union with no member that takes creates, a new file is made in the directory that was there first.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | *new* or *old* isn't there; or `unmount` of a place where nothing is bound |
| `'bind' expects :before, :after or :create` | Another atom was given |
| *dev*`: no such device` | `mount` of a device that isn't there |

#### See also

`newns`, `ls`, `cd`; the commands `bind`, `mount`, `unmount` and `ns` (Chapter 2 of this book).

#### Example

```session
hylang> (mkdir "/ram/a")
=> NIL
hylang> (mkdir "/ram/b")
=> NIL
hylang> (write-file "/ram/a/one" "1")
=> NIL
hylang> (write-file "/ram/b/two" "2")
=> NIL
hylang> (bind "/ram/a" "/ram/b" :after)
=> NIL
hylang> (ls "/ram/b")
=> {"one" "two"}
hylang> (unmount "/ram/b")
=> NIL
hylang> (ls "/ram/b")
=> {"two"}
hylang> (bind "/ram/a" "/ram/b")
=> NIL
hylang> (ls "/ram/b")
=> {"one"}
```

## note, note-group, on-note

Functions: send a note to a task, or to a group of tasks; and choose what hylang does when one arrives.

#### Syntax

```text
(note task n)
(note-group group n)
(on-note f)
```

#### Arguments

*task*
: A task's number.

*group*
: A note group's number, as `task-info`'s `:group` gives it.

*n*
: The note: one of the atoms `:interrupt`, `:kill`, `:hangup`, `:alarm` and `:brk`, or a note's number, 1 to
  31. The numbers 16 to 31 are for programs' own purposes.

*f*
: A function of one argument, or `NIL`.

#### Result

Each gives `NIL`.

#### Description

A **note** is a short message from one task to another, or from the system: <kbd>CTRL</kbd>-<kbd>C</kbd> is
the note `:interrupt`, sent to every task in the window's group. `note` sends one to a task, and
`note-group` to every task of a group, hylang itself included if it is in it.

What a note does to the task that gets it is that task's choice. Left alone, hylang treats `:interrupt` as
the error `interrupted`, which stops what it is doing and returns to the prompt, and ends when any other
note arrives, as most programs do.

`on-note` changes that. From then on, each note that arrives is passed to *f*, as an atom for the named
notes and a number for the others. If *f* returns a true value, the note has been dealt with and the
program goes on. If it returns `NIL`, the note has its usual effect. `(on-note NIL)` goes back to the usual
effect for all of them.

*f* is not called at the instant the note arrives, but at hylang's next step after it, so it can do anything
a function can. The `:kill` note can't be caught: it always ends the task.

A task ended by a note has the exit code 128 plus the note's number: 130 for `:interrupt`, 137 for `:kill`.

#### Errors

| Message | Cause |
|---|---|
| *task*`: no such task` | There is no such task |
| `'note' expects a note: :interrupt, :kill, :hangup, :alarm, :brk, or 1-31` | *n* is not a note |
| `'on-note' expects a function or NIL` | *f* is neither |

#### See also

`kill`, `wait`, `sleep`, `key`; the commands `kill` and `slay` (Chapter 2 of this book).

#### Example

```session
hylang> (def {got} {})
=> NIL
hylang> (on-note (fn {n} {do (set! {got} (cons n got)) T}))
=> NIL
hylang> (note (pid) :alarm)
=> NIL
hylang> (note (pid) 20)
=> NIL
hylang> got
=> {20 :alarm}
hylang> (def {k} (spawn "sleep" "30"))
=> NIL
hylang> (note k :interrupt)
=> NIL
hylang> (wait k)
=> 130
```

## sys

A function: makes a system call. Each call also has a function of its own, named `sys-` and the call's
name.

#### Syntax

```text
(sys name x ...)
(sys-name x ...)
```

#### Arguments

*name*
: The call's name in small letters, with `-` where the call has `_`: an atom, a symbol or a string. `OPEN` is
  `:open`, and `SLEEP_UNTIL` is `:sleep-until`.

*x*
: The call's inputs, in the order its entry in Chapter 3 of this book gives them. A string is passed as the address of
  its text.

#### Result

The call's output: a number, or a list of them if the call gives several. A call with no output gives `NIL`.
If the call fails, the result is an error, with the system's message, and its code as the error's code.

#### Description

`(sys :getpid)` and `(sys-getpid)` are the same call, written two ways. The first form takes the name as a
value, so it can be chosen as the program runs; the second reads like any other function.

These reach everything the kernel offers, at the level a program in assembly language works at: file
descriptors are numbers, and nothing is closed for you. Most programs never need them, because the
functions of this chapter (`open`, `read-line`, `spawn`, `sleep` and the rest) are built on the same calls
and look after the details. They are for the calls that have no function of their own.

"From the languages", in each call's entry in Chapter 3 of this book, says whether a call can be made this way.

#### Errors

| Message | Cause |
|---|---|
| `'sys' has no call :bogus` | There is no call of that name |
| `'sys-open' takes at least 1 argument, not 0` | Too few inputs for the call |
| The system's message | The call failed |

#### See also

Chapter 3 of this book; `error-code`, `errstr`.

#### Example

```session
hylang> (sys-getpid)
=> 2
hylang> (sys :getppid)
=> 1
hylang> (def {fd} (sys-open "/rom/lib/shell" 0))
=> NIL
hylang> fd
=> 3
hylang> (sys-close fd)
=> NIL
hylang> (error-code (sys-open "/nothere" 0))
=> :noent
```
