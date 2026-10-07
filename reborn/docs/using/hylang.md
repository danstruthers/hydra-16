# Using hylang

hylang is danlang on the Hydra-16: a lisp with big integers, exact fractions, hashes and closures, and the Hydra's
files, tasks, memory and devices a function away.  The language is danlang's, exactly: the specification is
danlang's `reference.md` (`C:\source\danlang`, <https://github.com/SNSTRUTHERS/danlang>), and danlang's own
regression suite runs on the Hydra unchanged.  This guide is for using it; [../hylang.md](../hylang.md) is its
design (where it differs from danlang, the Hydra's layers in full, and how it's built).

Contents: [Starting it](#starting-it) · [The basics](#the-basics) · [Numbers](#numbers) · [Lists, strings and
hashes](#lists-strings-and-hashes) · [Control](#control) · [Files and programs](#files-and-programs) · [The
Hydra](#the-hydra) · [Devices](#devices) · [Every system call](#every-system-call) · [Errors and Ctrl-C](#errors-and-ctrl-c)

## Starting it

| Typed at the shell's prompt | What runs |
| :--- | :--- |
| `hylang` | The REPL: each expression typed is evaluated, its value shown after `=> `; `exit` ends it |
| `hylang file.hl a b` | A script: the file run, then hylang ends (`args` is `{"file.hl" "a" "b"}`); a file whose first line is `#!/bin/hylang` runs by its name too |
| `hylang -l` | hylang as a login shell, over rc: a line that starts with `(`, `{` or `[` (or a character right against one: `?(`) is hylang's, any other an rc command line.  Name `/bin/hylang -l` in a card's `/lib/shell` to have it in every window |

```
/> hylang
hylang (danlang on the Hydra-16)
Type 'exit' to Exit

hylang> (+ 1 2)
=> 3
```

At the REPL the console edits the line (Up and Down for the last ones), and an expression that isn't closed asks for
another line.  `(exit n)` ends a script with a status.

## The basics

An S-expression `(f a b)` calls `f` with the values of `a` and `b`; a Q-expression `{a b}` is a list, as it's
written (data, or code to run later); `[a b]` is a list of the values.  Names are case-insensitive.

```
hylang> (def {x} 5)
=> NIL
hylang> (* x x)
=> 25
hylang> (fun {sq n} {* n n})
=> NIL
hylang> (sq 12)
=> 144
hylang> (map (fn {n} {* n n}) {1 2 3})
=> {1 4 9}
```

`def` binds globally, `set` in the current scope, `set!` changes the nearest binding; `fn` makes a function (a
closure); `fun` defines one by name.  A function given fewer arguments than its formals is partially applied: with
`(fun {add a b} {+ a b})`, `(map (add 10) {1 2 3})` is `{11 12 13}`.  Seven prefixes are shorthand for a call: `?(c a b)` is `(if c a b)`, and
`=` `set`, `:` `def`, `#` `hash-create`, `@` `fn`, `.` `unpack`, `~` `format`.

## Numbers

Integers have no fixed size (on the Hydra, up to 255 bytes: some 614 digits), division is exact, and a decimal
written with a point is a fixed decimal, exact too:

```
hylang> (pow 2 100)
=> 1267650600228229401496703205376
hylang> (/ 1 3)
=> 1/3
hylang> (+ 1/3 1/6)
=> 1/2
hylang> (+ 0.1 0.2)
=> 0.3
```

Bases are read with `#` (`#xFF`, `#b101`, `#16rFF`) and written with `(to-str n "x")`; `hex`, `bin`, the bit
functions (`bit-and`, `shl` ...), `lo`, `hi` and `word` are for bytes and words.  Complex numbers, `random`,
`truncate`, `to-fixed`, `div` and `mod` are there too.

## Lists, strings and hashes

```
hylang> (filter even? (range 1 10))
=> {2 4 6 8}
hylang> (len "hydra")
=> 5
hylang> (def {h} #({{:name "hydra"} {:tasks 16}}))
=> NIL
hylang> (h :name)
=> "hydra"
hylang> (format "{} has {} tasks" (h :name) (h :tasks))
=> "hydra has 16 tasks"
```

Lists: `head`, `tail`, `join`, `nth`, `take`, `drop`, `reverse`, `sort`, `map`, `filter`, `foldl`, `find`, `zip`
...  Strings are bytes: `substring`, `index-of`, `str-split`, `str-join`, `str-replace`, `str-upper` ...; `+` of a
string joins what follows as `print` shows it (`(+ "n=" 5)`).  A hash is called with a key to look it up, and a
function in it is a method (`(obj :add 3)`, `&0` the hash).  Buffers are bytes changed in place (`(buffer 16)`).

## Control

`if`, `cond`, `case`, `and`, `or`, `while`, `each`, `dotimes`, `do`, `let`:

```
hylang> (if (> 5 3) "bigger" "smaller")
=> "bigger"
hylang> (each {f (ls "/rom/lib")} (print f))
forth
hylang
namespace
profile
shell
=> NIL
hylang> (dotimes {i 3} (print "line" i))
line 0
line 1
line 2
=> NIL
hylang> (fun {fact n} {if (zero? n) 1 (* n (fact (- n 1)))})
=> NIL
hylang> (fact 30)
=> 265252859812191058636308480000000
```

A branch of `if` written as a Q-expression is that data, not run: write `(if c (a) (b))`, not `{a}`.  A call in tail
position takes its caller's place, so a tail-recursive loop runs in constant space; other calls nest 2,500 deep at
most.

## Files and programs

```
hylang> (write-file "/ram/x.txt" "hello file")
=> NIL
hylang> (read-file "/ram/x.txt")
=> "hello file"
hylang> ((stat "/ram/x.txt") :length)
=> 10
hylang> (ls "/rom")
=> {"README" "bench" "bin" "doc" "lib" "sample" "songs"}
hylang> (sh-out "ls /rom/bin | wc -l")
=> "      6\n"
hylang> (run "ls" "/rom/lib")
forth/
hylang/
namespace
profile
shell
=> 0
```

Files: `read-file`, `read-lines`, `write-file`, `append-file`, `ls`, `dir`, `stat`, `exists?`, `dir?`, `mkdir`,
`remove`, `rename`, `copy-file`, `glob`, `cd`, `cwd`; streams with `open`, `read-line`, `read-byte`, `read-bytes`,
`write-bytes`, `seek`, `close`.  Programs: `(run prog args...)` (its exit code), `(sh line)` (rc's), `(sh-out line)`
(its output), `(spawn prog args...)` and `(wait task)`.  The environment is rc's variables: `$name`, `(env name)`,
`setenv`, `unsetenv`.  The clock: `(time)` (seconds since 2000-01-01), `(date)`, `(ticks)`, `(sleep 1/10)`.

`(load "file")` runs a file; `(use "name")` loads a library once, from `/lib/hylang` (the ROM's, and a card's or the
RAM disk's before it).

## The Hydra

Built in, for what isn't a file:

| Area | Functions |
| :--- | :-------- |
| Tasks | `(ps)` (a hash each), `(task-info t)`, `(pid)`, `(yield)`, `(hold body...)` (no task switch meanwhile) |
| Notes | `(note task :interrupt)`, `(note-group g n)`, `(on-note f)` (each note given to `f`) |
| The namespace | `(bind new old [:before \| :after] [:create])`, `(mount "#f" old [spec])`, `(unmount old [new])`, `(ns)`, `(newns)` |
| Memory | `(peek a)`, `(poke a b)`, `(banks)`, `(bank-alloc n)`, `(bank-read bank ofs n)`, `(bank-write bank ofs bytes)`; shared segments: `(seg-create banks)`, `(seg-read ...)`, `(seg-write ...)`; `(free)` |
| The system | `(sysinfo)`, `(mods)`, `(errstr :noent)` |
| Keys | `(key)` (the next key, raw: a character, or `:up`, `:f1` ...), `(key?)` |

## Devices

Each device has a library over its files, loaded by `use`; each is also an example of driving the device by hand:

| `(use ...)` | For |
| :---------- | :-- |
| `"cons"` | The windows: `(window)`, `(windows)`, `(new-window [cmd])` (`(new-window "top")`: a command run in a window made and shown; none, the shell), `(new-group [cmd])` (in a group of its own), `(show-window n)`, `(window-size)` (`{cols rows}`), `(window-label s)`, `(window-status s)`, `(window-ctl s)`, `(raw-on)`, `(beep)` |
| `"gpio"` | Port A's pins: `(gpio pin)`, `(gpio! pin level)`, `(gpio-port)`, `(gpio-wait)` (CA1's next edge) |
| `"i2c"` | `(i2c-devices)`, `(i2c-read addr n [reg])`, `(i2c-write addr bytes [reg])` |
| `"spi"` | `(spi dev bytes)` (a transaction), `(spi-mode dev 3)` |
| `"snd"` | The YM2151's channels 0-7, and with a Vera X its PSG's 8-23: `(snd-claim 0)`, `(snd-patch 0 n)`, `(snd-note 0 60)`, `(snd-wave 8 :saw)` (a PSG channel's waveform), `(tune {{60 1} {64 1} {67 2}})` (MIDI notes, beats; `(note-of "C4")` a note's number), `(snd-mml 0 "t180 o4 l8 c d e")` and `(snd-chord 0 "o4 c e g")` (the score language: play's `-m` and `-c`), `(play path)` |
| `"disk"` | `(disks)`, `(df)`, `(cards)`, `(disk-start d)`, `(disk-stop d)` |
| `"proc"` | Another task's `(task-args t)`, `(task-cwd t)`, `(task-env t)`, `(task-regs t)`, `(task-mem t addr n)` |
| `"clock"` | `(set-date "2026-10-04 12:00:00")`, `(rtc)` |
| `"pc"` | `(pc?)`: whether the PC tool answers (`/pc`'s files are files) |
| `"screen"` | The terminal: `cls`, `at`, `color`, `bold` |

```
hylang> (use "cons")
=> NIL
hylang> (window)
=> 0
```

## Every system call

Each call a program makes is a function, `sys-` and its name: `(sys-getpid)`, `(sys-open "/rom/README" 0)`.  Its
arguments are the registers the call takes, and its value what it gives (one, or a list; NIL for none); a failure
is the system's error.  `/rom/doc/api.md` lists each call's (the `hylang` column).  `(sys :name args...)` makes any
of them by name.

## Errors and Ctrl-C

An error is a value: it stops what it reaches, and the REPL shows it (`=> Error: ...`).  `(try x handler)` catches
one, `&err` its message and `&code` its code; a system failure's code is an atom (`:noent`, `:exist` ...).

```
hylang> (try (/ 1 0) {+ "caught: " &err})
=> "caught: Division by zero."
hylang> (error-code (read-file "/nosuch"))
=> :noent
```

Ctrl-C is the error `interrupted` (`:intr`) at the next call or loop step, which `try` can catch; `(on-note f)`
gives it to a function instead (T to go on).
