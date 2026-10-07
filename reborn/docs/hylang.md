# hylang

hylang is danlang on the Hydra-16: the lisp the plan makes the preferred command shell
([../../docs/reimplementation-from-scratch.md](../../docs/reimplementation-from-scratch.md), §17; phase 7).  It's
being written again from scratch, in 65C02 assembly: the first hylang (7.1 to 7.4a, to commit `a0973eb`) was deleted
in October 2026, after a review of danlang and of it, and the new one is built to the plan that review led to, "danlang:
review and 65C02 plan" (a Claude Docs document: <https://claude.ai/code/artifact/32e0e85e-18a6-4e48-9ded-99898b50f43f>).
This is its specification: the language, where it may differ from danlang, what it adds for the Hydra, and its design.

## The reference

**hylang is danlang**: the C# interpreter in `C:\source\danlang` (<https://github.com/SNSTRUTHERS/danlang>, its
`master`, and its `feature/speed` to `744d4db`: buffers, `open`'s `:update`, `clock`, `key`, `round`), whose `reference.md` specifies the language: its syntax, its evaluation, and every built-in's arguments,
value and errors.  danlang's review (October 2026) fixed its bugs and its quirks there first, and settled every rule
that was inconsistent, so the two are one language: text is bytes; `[a b c]` is a list of values; the shorthand is
seven prefixes (`?` if, `=` set, `:` def, `#` hash-create, `@` fn, `.` unpack, `~` format); a hash is called to look a
key up (`(h :k)`, a method `(obj :add 3)`); extra arguments are `&1`, `&2` ... past the formals; `$name` is the
environment's variable; every ordinary built-in gets its arguments' values, the first error stopping it.

**The conformance suite is danlang's regression suite**, `tests/regress/` (1,334 checks), copied to `tests/hylang`
(its README says which phase runs which file).  It's written in danlang, so hylang runs it unchanged, from an
emulated card (`hylang run.dl`, status 0 when every check passes).  A change to the language is made in danlang
first, with its checks, then in hylang.  What only the Hydra has is checked by a file of its own, `hydra.dl`.

## Where hylang differs

| Area | hylang | Why |
| :--- | :----- | :-- |
| **Integers** | Up to 255 bytes (about 614 digits; danlang's have no limit), each step of working one out too (a product, a rational's parts); past that, the error `Too big: an integer past 255 bytes`; 15 bits in the value, an object past that | Most numbers are small; the rest costs only when used |
| **Reading** | 255 brackets open at once; a name or a number of 255 bytes at most; a word or an expression typed at the REPL of 4,096 bytes at most; a value printed 255 lists deep (deeper: `...`) | The reader's and the printer's stacks, in the task's RAM |
| **Strings made** | 8,184 bytes at most for one string made by `+`, `format`, `repr`, `output-of` ... (those it's made in, nested, together) | A capture bank, and a blob's most |
| **Call depth** | 2,500 frames nested, about as many calls not in tail position (danlang 10,000); deeper is danlang's error, `Too deep: more than 2500 calls nested` | The evaluation stack: 6K in the task's RAM, spilled to 4 banks (32K) |
| **`range`, strings, lists** | As memory allows (danlang caps `range` at 1,000,000) | Memory |
| **`load` and `use`** | A path as it is, or with `.hl`; then a bare name is `/lib/hylang/name` or `name.hl`, through the namespace (as forth's `/lib/forth`), so a card's or the RAM disk's `/lib/hylang` adds to the ROM's; a directory is passed over (at `/`, `(use "proc")` is the library, not `/proc`) | Plan 9 names |
| **Streams** | Over the system's fds: `stdin`, `stdout`, `stderr` are fds 0-2; `output-of` points fd 1 at a buffer meanwhile | |
| **Ctrl-C** | The window's `interrupt` note: the error `interrupted` (`:intr`) at the next call or loop step, made once (it stops what it reaches as any error does, so `try`'s handler runs); `on-note`'s function, if there is one, is given it first, as danlang's | As forth's THROW -28 |
| **`random`** | A generator (a 16-bit xorshift) seeded by the tick count as it's first wanted | |
| **Start-up** | `/lib/hylang/globals.hl` (danlang's `globals.dl`) as it is when loaded, from a snapshot in the ROM (the module `hysnap`, made at the build); a ROM without it, loaded as text.  A login shell (`hylang -l`) then runs `#fx/lib/hylang/login.hl`: its namespace made, then `/lib/hylang/profile.hl` (the shell's, below) | Loaded as text, it takes 8 M cycles (2.3 s); from the snapshot, 286,000 (0.08 s) |
| **Files** | `.hl` (the suite keeps danlang's `.dl` names, loaded by their whole names) | |
| **Buffers** | 8,184 bytes at most | A blob's most |
| **`buffer-cmp`** | Library code (`hylib.hl`'s), a byte at a time | The table of built-ins is full |
| **`clock`** | To the tick (5 ms), library code (`hylib.hl`'s) over `clock.start`, the ticks and the time as hylang started; a clock set meanwhile (`set-date`) throws it off | danlang's is to the millisecond; the table of built-ins is full |
| **A file read** | Run an item at a time as it's read, so what comes before a read error has run (danlang reads the whole file first); the error says the line, as danlang's (`file.hl:12: missing )}`: the line of the bracket left open) | The text buffer's 4K, and a file's any length |
| **Where an error was made** | Not shown: an error that ends a program is its message, without danlang's trace on stderr (the file, the line and the calls it was in), and there's no `-w` (its warnings when `def` or `fun` replaces a built-in or a global of another kind) | Code carries no places: each list's file and line would cost heap for every list read |

## Made for the Hydra

hylang is the Hydra's language as much as BASIC was an 8-bit machine's: what the machine has should be a word away.
The Hydra is Plan 9's kind of system, so most of it is files (a device is a directory of files; it's controlled by
writing commands to its `ctl`), and hylang's reach comes in four layers, from the most portable to the most raw:

1. **The system library**, built in, which danlang has too, so a program that uses only it runs on a PC.
2. **The Hydra's built-ins**: what isn't a file (notes, namespaces, memory and banks, the tasks), built in.
3. **The device libraries**, in hylang, over the devices' files, each loaded by `use` (as forth's `REQUIRE
   tools.fl`): the console, GPIO, I2C, SPI, sound, the disks, `/proc`, the clock's chip.
4. **Every system call**, as a `sys-` function, for what the rest doesn't cover.

The first is part of danlang parity (phase 7); the others come after it, as library modules beside the core (the
Hydra built-ins and the `sys-` functions) and `.hl` files (the device libraries).  A failure anywhere is the
system's error: its text (`ERRSTR`'s, after the name it's about: `x: not found`) and its code as an atom
(`error-code`: `:noent`, `:exist`, `:notempty`, `:intr` ... the names of `spec/errors.def`, lower case, without
`E_`).  danlang gives the same for a PC's failures, so the checks are the same on both.

### 1. The system library (built in; danlang too)

| Area | Functions | On the Hydra |
| :--- | :-------- | :----------- |
| **Files** | `read-file`, `read-lines`, `write-file`, `append-file`, `ls`, `dir`, `stat` (a hash: `:name`, `:length`, `:dir`, `:mtime`), `exists?`, `dir?`, `file?`, `mkdir`, `remove`, `rename`, `copy-file`, `cd`, `cwd`, `glob` (rc's `*`, `?`, `[...]`, `[~...]`) | `stat` adds `:mode`, `:qid` and `:dev` (the device letter); `rename` is in its own directory (`WSTAT`'s, as `mv`'s); `cd` alone goes to `/` |
| **Programs** | `(run prog args...)` (its exit code), `(sh line [input])`, `(sh-out line [input])` (its output, a string), `(spawn prog args...)` (a task, not waited for), `(wait task)`, `(kill task)`, `pid` | A bare name is `/bin`'s; the shell is rc (`rc -c`); a task is its number (0-15); `kill` is the `kill` note; an exit status that's a number (rc's `exit 3`: the code 1, the message `3`) is that number |
| **Environment** | `(env name)`, `$name`, `(env)` (a hash of them all), `setenv`, `unsetenv` | `/env`'s, rc's variables: one of several words is a list of strings; `setenv` of a list makes one |
| **The clock** | `time` (seconds since 2000-01-01), `date`, `date-parts`, `seconds-of`, `ticks`, `tick-rate` (200), `sleep` (seconds: `(sleep 1/10)`), `clock` (the seconds since the program started, a fixed decimal) | `TIME`, `TICKS`, `SLEEP`; `sleep` ends early, with `:intr`, on a note; `clock` to the tick |
| **Keys** | `(key)` (the next key, raw: a character, or an atom for the terminal's keys, `:up` ... `:f12`), `(key?)` (whether one's waiting) | The console raw (`consctl`'s `rawon`) till the next line is read |
| **Bits and bytes** | `bit-and`, `bit-or`, `bit-xor`, `bit-not`, `shl`, `shr`, `bit?`, `hex`, `bin`, `lo`, `hi`, `word`, `bytes`, `from-bytes`, `read-bytes`, `write-bytes` (a buffer's, or a part of one) | |
| **Buffers** | Bytes changed in place (shared, as a hash is): `(buffer n [fill])`, `(buffer s)`, `(buffer l)`, `(buffer b)`, `buffer?`, `(b i)` and `buffer-get`, `buffer-put` (its old byte), `buffer-fill`, `buffer-copy` (overlapping too), `buffer-cmp` (a part of one against a string's or a buffer's bytes, as `cmp` orders strings), `read-buffer` (how many; NIL at the stream's end); `len`, `bytes`, `from-bytes`, `eq`, `cmp` and `save` take one; printed `<buffer>{1 2}` | `modules/hylang/buffers.inc`: a string's cell, its blob changed in place |
| **Where** | `(platform)`, `(hydra?)` | `:hydra`, T |
| **The screen** | `(use "screen")`: `cls`, `at`, `color`, `bold`, `plain`, `clear-line`, `cursor-off`, `cursor-on` | The console is a terminal (ANSI) |

### 2. The Hydra's built-ins

| Area | Functions |
| :--- | :-------- |
| **Notes** | `(note task n)` (Plan 9's postnote: `n` is `:interrupt`, `:kill`, `:hangup`, `:alarm`, `:brk`, or its number: 16-31 a program's own), `(note-group group n)`; `(on-note f)`: `f` is called with each note (but a kill) at the next call, the note an atom (`:interrupt`, `:hangup`, `:alarm`, `:brk`) or its number, and returns T to go on, NIL for the default (Ctrl-C's: the error `:intr`; another's, hylang's end, as the system's default: 129 for a hangup ...); `(on-note NIL)`: the defaults again; anything but a function or NIL, an error |
| **Namespaces** | `(bind new old [:before \| :after] [:create])`, `(mount dev old [spec] [:before \| :after] [:create])` (`dev` a string: `"#f"`; `spec` which of its trees: `"x"`, the ROM disk), `(unmount old [new])`, `(ns)` (the binds and mounts, a list of hashes: `:old`, `:new` (the device's path: `"#fx/lib"`), `:create`), `(newns)` (the default namespace built again in hylang's own, as rc's `newns` builds it: `/lib/hylang/newns.hl`, loaded: its area of the RAM disk emptied and made, then `/rom/lib/namespace`'s lines and a card's) |
| **Tasks** | `(ps)` (a list of hashes: `:task`, `:name`, `:state` (`:ready`, `:wait`, `:call`, `:idle`, `:new`, `:sleep`, `:blocked`, `:event`), `:parent`, `:cpu` (ticks), `:group`, `:args`), `(task-info task)` (one of them), `(yield)`, `(sleep-until tick)`, `(hold body...)` (no task switch meanwhile: `PREEMPT_OFF`, for a few ticks' timing; a note still comes; preemption back after an error too) |
| **Memory** | `(peek addr)`, `(poke addr byte)`, `(peek-word addr)`, `(poke-word addr n)`: the task's own 64K, as it is as hylang runs (its fifth bank at `$A000`, a heap bank at `$8000`; the I/O area's chips belong to their drivers: poke them only knowing that); `(banks)` (the task's RAM banks), `(bank-alloc n)`, `(bank-free bank n)`, `(bank-read bank offset n)` (bytes, a list), `(bank-write bank offset bytes)` (what fits, then the error); shared segments: `(seg-create banks)`, `(seg-attach seg)`, `(seg-detach seg)`, `(seg-read seg bank offset n)`, `(seg-write seg bank offset bytes)`; `(free)` (the RAM, as `free` shows it, in K: `:ram` a task's, `:shared`, `:used`, `:free`, `:segments`) |
| **The system** | `(sysinfo)` (a hash: `:abi`, `:ram-modules`, `:free-tasks`), `(mods)` (the paged ROM's modules: hashes of `:name`, `:type` (`:program`, `:driver`, `:library`), `:bank`, `:banks`), `(errstr code)` (a code an atom, `:noent`, or its number) |
| **Keys** | `(key)`: the next key, raw (unechoed, as it comes): a character, or an atom for the terminal's keys (`:up`, `:down`, `:left`, `:right`, `:home`, `:end`, `:ins`, `:del`, `:pgup`, `:pgdn`, `:f1` ... `:f12`); `(key?)`: whether one is waiting (a read of the console that doesn't wait).  Raw mode lasts till the next line is read (the prompt's), as forth's `KEY?` has it |

### 3. The device libraries (`/lib/hylang/NAME.hl`, `(use "NAME")`)

Each is hylang over the device's files, so it's also a working example of driving the device by hand.  What they
share is `dev.hl` (`(use "dev")`): a file opened for one request (a READ or a WRITE, by the `sys-` functions, so
an I2C or SPI transaction is one) and closed after, a failure's file closed too.  Bytes are a list of integers
(a string's will do where they're written); a failure is the system's error (`:io`, `:busy` ...).

| Library | Device | Functions |
| :------ | :----- | :-------- |
| `cons` | `#c` (`/dev`) | `(window)` (this one's number, `$window`), `(windows)` (their numbers), `(shown-window)`, `(new-window)` (made and shown, `wctl`'s `new`), `(show-window n)`, `(raw-on)` (`consctl` kept open), `(raw-off)`, `(beep)` (`#a/bell`, or a BEL) |
| `gpio` | `#g` (`/dev/gpio`) | `(gpio pin)` (its level, 0 or 1), `(gpio! pin level)` (the pin made an output, then set), `(gpio-in pin)`, `(gpio-out pin)`, `(gpio-port)` (all 8, a byte), `(gpio-port! byte)`, `(gpio-ddr! byte)` (1: an output), `(gpio-ca1! :rise \| :fall)`, `(gpio-ca2! 0 \| 1 \| :in)`, `(gpio-wait)` (CA1's next edge: its count), `(gpio-state)` (a hash from `ctl`: each pin its direction and level, `{:out 1}`; `:ca1` `{:rise 3}`; `:ca2` 0, 1 or `:in`) |
| `i2c` | `#i` (`/dev/i2c`) | `(i2c-devices)` (the addresses that answer, a list), `(i2c-read addr n [reg])` (bytes, a list; `reg` written first, a repeated start before the read), `(i2c-write addr bytes [reg])`, `(i2c-speed khz)`, `(i2c-reg-size 1 \| 2)` |
| `spi` | `#S` (`/dev/spi`) | `(spi dev bytes)` (a transaction: the bytes that came back), `(spi-read dev n)` (n clocked in), `(spi-mode dev 0 \| 3)` |
| `snd` | `#a` (`/dev`) | Channels 0-7 the YM2151's, 8-23 the Vera X's PSG's.  `(snd-claim ch...)`, `(snd-release ch...)`, `(snd-wave ch :pulse \| :saw \| :triangle \| :noise [width])` (a PSG channel's waveform; width 0-63, 63 a square), `(snd-patch ch n)` (the X16's 163), `(snd-note ch note)` (MIDI: 60 middle C), `(snd-off ch)`, `(snd-level ch v)` (a channel's volume, 0-127; `snd-vol` its old name), `(snd-pan ch :left \| :right \| :both)`, `(snd-bend ch n)`, `(snd-drum ch n)` (General MIDI's), `(snd-freq ch hz)`, `(snd-glide ch n)` (no new attack), `(snd-lfo rate pmd amd wave)`, `(snd-sens ch pms ams)`, `(snd-noise n)` (channel 7's, 0-31; negative: off), `(snd-volume v)` (the master, 0-200), `(snd-reset)`, `(snd-reg reg value...)` (the chip's registers, pairs), `(snd-regs)` (the 256 as written, a buffer), `(note-of "C#4")` (a note's number), `(tune {{note beats} ...} [ch] [tempo])` (notes played in step with the tick, a rest NIL; 120 a minute), `(play path [times])` (a ZSM song, by `play`), `(snd-mml ch mml)` and `(snd-chord ch notes)` (a line of MML and a chord, by `play -m` and `-c`) |
| `disk` | `#d` (`/dev/sd`), `#f` | `(disks)` (the disks started, hashes: `:disk`, `:kind`, `:blocks`, `:label`, from each `ctl`), `(disk-start d)`, `(disk-stop d)`, `(df)` (each file system's room, hashes: `:disk`, `:label`, `:free`, `:size`, in KB), `(cards)` (the SD cards there, their SPI devices) |
| `proc` | `#p` (`/proc`) | `(task-args task)` (a list of strings), `(task-cwd task)`, `(task-env task)` (a hash, its names strings), `(task-ns task)` (a line each), `(task-regs task)` (a hash: `:pc`, `:a` ... `:rom`), `(task-mem task addr n)` (bytes, a list), `(task-ram task bank offset n)` |
| `clock` | `#t` (`/dev`) | `(set-date "2026-10-04 12:00:00")` (the clock and the DS1747), `(rtc)` (a list: `:running`, `:stopped` or `:none`, and `:battery-low`) |
| `pc` | `#P` (`/pc`) | `(pc?)` (whether the PC tool answers); `/pc`'s files are files, for everything else |

### 4. Every system call (`sys-`)

Each call a program makes (`spec/api.def`'s, but the servers', the debugging calls and `NOTIFY`, as forth's `sys-`
words) is a function, `sys-` and its name in lower case (`sys-open`, `sys-sleep-until`): `(sys-open "x" 0)` is
`(sys :open "x" 0)`, `sys` being the built-in that makes any of them by name.  A `sys-` name is bound as it's first
looked up, to `sys` partially applied to the name's atom (`<function>(sys :open)`), so every call's function
costs one built-in.  The arguments are the registers the call takes, in the order of its `hl:` line in `spec/api.def`, and
its value is what it gives (one, or a list of them; NIL for none):

* a register: a whole number (a character, its code), at most the register's size (`SEEK`'s offset may be
  negative); some may be left out, as 0;
* a name the call takes (zero-terminated): a string (an atom's or a symbol's name will do); bytes it reads
  (`WRITE`'s, `ENV_PUT`'s): a string, its length the count; `SPAWN`'s arguments: a list of strings (the
  program's name isn't one of them);
* a buffer it fills: a string of its bytes (`READ`'s: an argument, the count, and the string as long as what was
  read; a name's, its text);
* a stat record: a hash, as `stat` gives (`WSTAT`'s: `:name`, `:mode`, `:length`, `:mtime`, what isn't in it left as
  it is).

A failure is the system's error, about the call's first string (`(sys-open "x")`: `x: not found`, `:noent`).
hylang's output is written before the call (so `sys-puts` comes after it).  A call's strings and buffer share
1,536 bytes (`TASKREAD`'s, `ENV_SIZE`, fits).  `tools/apigen.js` makes the calls' records from the `hl:` lines
(`obj/gen/hylsys.inc`), checking each register against the call's `in:` and `out:` lines, and the reference
(`obj/gen/api.md`) has each call's; a call a program makes needs its `hl:` line (apigen fails without one), and is a
function with no more work.

## The prompt

hylang is the console's shell, over rc, as a login shell (`hylang -l`, as `/lib/shell` names it: init's in window
0, wstart's in the windows made) or once `(use "shell")` turns its rule on: **a line that starts with `(`, `{`,
`[`, or a character right against `(` or `{` (`?(`, `=(`, `#(` ...) is hylang; any other is an rc command line**
(the plan's §17.4), run whole by rc (`rc -c`) and waited for, so `ls -l | wc` works as in rc, and `(map print (ls
"/bin"))` as lisp.  The rules are HyForth's shell's ([hyforth.md](hyforth.md), "The shell"): each rc line is an rc of
its own, so what one sets (a variable, a function) goes with it; its code is `status`, and `$status` (rc's) goes in
the environment for the next line's rc.  rc's own forms stay rc's: `$x`, `# comment`, `. file`, `~ subject
pattern`; a block that starts a line (`{...}`) is hylang's, so it comes after something (`rc -c '{...}'`), and rc's
`@{...}` is written `@ {...}`.  What an rc line can't do to hylang (it runs in a task of its own) the shell does
when it's the line's one command, its arguments parsed as rc's built-ins do (`'...'` quotes, `$name`): `cd` (the
prompt follows it), `bind`, `mount`, `unmount`, `newns`; and `exit` ends hylang (its code `status`).  A line
ending in `&` isn't waited for: its task is `$apid`, in a note group of its own.  A value is printed as the REPL's
(`=> ` and its `repr`); a line that doesn't close goes on with the open brackets shown, as danlang's does, each line
edited by the console.  Ctrl-C while rc runs is rc's, and the shell goes on, on a new line; at hylang's own line it
stops what's running.  The prompt is `shell-prompt`'s: the directory and `> ` (`/rom/lib> `; on a card, `0:/games> `),
and a function (or a string) of the user's in its place is the prompt.

`hylang -l` runs `#fx/lib/hylang/login.hl` before its first prompt (a window's shell starts in an empty namespace,
so it's read by its device's name): its namespace made, as `newns` makes it, then `/lib/hylang/profile.hl`, through
the `/lib` union (a card's or the RAM disk's in the ROM's place), as rc's `/lib/profile`: the shell (`shell.hl`), the
window at `/dev`, its notes to hylang's note group.  The REPL finds the shell by name, as HyForth's core does: an
expression's first line that isn't hylang's goes to `shell-line` (a string), and the prompt is `shell-prompt`'s, each
when it's bound.

## Against HyForth

Twenty benchmarks are written in each language, on the ROM disk at `/rom/bench`: `bench.hl` (which loads each
benchmark's own file, `hl/NAME.hl`) and `bench.fs`.  They have the same algorithms, sizes and results, each written
the language's own way (a tail call, `while`, `dotimes` or `each` in hylang, `DO LOOP` or `BEGIN WHILE REPEAT` in
HyForth; lists, `map`, `filter` and `foldl` in hylang where HyForth loops over an array and `EXECUTE`s a word), every
value under 16,384 (hylang's fixnums, a cell that doesn't overflow), as deep as HyForth's data stack (32 cells)
takes.  Each prints a line a benchmark, `bench LANGUAGE NAME RESULT TICKS REPS` (`hylang /rom/bench/bench.hl [reps
[q|f [name...]]]`, `forth /rom/bench/bench.fs [reps [q|f [name...]]]`, on the board too).  `node sim/bench.js` runs
both in the emulator and prints them by kind (calls, loops, arithmetic, bytes, lists, text), with each kind's
geometric mean: `--quick` (the small sizes), `--only` and `--kind` (some of them), `--hylang-reps` and
`--forth-reps` (1 and 5), `--together`, `--vs TREE` (another tree's build beside this one's, the same benchmarks
on its disk: another branch's worktree), `--json FILE`.  Each of hylang's runs in a hylang of its own;
`--together` runs them in one.  The `bench` test
runs both at the quick sizes and checks each result is the same.

In October 2026 (`reborn-hynat`, the native code), at 3.58 MHz, one run of each, beside the bytecode machine's
(`reborn` at 76546a8, `--vs`):

| Kind | Benchmark | What | Result | hylang | Bytecode | HyForth | hylang / HyForth |
| :--- | :-------- | :--- | -----: | -----: | -------: | ------: | ---------------: |
| calls | `calls` | 2,000 calls of a function of two arguments, in a tail call's loop | 2000 | 540 ms | 995 ms | 65 ms | 8.3x |
| calls | `fib` | Fibonacci of 16, recursively (3,193 calls) | 987 | 515 ms | 980 ms | 181 ms | 2.8x |
| calls | `tak` | Takeuchi's function, tak(9, 6, 3) six times (1,758 calls of three arguments) | 36 | 310 ms | 560 ms | 199 ms | 1.6x |
| calls | `ack` | Ackermann's, ack(2, 9) eight times (1,840 calls, 22 deep) | 168 | 255 ms | 520 ms | 115 ms | 2.2x |
| loops | `loop` | A counting loop of 4,000 steps, a tail call each | 4000 | 400 ms | 925 ms | 77 ms | 5.2x |
| loops | `while` | A sum of i & 3 for i below 4,000, by `while` over two locals | 6000 | 725 ms | 3,685 ms | 421 ms | 1.7x |
| loops | `dotimes` | The same sum by `dotimes` (HyForth: `DO LOOP`) | 6000 | 750 ms | 3,655 ms | 213 ms | 3.5x |
| loops | `nested` | A `dotimes` in a `dotimes`, 60 by 60, `bit-xor` and a test | 1800 | 825 ms | 5,435 ms | 309 ms | 2.7x |
| arith | `gcd` | gcd(i, j) by subtraction, for i and j 1 to 20, summed | 880 | 475 ms | 950 ms | 351 ms | 1.4x |
| arith | `collatz` | The Collatz steps of 1 to 60, summed (1,457) | 1457 | 420 ms | 2,220 ms | 289 ms | 1.5x |
| arith | `hash` | h = ((h & 255) * 31 + i) & 4095 for i below 2,000 | 4072 | 715 ms | 4,820 ms | 669 ms | 1.1x |
| bytes | `sieve` | The primes below 1,024, a byte each | 172 | 1,105 ms | 1,905 ms | 332 ms | 3.3x |
| bytes | `sort` | 100 bytes sorted by insertion | 407 | 1,815 ms | 3,270 ms | 480 ms | 3.8x |
| bytes | `matrix` | Two 10 by 10 matrices of bytes multiplied, summed | 1375 | 965 ms | 4,030 ms | 1,082 ms | 0.9x |
| bytes | `queens` | The 7 queens' 40 placements, by backtracking | 40 | 2,505 ms | 4,390 ms | 1,301 ms | 1.9x |
| lists | `mapf` | `sum`, `map`, `filter` over `range` 0 to 39, 20 times (HyForth: a loop, `EXECUTE`) | 9880 | 1,025 ms | 2,345 ms | 207 ms | 5.0x |
| lists | `fold` | `foldl` of a function made with `fn` over 200 items, 10 times | 964 | 1,265 ms | 4,535 ms | 712 ms | 1.8x |
| lists | `each` | `each` over 200 items, 10 times (HyForth: an array) | 700 | 450 ms | 1,855 ms | 209 ms | 2.2x |
| text | `chars` | The a's in 64 characters, 40 times: `char-at`, `char-code` | 7 | 775 ms | 2,270 ms | 203 ms | 3.8x |
| text | `digits` | The numbers below 1,000 written out, their lengths summed | 2890 | 1,820 ms | 1,950 ms | 2,105 ms | 0.9x |
| All | | | | 17,655 ms | 51,295 ms | 9,520 ms | 1.9x (the ratios' geometric mean 2.3x) |

hylang is nearest HyForth where a step does much (`matrix` and `digits` 0.9 times, `hash` 1.1, `gcd` 1.4,
`collatz` 1.5, `tak` 1.6, `while` 1.7).  A call (`calls` 8.3 times) and a tail loop's step (`loop` 5.2) cost most
next to HyForth's, whose calls are a JSR and whose loop counter is a register's; then the walks of `map` and
`filter` (`mapf` 5.0).  The bits' built-ins (`bit-and`, `bit-or`, `bit-xor`, `shl`, `shr`) and the
characters' (`char-at`, `char-code`) were each a built-in's call, some 2,000 cycles, where HyForth's `AND` and `C@`
take a few; native code has them in the machine's quick way for fixnums (a string's byte), as `*` of two, and the
loops that use them came from 7 to 16 times HyForth's time to 1.7 to 3.5 (`while`, `dotimes`, `nested`), `hash`
from 4.6 to 1.1, `chars` from 9.6 to 3.8.

The arena: the native code of all twenty benchmarks' 41 functions is 37,405 bytes (their bytecode 3,698): `RET` in
ROM and stubs of 5 bytes (not 93 and 11) made it a third smaller, and a code's place even let the arena have eight
banks (64K, not four), so all of them fit in one hylang: `--together` 18,010 ms, as each in its own (17,655), where
with four banks the arena filled by `sort`'s and the rest were evaluated (then 110,185 ms against 33,945).  A
script that fills it gets it emptied between its items: with an arena of one bank, a script of all twenty's files
took 62 million cycles, against 58 with eight banks and 204 when it stayed full.

## The design

The plan has it whole; in short:

* **Values** are 16-bit words: a fixnum (15 bits, bit 0 set), or a reference (bit 0 clear) counting 2-byte units:
  bits 15-12 one of 16 banks, bits 11-0 times 2 the place in the `$8000` window, so the cell heap is 128K (16,384
  conses would have been too few: `globals.dl`, `dice.dl` and `harn.dl` take 7,300 cells once read).  Below `$0600`
  a reference is an immediate (NIL `$0000`, T, (), exit, the characters, the built-ins).  Lists, strings and numbers
  are immutable and shared, never copied; hashes, streams and scopes change.
* **Memory** (`modules/hylang/heap.inc`, in the task's RAM): each 512-byte page of the cell heap holds one kind of
  cell (a 256-byte table gives a value's type); a list is its first cons, its page saying code or data; strings,
  bignums and symbols' names are blobs in banks of their own, each owned by one cell; symbols are interned and
  hold their global value; a scope is a frame, its symbol-value pairs side by side.  Mark and sweep: a
  mark stack of 255 (a list's spine followed in place, so it costs none), every marked cell scanned again if it
  fills; a free list per kind; the blobs nothing owns dropped and the rest slid down.  Banks are taken as they're
  needed (up to 16 for cells, 16 for blobs, 4 for the stack; a cell bank more after a collection while fewer pages
  are free than used, so a collection's cost, which is what's live, is shared by as many cells made), and a
  collection while the prompt waits too.  The collector's code is in the sixth bank, entered by a far call.
* **The snapshot** (`tools/hysnap.js`, `hylang.s`'s `snap_restore`): what hylang keeps between collections (the
  heap's tables, the symbols, the evaluator's own: `hylang.cfg`'s PSTATE segment) and the pages and blobs in use,
  as they are once `globals.hl` is loaded, in the module `hysnap` (a library of data, two banks).  The build makes
  it: hylang run in the emulator from a ROM without it, stopped once its library is loaded (`lib_done`), read from
  its task's RAM; with hylang's id, a CRC-16 of its image patched into it (`snap_id`).  As hylang starts, it looks
  for it in the bank after its own (where `rom.txt` puts it) or else in the module directory, and if it's this
  hylang's, takes banks anew and copies it in (16 loads and stores a step, about 10 cycles a byte: 17K of it,
  and `DATA4`'s 3K, are most of the 286,000 cycles to the first prompt); if not, it loads `globals.hl` as text.
* **The reader and the printer** (`read.inc`, `print.inc`, in the second bank) are danlang's, in one pass without
  tokens: a level for each bracket open (its closer, its list so far and its last cons, the levels' lists marked
  as roots), each item made as it's read and put at its level's end; the first error ends it, as the first token in
  error is danlang's, with danlang's message.  At the REPL, the text so far is read again with each line while a
  bracket or a here string is open.  The printer is a loop too, over a stack of the lists it's in.  `hylang -g`
  collects before every allocation: a test of what's kept as a root.
* **The evaluator** (`eval.inc`, `forms.inc`) is a machine: the expression or its value, its scope, and a stack
  of frames (6K in the task's RAM, spilled to banks in 2K blocks), never the 65C02's stack.  A frame is its words
  and its code on top, all values or fixnums, so the collector marks the stack whole.  A call in tail position (a
  body, `if`'s branches, `do`'s, `let`'s and `eval`'s last) pushes nothing; an argument that isn't a call is
  evaluated where it's met, with no frame; a call's values go on the stack, and a built-in reads them there.
  `map`, `filter`, the folds, `any?`, `all?`, `find`, `count`, `sum`, `product` (one walk over a list, its frame
  kept on the stack as it goes), `sort` (a merge sort, its runs relinked, its state its frame; `less` called
  through the machine) and the loops are frames too, so what they call nests like any call and Ctrl-C stops it.
  An error passes every frame that doesn't take one (`try`'s, a built-in's that takes errors).  A scope is a
  frame cell of name-value pairs; a symbol never bound in one is looked up at its global value at once.  A
  Q-expression evaluated, and an fexpr's argument, remember their scope (a scoped cell).  `load` reads a file an
  item at a time, the reader's text refilled from it, and seeks it back if a nested `load` used the text
  meanwhile.
* **The bytecode machine** (`vm.inc`, in the seventh bank; its compiler in the sixth): a function `fun` defines is
  compiled as it's defined, any other at its second call, to the code of a small machine whose value register is
  `ex`; the code is in an arena of RAM banks of its own (eight at most, 64K), never moved, and the function's word 4
  is its place (word 5 counts its calls till then; `fun` counts one, so a function it couldn't compile, the arena
  full, is compiled at its next call).  A frame is the function's word and its arguments, where the caller pushed them on
  the evaluation stack, then a record of two words (its return and its scope), so a call makes nothing on the heap
  and an argument is a word at a fixed place; a call in tail position (`TCALL`) reuses its caller's frame.
  Constants, arguments, globals, `if`, `do`, `and`, `or` and `while` are compiled in place, and `set` (`=(...)`),
  `set!` and `def` (each value compiled, the binding an op); the names a body binds with `set` are its locals, words
  after the record, each a hole (UNBOUND) till it's set, read past the frame meanwhile (the evaluator's lookups pass
  a hole by too); `+`, `-`, `1+`, `1-`, `zero?`,
  `one?` and the comparisons are ops that work fixnums at once (with a constant, one op); a built-in is called at
  once; any other call is `HEAD` (its function a function?) and `CALL` (a global's function, `SHEAD`: the global read
  in the same op; the function's own, by its name, `CSELF` and `TSELF`, which make its frame at once; a buffer
  given an index, or a built-in partially applied, called at once too: a buffer's byte, and `buffer-put`, done in
  the machine for a fixnum index in it).  `SHEAD` keeps the global's value it read, and `CALL` the function it
  called and its code, with the globals' epoch (`vm_ep`), which moves on as a global is bound again, a name is
  first bound in a frame, or a collection runs: while it's the same, they use them at once.  The compiler joins
  ops as it emits them: an argument and the quick op after it (`LQ`; `LQP`, its value pushed), two arguments and a
  quick op (`LL`), and a test and the `JF` after it (`JLQ`, `JLL`, `JQ`), each with code of its own for the usual
  quick ops; a local set by a statement of the body (its `do`'s parts, in order) is read and `set!` with no
  hole's check after.  Six rare ops come after `EXT`.  The evaluator does the rest: the other
  special forms, an fexpr's call, a function not compiled (or with extras), a built-in that runs the machine; its
  value comes back through a `K_VM` frame.  A frame's scope is made only when it's wanted (a Q-expression with
  names in it, the evaluator, the built-ins that keep their caller's scope: `list`, `fn`, `fun`, `fexpr` and the hash
  makers), its formals and locals bound there in order (holes too), and they're read and set there after, so a
  closure or `eval` sees what the code sees.  `let`, a step of `each` or `dotimes`, and `try`'s handler are blocks:
  a block's variables are words in the frame too (after the locals, each a hole till it's bound), with a word for
  its scope, NIL till one's wanted; `BLOCK` begins it (again at each step of a loop, so a closure keeps its step's
  variables).  A scope wanted, the function's table of its blocks (each one's code, and the block it's in) gives
  those the code is in, each made then, outermost first, its variables bound there and read and set there after.
  Errors are values, as the evaluator's: an op that may give one returns it from the function, unless what it's
  for takes errors (`error?`'s argument, say); a quick op given one has it as its value.  The compiler counts on
  a name's built-in value (a special form, an operator) only while no frame has bound the name, and marks it
  (`SF_INLINED`); bound in a frame then, or bound again globally, every function's code is dropped and compiled again
  as it's next called.  Ctrl-C and notes are taken at each call, as the evaluator takes them.  The arena full,
  nothing is compiled till the evaluator's next start (a line at the prompt) or a script's next item (`load`'s,
  when nothing is under the `load`, so no frame of the machine's is left), which empties it.
* **Native code** (`vmx.inc`, in the eighth bank): the machine's code is the 65C02's own.  The compiler writes a
  function's bytecode in a scratch bank of its own (`vm_sb`, 4K at most: a bigger function is evaluated), and
  `vm_xlate` makes it native code in the arena, in two passes (each op's place, in a map bank, `vm_mb`; then the
  code).  An op is a stub (`jsr vm_sj` and its code's word in the machine: `vm_sj` points `vm_ip` at the data
  after them, the op as it was, and jumps to the code, whose next op is `jmp (vm_ip)`), or in line: its own code
  from a template (`vmxt.inc`, made by `tools/hyvmxt.js`), its operands patched in, with its stub after it as its
  slow way (not fixnums, a scope made, a cache missed, Ctrl-C).  In line: constants, arguments and locals, pushes,
  jumps, the fused ops, the quick ops of two values, blocks' and loops' ops, `SHEAD` and `CALL` while their caches
  hold, `CSELF`, `TSELF`, `JE`, and `HEAD` of one argument (a buffer, or a function partially applied, pushed), and
  `BCALL` of `*` of two (fixnums whose product is one: `vm_bmul`, by quarter squares), of `bit-and`, `bit-or`,
  `bit-xor`, `shl` and `shr` of two fixnums (`vm_band` ...: a fixnum's tag bit is its bits' for `and` and `or`; a
  shift a bit a step, `shl`'s while its sign holds), `char-at` of a string and an index in it, and `char-code` of a
  character (`vx_tsel`'s table, `vx_bcb`: each a template that calls the machine's routine, its stub if it says
  no).  A tail call of the function by its own name whose arguments call nothing has its `SHEAD` flagged by the
  compiler (m's bit 7, `vc_shflag`): its head isn't pushed while its cache (this frame's function alone) holds,
  and `TSELF` takes the arguments as they are (`vm_shf` says if the machine's way pushed it).  `map`, `filter`, `foldl` and the
  other walks but `foldr`, called from native code, walk their list in the machine (`vm_walk`): the function
  called for each item as `CALL` would, returning to a trampoline at the arena's first bytes (`jmp vm_wret`).  A `CALL` whose
  cache missed goes on past its look at it (`op_callm`), where a buffer given a fixnum
  index has its byte at once; `buffer-put` of three is the first thing `BCALL`'s code looks for.  `CALL` and
  `CSELF` have a return pad past their data, where their returns go: the caller's frame found again from the
  call's h and r (`vm_s`, `vm_rb`, `vm_mat`), so `RET` in line only finds the caller's code, drops the frame and
  looks for an error (the caller's r, the byte before the pad, says if it's returned too: `vm_reterr`).  The pad
  gives the frame the machine found already, if it did, so the machine's `RET` and the evaluator's resume
  (`HEAD`'s and `SHEAD`'s t are the pad, `VXK_Q`) go through it as well.  `RET` is `jmp vm_nret`, the machine's
  code for it in ROM (a function has two or three).  A code's place is even (a `NOP` before a stub puts a pad or a
  record's scope word there), so its fixnum is its bank's index (3 bits) and its address's bits 12-1, and the
  arena has eight banks.  The places in the data that are code
  (jumps' targets, blocks' parents and table, a function's start, calls' returns) are the native code's, so
  every op's code in the machine runs as it did.  Native code runs in the RAM window ($8000-$9FFF) a heap cell is read through, so whatever reads
  one is the machine's, in ROM, and sets the code's bank again before going on.
* **Built-ins**: a table of all danlang's (its arity, flags, the bank its code is in), so partial application, too
  many arguments and taking errors are the dispatcher's (till phase 7 made the last, those not made yet answered
  `Not yet: 'name'`).
  Strings are made by capturing output (a bank of its own), as danlang's `StringBuilder`.
* **Numbers** (`numreg.inc`, `numval.inc`, `numtext.inc`, `numbi.inc`, `numbits.inc`, in the third bank): an integer
  is a fixnum, or a bignum (its sign and length in its cell, its bytes in a blob, least first); a fixed decimal its
  digits and places, a rational its numerator and denominator (in lowest terms), a complex number its real parts.
  Integers are worked in registers, pages of the reader's scratch, so a sum or a product of bignums makes nothing
  on the heap till its value is made; a real number is worked there as a fraction, n/d, then made the kind
  danlang's rules give; a complex number by its parts, on the root stack.  Each entry to the number code sets an
  abort point, which a result too big goes back to from however deep.  The reader gives each word that starts
  like a number to danlang's grammar, whole (every base: digits of its own, balanced, negative, least digit
  first); `+`, `-` and `*` keep a fixnum's quick way.
* **Strings and hashes** (`strs.inc`, `hashes.inc`, in the fourth bank): a string built-in reads its arguments'
  bytes where they are, two at once (a character is a string of one), and makes its value by capturing output.  A
  hash is a cell of its items: its entries, each a list `{key value tag...}` never changed in place (a change puts
  a new one in its place), in the order they were put, then its own tags; so `from#` and the printer have it as
  it is, and a key is found by a walk along them (a hash is small: danlang's objects).  A key is an atom, a string
  or an integer (`2.0` is `2`).  `hash-create`, `to#` and `hash-put` evaluate each entry's value through the
  machine, in a frame of their own.  A hash called, `(h key arg...)`, evaluates its key, then calls the function
  there with the arguments as any function is called, `&0` the hash (a proxy, through which its private entries
  are had) in a scope of its own between the function and its scope.
* **Streams and the system library** (`sys.inc`, `streams.inc`, `system.inc`, in the fifth bank): a stream is a
  cell of its fd and its flags; `stdin` is read through the REPL's own buffer, so a program's reads and the REPL's
  share it, and `stdout` is the output itself, so `output-of` has what's printed to it.  A file's line is read a
  chunk at a time, what's past it given back (`SEEK`).  Each built-in is the system's calls; its failure is the
  system's error, `name: text` (`ERRSTR`'s) and its code an atom (the names `tools/apigen.js` makes from
  `spec/errors.def`: `obj/gen/errnames.inc`).  `sh` and `sh-out` run `rc -c`, their input and output through
  pipes; `date`, `date-parts` and `seconds-of` work the calendar on 32-bit seconds.  `hylang file args...` runs
  the file (`args`: its path and the args), its status 0, 1 after an error (on stderr), or `(exit n)`'s.
* **The module**: hylang is one program of eight banks (a module's most): the evaluator, its
  special forms, the dispatch and the built-ins that run the machine in the first; the reader, the printer, the
  list built-ins, equality and order in the second; the numbers (and, as yet, `fn`, the type tests and `error`)
  in the third; strings, hashes and the errors' messages in the fourth; streams, the system library and the
  Hydra's built-ins (`hydrabi.inc`) in the fifth; the collector and the bytecode machine's compiler in the sixth;
  the machine in the seventh; its native code's translator and templates in the eighth.  What every bank calls
  is in the task's RAM (the heap, the
  output, the evaluation stack): the most of that code is kept in the fourth bank and copied to the RAM as hylang
  starts (`hylang.cfg`'s DATA4), so the first bank's room is the evaluator's.  `+`, `-`, `1+`, `1-`, `zero?`,
  `one?` and the comparisons work fixnums in the first bank (`bi_fast`), without a far call.  The Hydra layers
  are library modules beside it.
* **Budgets** (at 3.58 MHz, the library loaded; each from the REPL's echo to its `=>`, a difference of two lines'
  times so the REPL's own work drops out): start-up from the snapshot to the first prompt 300,000 cycles (286,000);
  a parameter looked up 300 (279; compiled, 13); a call of a function of two arguments 4,000 (3,683; compiled,
  1,146); a tail loop's step (`if`, `zero?`, `-`, the call) 6,500 (6,059; compiled, 632); `map` with a function of
  one argument 4,000 an item (3,652; compiled, 2,955); a full
  collection of a full 64K cell heap 3,600,000 (213 cycles a live cell: 14,000 conses live, 3.8 M).  The `hyspeed`
  test checks the four of the evaluator on every run, the `heap` test the collector's (255 a cell, 9,000 live).
  They're phase 8's: the plan's were 150, 1,500, 3,000, 2,000 and 1,500,000, targets set before a spike, and
  tuning got the first hylang's 10,600 cycles a step (and this one's 10,960, untuned) to 6,059.  What's left is
  the evaluator's shape: a scope is a frame of name-value pairs looked up by name, and a call not quick (below)
  pushes a frame of the machine's.  Phase 8's tuning: a function's formals counted as it's made; quick calls, a
  built-in with a fixnum way (`+`, `-`, `1+`, `1-`, `zero?`, `one?`, the comparisons) given fixnums or names
  of them, worked where they're met (an argument, `if`'s condition, a call), no frame and no far call; an ordinary
  built-in's arguments counted as they're evaluated; a global's symbol read once; `pop`, `cell_get` and the
  lookups' derefs quicker; and a cell bank more after a collection while fewer pages are free than used.

## The phases (phase 7, again; now its phase 8)

0. **The spec**: danlang's fixes, its rules made one, its reference (`reference.md`), its cleanup; the old hylang
   deleted; the suite copied here.  Done.
1. **The runtime**: the module's four banks, the memory map, the cell and blob heaps, symbols, fixnums, the collector;
   a measure of 16-bit values against real programs.  Done: `modules/hylang` (a program of four banks, as yet its
   heap made and each bank answering), `heap.inc`, and the heap test (`tests/mod/t_heap`: a million cells made and
   dropped with none lost, the heap grown to 16 banks and E_NOMEM).
2. **Reader, printer and REPL**.  Done: `read.inc` and `print.inc`, and the REPL (it prints what a line reads to,
   till phase 3 evaluates it); the emulator's `hylang` test, and 250 lines read by both danlang and hylang the same
   (but for the numbers phase 5 reads), with a collection before every allocation too.
3. **The evaluator**: frames, scopes, the special forms, tail calls, partial application, errors, Ctrl-C.  Done:
   `eval.inc`, `forms.inc`, `builtins.inc` (the table, all danlang's built-ins), with the built-ins the suite's
   files need (lists, equality and order, output, fixnums), `load` (a file an item at a time), the REPL
   evaluating; `eval.dl` (its numbers past a fixnum made smaller), `scope.dl`, `control.dl` and `errors.dl` pass
   (221 checks), and a tail loop of 50,000 steps (the `hysuite` test).
4. **Built-ins and lists**, and the library's built-ins.  Done: danlang first (`map`, `filter` and the folds
   given what isn't a function: the evaluator's error, not a .NET exception; 1,182 checks); `filter`, `foldl`,
   `foldr`, `any?`, `all?`, `find`, `count`, `sum`, `product` (one walk, with `map`, in `forms.inc`), `sort` (a
   merge sort, stable, by `cmp` or by `less`), `subset`, `index-of` and `last-index-of` (lists and strings),
   `gensym`, `to-atom`, `random`; `load` finds a bare name in `/lib/hylang`.  `lists.dl`, `types.dl` and
   `library.dl` pass but for their checks of phase 5's numbers (and of a hash and a stream), with danlang's
   library loaded from a card's `/lib/hylang` (`globals.dl` but its constants, `dice.dl`, `screen.dl`): 510 checks
   with phase 3's files (the `hysuite` test).
5. **Numbers**: bignums, the tower, every base.  Done: danlang first (`to-rational` and a fraction in a base other
   than 10 give an integer when it's whole; the bit, byte and path built-ins' argument errors keep their own
   messages, code `:inval`; 1,188 checks); `numreg.inc` (integers in registers), `numval.inc` (the tower),
   `numtext.inc` (written, read in danlang's grammar, written in a base), `numbi.inc` (the arithmetic and
   conversions, `fib`, `random`), `numbits.inc` (bits and bytes).  `numbers.dl` passes, `bits.dl` but for its
   streams (phase 7), and `eval.dl` and `library.dl` whole: 738 checks with the rest so far (the `hysuite` test);
   and 2,100 random expressions and number texts give the same in danlang and hylang.
6. **Strings, characters and hashes**.  Done: danlang first (a hash's bad entry, override or tag is an error that
   says what; `hash-clone`'s overrides each an argument of its own; 1,195 checks); `strs.inc` (the string and
   character built-ins), `hashes.inc` (hashes, their built-ins, methods and `&0`); hashes printed, compared,
   ordered, counted by `len`; and print's form shows what's in a list as repr does, as danlang's.  `strings.dl`
   and `hashes.dl` pass, and `types.dl` whole but its stream: 929 checks with the rest so far (the `hysuite`
   test); and 2,800 random expressions of strings and hashes give the same in danlang and hylang, 150 more with
   a collection before every allocation.
7. **Streams, I/O and the system library**.  Done: danlang first (`save` writes an empty list `{}`, so it reads
   back, and a file in a folder that isn't there is `:noent`; library.dl's check of `dice+` wanted 4 to 13 of
   4 to 14; 1,197 checks); modules of up to eight banks (the SDK, the build, the ROM image, kdev's `#m`), and
   hylang's fifth: `sys.inc`, `streams.inc` (the streams, `save`, `read`), `system.inc` (files, `glob`,
   programs and the shell, the environment, the clock); `load` of several files; script mode and `args`.
   `io.dl` and `system.dl` pass, and so does every file of `run.dl`'s: 1,198 checks run as a script (a script of
   hylang's own, the `hysuite` test), with danlang's library loaded first.
8. **The library** (`globals.hl`, `dice.hl`, `screen.hl`), tuning to the budgets, the ROM.  Done: the library
   on the ROM disk (`romfs/lib/hylang`, danlang's `lib/` whole), `globals.hl` in hylang as it starts, from its
   snapshot (the module `hysnap`, made at the build) or as text; `run.dl` passes whole as danlang runs it, `hylang
   run.dl` (1,197 checks, the `hysuite` test), and `hytext` starts hylang without the snapshot.  Tuned: a loop's
   step 10,960 cycles to 6,059, a call 8,400 to 3,683; the budgets set to what tuning reached (the user's choice:
   above), each met and checked (`hyspeed`).

Then the Hydra layers (the plan's phases 9 to 12: its section "The Hydra layers"):

9. **The Hydra's built-ins**.  Done: `hydrabi.inc` in the fifth bank (room made: the collector moved to a sixth),
   the table above's: the system, tasks, memory, namespaces, notes, raw keys; `hold` a special form, preemption
   back on after an error too (a frame that takes errors), and when the machine's stack is given up.  `on-note`'s
   function is called by the evaluator at the next call (the note handler only notes the note, as it comes between
   any two instructions), with Ctrl-C's too.  `newns` is hylang in `/lib/hylang/newns.hl`, loaded: a child's
   namespace is its own once it changes it, so rc's `newns` can't build hylang's.  `tests/hyhydra/hydra.hl`'s 62
   checks pass, and the built-ins that make values with a collection before every allocation too (the `hyhydra`
   test, which also types raw keys and gives Ctrl-C to an `on-note` function).  248 built-ins of 256.
10. **Every system call** (`sys-`).  Done: `hysys.inc` in the sixth bank, `sys` and the `sys-` functions bound as
   they're first looked up, from `spec/api.def`'s `hl:` lines (`obj/gen/hylsys.inc`, apigen's).
   `tests/hyhydra/hydra.hl`'s 125 checks (a call of each group, and the errors) pass, and with a collection before
   every allocation too.  249 built-ins of 256.
11. **The device libraries**.  Done: `/lib/hylang`'s `gpio`, `i2c`, `spi`, `cons`, `proc`, `clock`, `disk`, `pc` and
   `snd` (and `dev`, what they share), the table above's, each hylang over its device's files.
   `tests/hyhydra/devices.hl`'s 67 checks pass against the emulated devices (the `hydev` test: the pins and CA1,
   two I2C memories, an SPI echo device, a card, the DS1747, `/pc`), and a tune's key-ons on the YM2151 keep time.
12. **The prompt**.  Done: `hylang -l`, `login.hl`, `profile.hl` and `shell.hl` (the rule, rc's lines, the shell's own
   commands, the prompt), and the REPL's hooks (`shell-line`, `shell-prompt`).  The `hysh` test runs the rc test's
   lines that stand alone at hylang's prompt, as at rc's, and the shell's own; `hywin` has a card's `/lib/shell`
   name `/bin/hylang -l`, and window 0 and a window made start in hylang.

Then danlang's `feature/speed` (October 2026, to `744d4db`): its speed is its own, and what it changed in the language
hylang has too.  Buffers (`buffers.inc`, a new kind of cell, `PK_BUFFER`: a string's, its blob changed in place),
`open`'s `:update`, `clock`, `buffer-cmp` and `round` (library code: `hylib.hl` loads `globals.hl`, danlang's `globals.dl` as it
is, then hylang's own), `key` and `key?` (the Hydra's already), and a file's read error with its line.  The rest was
there already: values shared, not copied; an integer key and an atom's different keys; 64-bit edges.  The suite's
1,334 checks pass (`hysuite1` to `hysuite5`).  The table of built-ins is full: 256 of 256, so danlang's next built-ins
need library code, or a wider table.
