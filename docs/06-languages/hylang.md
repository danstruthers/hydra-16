# hylang

hylang is **danlang on the Hydra-16**: a lisp with integers of any size, exact fractions, hashes and closures, and the
Hydra's files, tasks, memory and devices a function away.  The language is danlang's exactly: its specification is
danlang's `reference.md` (<https://github.com/SNSTRUTHERS/danlang>), and danlang's own regression suite (1,530
checks) runs on the Hydra unchanged.  It's written in 65C02 assembly (`reborn/modules/hylang/`), eight banks of the
paged ROM, with a bytecode machine and native code.

## Contents

1. [Starting it](#starting-it)
2. [The basics](#the-basics)
3. [Numbers](#numbers)
4. [Lists, strings and hashes](#lists-strings-and-hashes)
5. [Control](#control)
6. [Files and programs](#files-and-programs)
7. [The Hydra's built-ins](#the-hydras-built-ins)
8. [The device libraries](#the-device-libraries)
9. [Every system call](#every-system-call)
10. [Errors and Ctrl-C](#errors-and-ctrl-c)
11. [hylang as a shell](#hylang-as-a-shell)
12. [Where hylang differs from danlang](#where-hylang-differs-from-danlang)
13. [How it's built](#how-its-built)

---

## Starting it

| Typed at the shell's prompt | What runs |
| :--- | :--- |
| `hylang` | The REPL: each expression typed is evaluated, its value shown after `=> `; `exit` ends it |
| `hylang file.hl a b` | A script: the file run, then hylang ends (`args` is `{"file.hl" "a" "b"}`); a file whose first line is `#!/bin/hylang` runs by its name too |
| `hylang -l` | hylang as a login shell, over rc ([below](#hylang-as-a-shell)) |

```
/> hylang
hylang (danlang on the Hydra-16)
Type 'exit' to Exit

hylang> (+ 1 2)
=> 3
```

At the REPL the console edits the line (Up and Down for the last ones), and an expression that isn't closed asks for
another line.  `(exit n)` ends a script with a status.

---

## The basics

An **S-expression** `(f a b)` calls `f` with the values of `a` and `b`; a **Q-expression** `{a b}` is a list, as it's
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

* `def` binds globally, `set` in the current scope, `set!` changes the nearest binding.
* `fn` makes a function (a closure); `fun` defines one by name.
* A function given fewer arguments than its formals is **partially applied**: with `(fun {add a b} {+ a b})`, `(map (add
  10) {1 2 3})` is `{11 12 13}`.  Extra arguments past the formals are `&1`, `&2` ...
* **Seven prefixes** are shorthand for a call: `?(c a b)` is `(if c a b)`, and `=` `set`, `:` `def`, `#` `hash-create`,
  `@` `fn`, `.` `unpack`, `~` `format`.

---

## Numbers

Integers have no fixed size (on the Hydra, up to 255 bytes: some 614 digits), **division is exact**, and a decimal
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

**Bases** are read with `#` (`#xFF`, `#b101`, `#16rFF`) and written with `(to-str n "x")` (`FF`; `"#x"` writes `#xFF`)
or `format`'s `{x}`.  A fraction in another base has a radix point where it ends (`(to-str 1/2 "#b")` is `#b0.1`), and
complex numbers are written as they're read, `1+2i`.  `(base "x")` makes a base the one everywhere: numbers are printed
in it and read in it, a program's too (there a bare number starts with a digit: `0FF`), until `(base "d")`.

```
hylang> (base "x")
=> "d"
hylang> (+ 0FF 1)
=> 100
hylang> (base "d")
=> "x"
```

`hex`, `bin`, the bit functions (`bit-and`, `shl` ...), `lo`, `hi` and `word` are for bytes and words.  Complex numbers,
`random`, `truncate`, `to-fixed`, `div` and `mod` are there too, and `(number-bytes n)` and `(bytes-number b)` give a
number's bytes in the stored format every language of the Hydra shares.

**The math functions**, `sqrt`, `exp`, `log`, `sin`, `cos`, `tan`, `atan`, `(pi)` and `pow` of any real power, are exact
when their answer is, and otherwise a fixed decimal of `(digits)` significant digits (12 at the start; `(digits n)`
sets it, 1 to 100), correctly rounded:

```
hylang> (list (sqrt 9/4) (sqrt 2) (pow 8 1/3) (pow 2 0.5))
=> {3/2 1.41421356237 2 1.41421356237}
hylang> (list (sin 1) (log -1) (exp 100))
=> {0.841470984808 3.14159265359i 26881171418200000000000000000000000000000000}
hylang> (digits 30)
=> 12
hylang> (pi)
=> 3.14159265358979323846264338328
```

---

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

* **Lists**: `head`, `tail`, `join`, `nth`, `take`, `drop`, `reverse`, `sort`, `map`, `filter`, `foldl`, `find`, `zip` ...
  Lists, strings and numbers are immutable and shared, never copied.
* **Strings** are bytes: `substring`, `index-of`, `str-split`, `str-join`, `str-replace`, `str-upper` ...; `+` of a string
  joins what follows as `print` shows it (`(+ "n=" 5)`).
* **Hashes** are called with a key to look it up, and a function in one is a method (`(obj :add 3)`, `&0` the hash).
* **Buffers** are bytes changed in place: `(buffer 16)`, `buffer-get`, `buffer-put`, `buffer-fill`, `buffer-copy`.

---

## Control

`if`, `cond`, `case`, `and`, `or`, `while`, `each`, `dotimes`, `do`, `let`:

```
hylang> (if (> 5 3) "bigger" "smaller")
=> "bigger"
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
position takes its caller's place, so a tail-recursive loop runs in constant space; other calls nest 2,500 deep at most.

---

## Files and programs

```
hylang> (write-file "/ram/x.txt" "hello file")
=> NIL
hylang> (read-file "/ram/x.txt")
=> "hello file"
hylang> ((stat "/ram/x.txt") :length)
=> 10
hylang> (sh-out "ls /rom/bin | wc -l")
=> "     10\n"
hylang> (run "ls" "/rom")
README
bin/
doc/
lib/
songs/
=> 0
```

| Area | Functions |
| :--- | :-------- |
| Files | `read-file`, `read-lines`, `write-file`, `append-file`, `ls`, `dir`, `stat` (a hash: `:name`, `:length`, `:dir`, `:mtime`, `:mode`, `:qid`, `:dev`), `exists?`, `dir?`, `file?`, `mkdir`, `remove`, `rename`, `copy-file`, `glob`, `cd`, `cwd` |
| Streams | `open`, `read-line`, `read-byte`, `read-bytes`, `write-bytes`, `seek`, `close`; `stdin`, `stdout`, `stderr` are fds 0-2 |
| Programs | `(run prog args...)` (its exit code), `(sh line)` (rc's), `(sh-out line)` (its output), `(spawn prog args...)`, `(wait task)`, `(kill task)` |
| The environment | `$name`, `(env name)`, `(env)` (all of it), `setenv`, `unsetenv`: rc's variables |
| The clock | `(time)` (seconds since 2000-01-01), `(date)`, `(ticks)`, `(tick-rate)` (200), `(sleep 1/10)`, `(clock)` |
| Keys | `(key)` (the next key, raw: a character, or `:up`, `:f1` ...), `(key?)` |
| Libraries | `(load "file")` runs a file; `(use "name")` loads a library once, from `/lib/hylang` |

---

## The Hydra's built-ins

What isn't a file is built in:

| Area | Functions |
| :--- | :-------- |
| **Tasks** | `(ps)` (a hash each: `:task`, `:name`, `:state`, `:parent`, `:cpu`, `:group`, `:args`), `(task-info t)`, `(pid)`, `(yield)`, `(sleep-until tick)`, `(hold body...)` (no task switch meanwhile) |
| **Notes** | `(note task :interrupt)` (or `:kill`, `:hangup`, `:alarm`, `:brk`, or 16-31), `(note-group g n)`, `(on-note f)`: each note (but a kill) given to `f`, which returns T to go on |
| **Namespaces** | `(bind new old [:before \| :after] [:create])`, `(mount "#f" old [spec])`, `(unmount old [new])`, `(ns)` (a list of hashes), `(newns)` |
| **Memory** | `(peek a)`, `(poke a b)`, `(peek-word a)`, `(poke-word a n)`; `(banks)`, `(bank-alloc n)`, `(bank-free bank n)`, `(bank-read bank ofs n)`, `(bank-write bank ofs bytes)`; shared segments: `(seg-create banks)`, `(seg-attach seg)`, `(seg-detach seg)`, `(seg-read seg bank ofs n)`, `(seg-write seg bank ofs bytes)`; `(free)` |
| **The system** | `(sysinfo)`, `(mods)`, `(errstr :noent)`, `(platform)` (`:hydra`), `(hydra?)` |

---

## The device libraries

Each device has a library in hylang over its files (`/lib/hylang/NAME.hl`), loaded by `use`; each is also a working
example of driving the device by hand:

| `(use ...)` | For |
| :---------- | :-- |
| `"cons"` | The windows: `(window)`, `(windows)`, `(new-window [cmd])`, `(new-group [cmd])`, `(show-window n)`, `(window-size)`, `(window-label s)`, `(window-status s)`, `(window-ctl s)`, `(snarf)`, `(snarf! s)`, `(raw-on)`, `(raw-off)`, `(beep)` |
| `"gpio"` | `(gpio pin)`, `(gpio! pin level)`, `(gpio-in pin)`, `(gpio-out pin)`, `(gpio-port)`, `(gpio-port! byte)`, `(gpio-ddr! byte)`, `(gpio-ca1! :rise)`, `(gpio-wait)`, `(gpio-state)` |
| `"i2c"` | `(i2c-devices)`, `(i2c-read addr n [reg])`, `(i2c-write addr bytes [reg])`, `(i2c-speed khz)`, `(i2c-reg-size n)` |
| `"spi"` | `(spi dev bytes)`, `(spi-read dev n)`, `(spi-mode dev 3)` |
| `"snd"` | `(snd-claim ch...)`, `(snd-patch ch n)`, `(snd-note ch 60)`, `(snd-off ch)`, `(snd-level ch v)`, `(snd-wave 8 :saw)`, `(tune {{60 1} {64 1} {67 2}})`, `(note-of "C4")`, `(snd-mml 0 "t180 o4 l8 c d e")`, `(snd-chord 0 "o4 c e g")`, `(play path)` ... ([sound](../04-drivers/sound.md#from-each-language)) |
| `"disk"` | `(disks)`, `(df)`, `(cards)`, `(disk-start d)`, `(disk-stop d)` |
| `"proc"` | `(task-args t)`, `(task-cwd t)`, `(task-env t)`, `(task-ns t)`, `(task-regs t)`, `(task-mem t addr n)`, `(task-ram t bank ofs n)` |
| `"clock"` | `(set-date "2026-10-04 12:00:00")`, `(rtc)` |
| `"pc"` | `(pc?)`: whether the PC tool answers |
| `"screen"` | The terminal: `cls`, `at`, `color`, `bold`, `plain`, `clear-line`, `cursor-off`, `cursor-on` |
| `"video"` | The Vera X: `(bitmap 320 8)`, `(pen c)`, `(plot x y)`, `(line ...)`, `(box ...)`, `(bar ...)`, `(circle x y r)`, `(disc ...)`, `(text x y s)`, `(clear)`; the turtle; `(vpeek a)`, `(vpoke a v)`, `(palette! i rgb)`, `(sprite-at n x y)`, `(vsync)`, `(mouse)` ([video](../04-drivers/video.md#drawing)) |

```
hylang> (use "cons")
=> NIL
hylang> (window)
=> 0
```

---

## Every system call

Each call a program makes is a function, `sys-` and its name in lower case: `(sys-getpid)`, `(sys-open "/rom/README" 0)`.
Its arguments are the registers the call takes, and its value what it gives (one, or a list; NIL for none); a failure is
the system's error.  [The call reference](../09-programming/calls-reference.md) gives each call's hylang form (its
arguments by kind: `s` a string, `n` a number, `buf(N)` a buffer the call fills).  `(sys :name args...)` makes any call
by name.

---

## Errors and Ctrl-C

An error is a value: it stops what it reaches, and the REPL shows it (`=> Error: ...`).  `(try x handler)` catches one,
`&err` its message and `&code` its code; a system failure's code is an atom (`:noent`, `:exist` ...: the names of the
error codes, lower case, without `E_`).

```
hylang> (try (/ 1 0) {+ "caught: " &err})
=> "caught: Division by zero."
hylang> (error-code (read-file "/nosuch"))
=> :noent
```

Ctrl-C is the error `interrupted` (`:intr`) at the next call or loop step, which `try` can catch; `(on-note f)` gives it
to a function instead.

---

## hylang as a shell

`hylang -l` is a login shell over rc, by the same rule as HyForth's: **a line that starts with `(`, `{` or `[` (or a
character right against `(` or `{`: `?(`, `=(`, `#(`) is hylang's; any other is an rc command line**, run whole by an rc
of its own (`rc -c`).  So `ls -l | wc` works as in rc, and `(map print (ls "/bin"))` as lisp.

* `cd`, `bind`, `mount`, `unmount` and `newns`, as a line's one command, are done by hylang itself (with rc's quoting), so
  they change the shell's own directory and namespace; `exit` ends it.
* A line ending in `&` isn't waited for: its task is `$apid`.
* The prompt is the directory and `> ` (`/rom/lib> `); a function of yours bound to `shell-prompt` replaces it.
* `hylang -l` runs `/lib/hylang/login.hl` first (its namespace made), then `/lib/hylang/profile.hl` (a card's in the ROM's
  place).

Name `/bin/hylang -l` in a card's `/lib/shell` to have it in every window.

---

## Where hylang differs from danlang

| Area | hylang | danlang |
| :--- | :----- | :------ |
| Integers | Up to 255 bytes (about 614 digits): past that, the error `Too big` | No limit |
| Call depth | 2,500 frames nested | 10,000 |
| Reading | 255 brackets open at once; a name or number of 255 bytes; an expression at the REPL of 4,096 bytes | |
| Strings made | 8,184 bytes at most for one string made by `+`, `format`, `repr` ... | |
| Buffers | 8,184 bytes at most | |
| `load` and `use` | A bare name is `/lib/hylang/name` or `name.hl`, through the namespace | Its own folder |
| Streams | Over the system's fds | The PC's |
| `random` | A 16-bit xorshift, seeded by the tick count | |
| `clock` | To the tick (5 ms) | To the millisecond |
| A file read | Run an item at a time as it's read | Read whole first |
| Errors | No trace of where an error was made | The file, the line and the calls |
| Files | `.hl` | `.dl` |

---

## How it's built

* **Values** are 16-bit words: a fixnum (15 bits), or a reference to a cell in a **heap of 128K** in the task's RAM banks.
  Strings, big numbers and names are blobs in banks of their own.  A **mark-and-sweep collector** frees what isn't
  reachable, sharing its cost across the cells made.
* **The evaluator** is a machine with its own stack of frames (6K in RAM, spilled to banks), never the 65C02's stack, so
  calls in tail position push nothing and Ctrl-C can stop anything.
* **The bytecode machine**: a function `fun` defines is compiled as it's defined (any other at its second call) to the
  code of a small machine, kept in an arena of up to eight RAM banks.  **Native code** then makes the hot paths 65C02
  instructions.  Together they make hylang 1.9 times HyForth's time over twenty benchmarks ([how fast](README.md#how-fast)).
* **The snapshot**: the library of functions written in hylang (`globals.hl`) is loaded at the build and kept in the ROM
  as a snapshot of the heap (the module `hysnap`), so hylang starts in 0.08 s instead of 2.3.
* **Eight banks** of the paged ROM: the evaluator, the reader and printer, the built-ins, the collector, the compiler, the
  bytecode machine; the number system is the shared `numbers` and `math` libraries'.
* The design in full is `docs/design/hylang.md`.
