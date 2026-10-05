# hylang

hylang is danlang on the Hydra-16: the lisp the plan makes the preferred command shell
([../../docs/reimplementation-from-scratch.md](../../docs/reimplementation-from-scratch.md), §17; phase 7).  It's
being written again from scratch, in 65C02 assembly: the first hylang (7.1 to 7.4a, to commit `a0973eb`) was deleted
in October 2026, after a review of danlang and of it, and the new one is built to the plan that review led to, "danlang:
review and 65C02 plan" (a Claude Docs document: <https://claude.ai/code/artifact/32e0e85e-18a6-4e48-9ded-99898b50f43f>).
This is its specification: the language, where it may differ from danlang, what it adds for the Hydra, and its design.

## The reference

**hylang is danlang**: the C# interpreter in `C:\source\danlang` (<https://github.com/SNSTRUTHERS/danlang>, its
`master`), whose `reference.md` specifies the language: its syntax, its evaluation, and every built-in's arguments,
value and errors.  danlang's review (October 2026) fixed its bugs and its quirks there first, and settled every rule
that was inconsistent, so the two are one language: text is bytes; `[a b c]` is a list of values; the shorthand is
seven prefixes (`?` if, `=` set, `:` def, `#` hash-create, `@` fn, `.` unpack, `~` format); a hash is called to look a
key up (`(h :k)`, a method `(obj :add 3)`); extra arguments are `&1`, `&2` ... past the formals; `$name` is the
environment's variable; every ordinary built-in gets its arguments' values, the first error stopping it.

**The conformance suite is danlang's regression suite**, `tests/regress/` (1,155 checks), copied to `tests/hylang`
(its README says which phase runs which file).  It's written in danlang, so hylang runs it unchanged, from an
emulated card (`hylang run.dl`, status 0 when every check passes).  A change to the language is made in danlang
first, with its checks, then in hylang.  What only the Hydra has is checked by a file of its own, `hydra.dl`.

## Where hylang differs

| Area | hylang | Why |
| :--- | :----- | :-- |
| **Integers** | Up to 255 bytes (about 614 digits; danlang's have no limit); 15 bits in the value, an object past that | Most numbers are small; the rest costs only when used |
| **Call depth** | About 2,500 calls nested, not in tail position (danlang 10,000); deeper is danlang's error | The evaluation stack: 8K in the task's RAM, spilled to 4 banks |
| **`range`, strings, lists** | As memory allows (danlang caps `range` at 1,000,000) | Memory |
| **`load` and `use`** | A bare name is `/lib/hylang/name.hl`, through the namespace (as forth's `/lib/forth`), so a card's or the RAM disk's `/lib/hylang` adds to the ROM's | Plan 9 names |
| **Streams** | Over the system's fds: `stdin`, `stdout`, `stderr` are fds 0-2; `output-of` points fd 1 at a buffer meanwhile | |
| **Ctrl-C** | The window's `interrupt` note: the error `interrupted` (`:intr`) at the next call or loop step | As forth's THROW -28 |
| **`random`** | The kernel's entropy and a generator | |
| **Start-up** | `/lib/hylang/globals.hl` (danlang's `globals.dl`), loaded as text, then `/lib/hylang/profile.hl` if there is one | A ROM snapshot of the loaded heap later, if starting is too slow |
| **Files** | `.hl` (the suite keeps danlang's `.dl` names, loaded by their whole names) | |

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
| **Files** | `read-file`, `read-lines`, `write-file`, `append-file`, `ls`, `dir`, `stat` (a hash: `:name`, `:length`, `:dir`, `:mtime`), `exists?`, `dir?`, `file?`, `mkdir`, `remove`, `rename`, `copy-file`, `cd`, `cwd`, `glob` (rc's `*`, `?`, `[...]`, `[~...]`) | `stat` adds `:mode`, `:qid` and `:dev` (the device letter); `rename` is in its own directory (`WSTAT`'s, as `mv`'s) |
| **Programs** | `(run prog args...)` (its exit code), `(sh line [input])`, `(sh-out line [input])` (its output, a string), `(spawn prog args...)` (a task, not waited for), `(wait task)`, `(kill task)`, `pid` | A bare name is `/bin`'s; the shell is rc (`rc -c`); a task is its number (0-15); `kill` is the `kill` note |
| **Environment** | `(env name)`, `$name`, `(env)` (a hash of them all), `setenv`, `unsetenv` | `/env`'s, rc's variables: one of several words is a list of strings; `setenv` of a list makes one |
| **The clock** | `time` (seconds since 2000-01-01), `date`, `date-parts`, `seconds-of`, `ticks`, `tick-rate` (200), `sleep` (seconds: `(sleep 1/10)`) | `TIME`, `TICKS`, `SLEEP`; `sleep` ends early, with `:intr`, on a note |
| **Bits and bytes** | `bit-and`, `bit-or`, `bit-xor`, `bit-not`, `shl`, `shr`, `bit?`, `hex`, `bin`, `lo`, `hi`, `word`, `bytes`, `from-bytes`, `read-bytes`, `write-bytes` | |
| **Where** | `(platform)`, `(hydra?)` | `:hydra`, T |
| **The screen** | `(use "screen")`: `cls`, `at`, `color`, `bold`, `plain`, `clear-line`, `cursor-off`, `cursor-on` | The console is a terminal (ANSI) |

### 2. The Hydra's built-ins

| Area | Functions |
| :--- | :-------- |
| **Notes** | `(note task n)` (Plan 9's postnote: `n` is `:interrupt`, `:kill`, `:hangup`, `:alarm`, or 16-31, a program's own), `(note-group group n)`; `(on-note f)`: `f` is called with each note (as an atom) and returns T to go on, NIL for the default |
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

## The prompt

hylang is the console's shell (after parity), over rc: **a line that starts with `(`, `{`, `[`, or a prefix right
against `(` or `{` (`?(`, `=(`, `#(` ...) is hylang; any other is an rc command line** (the plan's §17.4), so
`ls -l | wc` works as in rc, and `(map print (ls "/bin"))` as lisp.  rc's own forms stay rc's: `$x`, `# comment`,
`. file`, `~ subject pattern`; rc's `@{...}` is written `@ {...}`.  A value is printed as the REPL's (`repr`); a
line that doesn't close goes on with the open brackets shown, as danlang's does, each line edited by the console.
Ctrl-C at the prompt stops what's running and the prompt comes back.

## The design

The plan has it whole; in short:

* **Values** are 16-bit words: a fixnum (15 bits, bit 0 set), a reference to a 4-byte cell (its own address: bits
  15-13 a bank of 8, bits 12-0 the offset in the `$8000` window), or an immediate (a character, a built-in, T).  NIL
  is cell 0.  Lists, strings and numbers are immutable and shared, never copied; hashes, streams and scopes change.
* **Memory**: each 256-byte page of the cell heap holds one kind of cell (a 256-byte table in the task's RAM gives a
  value's type); a list is its first cons, its page saying code or data; strings, bignums and hash tables are blobs
  in banks of their own, each owned by one cell; symbols are interned and hold their global value; a scope is a
  frame, its symbol-value pairs side by side.  Mark and sweep (marking by pointer reversal, a free list per page
  kind), and a collection while the prompt waits too.  Banks are taken as they're needed (up to 8 for cells, 16 for
  blobs, 4 for the stack).
* **The evaluator** is a loop over a stack of frames (8K in the task's RAM, spilled to banks), never the 65C02's
  stack: tail calls take no frame; an ordinary built-in gets its arguments' values from it; `map`, `filter`, the
  folds and the loops are frames too, so what they call nests like any call and Ctrl-C stops it.
* **The module**: the core (all of danlang) is one program of four banks: the evaluator, heap and dispatch in the
  first, with the hot built-ins; the reader, printer and list built-ins; the numbers; strings, hashes, streams and
  the system.  The Hydra layers are library modules beside it.
* **Budgets** (at 3.58 MHz, to confirm in phase 1): a parameter looked up in 150 cycles, a call of two arguments in
  1,500, a tail loop's step in 3,000 (the first hylang's: 10,600), a full collection in 1.5 M.

## The phases (phase 7, again)

0. **The spec**: danlang's fixes, its rules made one, its reference (`reference.md`), its cleanup; the old hylang
   deleted; the suite copied here.  Done.
1. **The runtime**: the module's four banks, the memory map, the cell and blob heaps, symbols, fixnums, the collector;
   a measure of 16-bit values against real programs.
2. **Reader, printer and REPL**.
3. **The evaluator**: frames, scopes, the special forms, tail calls, partial application, errors, Ctrl-C.
4. **Built-ins and lists**, and the library's built-ins.
5. **Numbers**: bignums, the tower, every base.
6. **Strings, characters and hashes**.
7. **Streams, I/O and the system library**.
8. **The library** (`globals.hl`, `dice.hl`, `screen.hl`), tuning to the budgets, the ROM.

Then the Hydra layers: its built-ins, the `sys-` functions, the device libraries, and the prompt.
