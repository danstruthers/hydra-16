# hylang

hylang is danlang on the Hydra-16: the lisp the plan makes the preferred command shell
([../../docs/reimplementation-from-scratch.md](../../docs/reimplementation-from-scratch.md), §17; phase 7).  This is
its specification (step 7.0): what hylang 1 is, how it's checked against danlang, where the two may differ, and what
hylang adds to make the Hydra's own things easy to reach.  The runtime's design (16-bit values, the heap over the
task's banks, the collector) is the plan's §17.3, to be confirmed by spike S5 (step 7.1).

## The reference

**hylang 1 is danlang as it is in `C:\source\danlang`** (the C# interpreter, its `lib/` and its `readme.md`, which
describes the language; branch `feature/hylang-prep`), after the work of October 2026 that readied it to be ported:

* **Lexical scope.**  A function's body sees its own variables and the scope it was made in (closures), never its
  caller's.  A Q-expression remembers the scope it was written in, and `eval` runs it there, so code handed to a
  function (`cond`'s clauses, `repeat`'s body, a `try` handler) sees the variables where it was written.
* **Tail calls** through `if`, `do`, `let` and `eval`; other calls nest to a limit, past which there's an error.
* **fexprs** (`fexpr`): functions given their arguments as written, unevaluated, each remembering the caller's
  scope for `eval`.
* **Errors that stop what they reach**: a function called with an error as an argument is that error; `do`, `let`
  and the loops stop at one; `try` (with `&err` and `&code`), `error-message` and `error-code` catch and read them.
* **Binding**: `def` (global), `set` (this scope), `set!` (the nearest binding), `let` with bindings.
* **Loops**: `while`, `each`, `dotimes`, `range`.
* **The basics that were missing**: `type-of`, `gensym`, `read`, `repr`, symbols and atoms from strings, string
  escapes, `print` showing a string's text, `write`, `format` (`~(`), the `str-` functions, character codes and
  tests, `sort`, `reverse`, hash keys of any of atom, string or integer, `hash-remove`, `len` of a hash, streams
  (`open`, `read-line`, `print-to` ...), `stdin`/`stdout`/`stderr`, `output-of`, a script's `args` and `(exit n)`,
  `use` (a library loaded once).
* **Numbers as they were**, their bugs fixed: comparison by value across kinds, consistent kinds for sums
  (complex, else rational, else fixed, else integer), exact quotients (a whole one an integer), division by zero
  an error, complex arithmetic, radix bases (`#16r`), rational literals whose parts must be numbers.
* **The system library** (below): files, programs and the shell, the environment, the clock, bits and bytes, the
  system's errors, and `lib/screen.dl`, the terminal's screen.

**The conformance suite is danlang's regression suite**, `tests/regress/` (965 checks: the reader, evaluation,
scope, control, errors, lists, strings, numbers, hashes, types, I/O, the system, bits and bytes, and the library).
It's written in danlang, so hylang runs it unchanged: the forth test's way, from an emulated card (`hylang run.dl`,
status 0 when every check passes).  A change to the language is made in danlang first, with its checks, then in
hylang.  What only the Hydra has is checked by a file of its own, `hydra.dl`, which `run.dl` runs only where
`(hydra?)` is T (it's written with phase 7's steps 7.4 and 7.5).

## Made for the Hydra

hylang is the Hydra's language as much as BASIC was an 8-bit machine's: what the machine has should be a word away.
The Hydra is Plan 9's kind of system, so most of it is files (a device is a directory of files; it's controlled by
writing commands to its `ctl`), and hylang's reach comes in four layers, from the most portable to the most raw:

1. **The system library**, built in, which danlang has too, so a program that uses only it runs on a PC.
2. **The Hydra's built-ins**: what isn't a file (notes, namespaces, memory and banks, the tasks), built in.
3. **The device libraries**, in hylang, over the devices' files, each loaded by `use` (as forth's `REQUIRE
   tools.fl`): the console, GPIO, I2C, SPI, sound, the disks, `/proc`, the clock's chip.
4. **Every system call**, as a `sys-` function, for what the rest doesn't cover.

A failure anywhere is the system's error: its text (`ERRSTR`'s, after the name it's about: `x: not found`) and its
code as an atom (`error-code`: `:noent`, `:exist`, `:notempty`, `:intr` ... the names of `spec/errors.def`, lower
case, without `E_`).  danlang gives the same for a PC's failures, so the checks are the same on both.

### 1. The system library (built in; danlang too)

| Area | Functions | On the Hydra |
| :--- | :-------- | :----------- |
| **Files** | `read-file`, `read-lines`, `write-file`, `append-file`, `ls`, `dir`, `stat` (a hash: `:name`, `:length`, `:dir`, `:mtime`), `exists?`, `dir?`, `file?`, `mkdir`, `remove`, `rename`, `copy-file`, `cd`, `cwd`, `glob` (rc's `*`, `?`, `[...]`, `[~...]`) | `stat` adds `:mode`, `:qid` and `:dev` (the device letter); `rename` is in its own directory (`WSTAT`'s, as `mv`'s) |
| **Programs** | `(run prog args...)` (its exit code), `(sh line [input])`, `(sh-out line [input])` (its output, a string), `(spawn prog args...)` (a task, not waited for), `(wait task)`, `(kill task)`, `pid` | A bare name is `/bin`'s; the shell is rc (`rc -c`); a task is its number (0-15); `kill` is the `kill` note |
| **Environment** | `(env name)`, `(env)` (a hash of them all), `setenv`, `unsetenv` | `/env`'s, rc's variables: one of several words is a list of strings; `setenv` of a list makes one |
| **The clock** | `time` (seconds since 2000-01-01), `date`, `date-parts`, `seconds-of`, `ticks`, `tick-rate` (200), `sleep` (seconds: `(sleep 1/10)`) | `TIME`, `TICKS`, `SLEEP`; `sleep` ends early, with `:intr`, on a note |
| **Bits and bytes** | `bit-and`, `bit-or`, `bit-xor`, `bit-not`, `shl`, `shr`, `bit?`, `hex`, `bin`, `lo`, `hi`, `word`, `bytes`, `from-bytes`, `read-bytes`, `write-bytes` | A string is bytes, so `bytes` is its bytes as they are |
| **Where** | `(platform)`, `(hydra?)` | `:hydra` |
| **The screen** | `(use "screen")`: `cls`, `at`, `color`, `bold`, `plain`, `clear-line`, `cursor-off`, `cursor-on` | The console is a terminal (ANSI) |

### 2. The Hydra's built-ins

| Area | Functions |
| :--- | :-------- |
| **Notes** | `(note task n)` (Plan 9's postnote: `n` is `:interrupt`, `:kill`, `:hangup`, `:alarm`, or 16-31, a program's own), `(note-group group n)`; `(on-note f)`: `f` is called with each note (as an atom) and returns T to go on, NIL for the default.  Ctrl-C's default in hylang isn't the end: it's the error `interrupted` (`:intr`) at the next call or loop step, which `try` can catch, and the prompt comes back. |
| **Namespaces** | `(bind new old [:before \| :after] [:create])`, `(mount dev old [spec] [:before \| :after] [:create])` (`dev` a string: `"#f"`), `(unmount old [new])`, `(ns)` (the binds and mounts, a list of hashes: `:old`, `:new`, `:flags`), `(newns)` (the default, from `/rom/lib/namespace`) |
| **Tasks** | `(ps)` (a list of hashes: `:task`, `:name`, `:state`, `:parent`, `:cpu`, `:group`, `:args`), `(task-info task)`, `(yield)`, `(sleep-until tick)`, `(hold body...)` (no task switch meanwhile: `PREEMPT_OFF`, for a few ticks' timing; a note still comes) |
| **Memory** | `(peek addr)`, `(poke addr byte)`, `(peek-word addr)`, `(poke-word addr n)`: the task's own 64K (the I/O area's chips belong to their drivers: poke them only knowing that); `(banks)` (the task's RAM banks), `(bank-alloc n)`, `(bank-free bank n)`, `(bank-read bank offset n)` (bytes, a list), `(bank-write bank offset bytes)`; shared segments: `(seg-create banks)`, `(seg-attach seg)`, `(seg-detach seg)`, `(seg-read seg bank offset n)`, `(seg-write seg bank offset bytes)`, `(free)` (the shared RAM, as `free` shows it) |
| **The system** | `(sysinfo)` (a hash: `:abi`, `:ram-modules`, `:free-tasks`), `(mods)` (the paged ROM's modules: hashes of `:name`, `:type`, `:bank`, `:size`), `(errstr code)` |
| **Keys** | `(key)`: the next key, raw (unechoed, unbuffered): a character, or an atom for the terminal's keys (`:up`, `:down`, `:left`, `:right`, `:home`, `:end`, `:ins`, `:del`, `:pgup`, `:pgdn`, `:f1` ... `:f12`); `(key?)`: whether one is waiting (`O_NONBLOCK`).  Raw mode lasts till the next line is read (`read-line`, the prompt), as forth's `KEY?` has it |

### 3. The device libraries (`/lib/hylang/NAME.hl`, `(use "NAME")`)

Each is hylang over the device's files, so it's also a working example of driving the device by hand.

| Library | Device | Functions |
| :------ | :----- | :-------- |
| `cons` | `#c` (`/dev`) | `(window)` (this one's number), `(windows)`, `(new-window)` (made and shown, `wctl`'s `new`), `(show-window n)`, `(raw-on)`, `(raw-off)`, `(beep)` (`#a/bell`, or a BEL) |
| `gpio` | `#g` (`/dev/gpio`) | `(gpio pin)` (its level, 0 or 1), `(gpio! pin level)` (the pin made an output, then set), `(gpio-in pin)`, `(gpio-out pin)`, `(gpio-port)` (all 8, a byte), `(gpio-port! byte)`, `(gpio-ddr! byte)` (1: an output), `(gpio-ca1! :rise \| :fall)`, `(gpio-ca2! 0 \| 1 \| :in)`, `(gpio-wait)` (CA1's next edge: its count), `(gpio-state)` (a hash from `ctl`) |
| `i2c` | `#i` (`/dev/i2c`) | `(i2c-devices)` (the addresses that answer, a list), `(i2c-read addr n [reg])` (bytes; `reg` written first, a repeated start before the read), `(i2c-write addr bytes [reg])`, `(i2c-speed khz)`, `(i2c-reg-size 1 \| 2)` |
| `spi` | `#S` (`/dev/spi`) | `(spi dev bytes)` (a transaction: the bytes that came back), `(spi-read dev n)` (n clocked in), `(spi-mode dev 0 \| 3)` |
| `snd` | `#a` (`/dev`) | `(snd-claim ch...)`, `(snd-release ch...)`, `(snd-patch ch n)` (the X16's 163), `(snd-note ch note)` (MIDI: 60 middle C), `(snd-off ch)`, `(snd-vol ch v)` (0-127), `(snd-pan ch :left \| :right \| :both)`, `(snd-bend ch n)`, `(snd-drum n)` (General MIDI's), `(snd-volume v)` (the master, 0-200), `(snd-reset)`, `(snd-reg reg value...)` (the chip's registers, pairs), `(note-of "C#4")` (a note's number), `(tune {{note beats} ...} [ch] [tempo])` (notes played, a rest NIL), `(play path [times])` (a ZSM song, by `play`) |
| `disk` | `#d` (`/dev/sd`), `#f` | `(disks)` (hashes: `:disk`, `:kind`, `:size`, from each `ctl`), `(disk-start d)`, `(disk-stop d)`, `(df)` (each disk's free and used space), `(cards)` (the SD cards there, by number) |
| `proc` | `#p` (`/proc`) | `(task-args task)`, `(task-cwd task)`, `(task-env task)` (a hash), `(task-ns task)`, `(task-regs task)` (a hash: `:pc`, `:a` ... `:rom`), `(task-mem task addr n)` (bytes), `(task-ram task bank offset n)` |
| `clock` | `#t` (`/dev`) | `(set-date "2026-10-04 12:00:00")` (the clock and the DS1747), `(rtc)` (`:running`, `:stopped` or `:none`, and `:battery-low`) |
| `pc` | `#P` (`/pc`) | `(pc?)` (whether the PC tool answers); `/pc`'s files are files, for everything else |

### 4. Every system call (`sys-`)

Each call a program makes (`spec/api.def`'s, but the servers' and the debugging calls, as forth's `sys-` words) is
a function, `sys-` and its name in lower case (`sys-open`, `sys-sleep-until`), its inputs as arguments in the
specification's order and its outputs as its value (one, or a list of them; NIL for none).  A name the call takes
(zero-terminated) is a string; a buffer it fills is a count (the value is a string of the bytes); a stat record is
a hash; a failure is the system's error.  `tools/apigen.js` makes them from the specification, as it makes forth's,
so a new call is a new function with no more work.

## What hylang 1 has, against danlang

All of danlang's built-ins and library, but where the table below says otherwise:

| Area | hylang 1 | Why |
| :--- | :------- | :-- |
| **Strings** | Bytes: a character is a code 0-255 (`code-char` past 255 is an error); `upper?`, `alpha?` and the rest are ASCII's | The Hydra's text is 8-bit |
| **Numbers** | The whole tower, as danlang has it; integers 15-bit in the value, 32-bit boxed, bignums from a library module (§17.2); fixed decimals, rationals and complex numbers on them; the exotic bases (balanced, negative, little-endian, custom digits) from a library module loaded on first use | Most numbers are small; the rest costs only when used |
| **Call depth** | Less than danlang's 10,000 (the evaluation stack's bank decides it); deeper is the same error | Memory |
| **`random`** | The kernel's entropy and a generator | |
| **`load` and `use`** | A bare name is `/lib/hylang/name.hl`, through the namespace (as forth's `/lib/forth`), so a card's or the RAM disk's `/lib/hylang` adds to the ROM's | Plan 9 names |
| **Streams** | Over the system's fds: `stdin`, `stdout`, `stderr` are fds 0-2; `output-of` points fd 1 at a buffer meanwhile | |
| **Interruption** | Ctrl-C (a note) is the error `interrupted` (`:intr`) at the next call or loop step: `try` can catch it | As forth's THROW -28 |
| **Start-up** | `/lib/hylang/globals.hl` (danlang's `globals.dl`), then `/lib/hylang/profile.hl` if there is one | |

## The prompt

hylang is the console's shell (step 7.7), over rc: **a line that starts with `(`, `{` or a prefix shorthand is
hylang; any other is an rc command line** (the plan's §17.4), so `ls -l | wc` works as in rc, and `(map print (ls
"/bin"))` as lisp.  A value is printed as the REPL's (`repr`); a line that doesn't close goes on with the open
parens shown, as danlang's does, each line edited by the console.  Ctrl-C at the prompt stops what's running and
the prompt comes back.

## Still to decide

These are the user's (the plan's §22 has them too); the drafts are what this specification assumes until then:

1. **The file extension**: `.hl` (drafted).  The suite keeps danlang's `.dl` names, loaded by their whole names.
2. **`$` inside hylang**: drafted as danlang's tail shorthand only (`$(`), with the environment through `env`.
3. **The license**: danlang is GPLv3 (Daniel and Simon Struthers); hylang in the ROM makes the image a combined
   work, so either the Hydra's software takes a license that allows it, or hylang is relicensed.

## The steps (the plan's phase 7)

7.1 spike S5 (values, heap, collector: pairs a second, pause lengths); 7.2 the reader, printer and evaluator
(tail calls, closures, fexprs, errors, notes); 7.3 numbers; 7.4 data and I/O, the system library and the Hydra's
built-ins; 7.5 the shell layer, the device libraries and the `sys-` functions; 7.6 the library; 7.7 hylang as the
login shell.  Each step runs the suite's files it makes pass, and none that passed may fail.
