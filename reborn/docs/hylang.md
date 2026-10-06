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

**The conformance suite is danlang's regression suite**, `tests/regress/` (1,197 checks), copied to `tests/hylang`
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
| **`load` and `use`** | A path as it is, or with `.hl`; then a bare name is `/lib/hylang/name` or `name.hl`, through the namespace (as forth's `/lib/forth`), so a card's or the RAM disk's `/lib/hylang` adds to the ROM's | Plan 9 names |
| **Streams** | Over the system's fds: `stdin`, `stdout`, `stderr` are fds 0-2; `output-of` points fd 1 at a buffer meanwhile | |
| **Ctrl-C** | The window's `interrupt` note: the error `interrupted` (`:intr`) at the next call or loop step | As forth's THROW -28 |
| **`random`** | A generator (a 16-bit xorshift) seeded by the tick count as it's first wanted | |
| **Start-up** | `/lib/hylang/globals.hl` (danlang's `globals.dl`) as it is when loaded, from a snapshot in the ROM (the module `hysnap`, made at the build); a ROM without it, loaded as text; then `/lib/hylang/profile.hl` if there is one | Loaded as text, it takes 8 M cycles (2.3 s); from the snapshot, 286,000 (0.08 s) |
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
| **Files** | `read-file`, `read-lines`, `write-file`, `append-file`, `ls`, `dir`, `stat` (a hash: `:name`, `:length`, `:dir`, `:mtime`), `exists?`, `dir?`, `file?`, `mkdir`, `remove`, `rename`, `copy-file`, `cd`, `cwd`, `glob` (rc's `*`, `?`, `[...]`, `[~...]`) | `stat` adds `:mode`, `:qid` and `:dev` (the device letter); `rename` is in its own directory (`WSTAT`'s, as `mv`'s); `cd` alone goes to `/` |
| **Programs** | `(run prog args...)` (its exit code), `(sh line [input])`, `(sh-out line [input])` (its output, a string), `(spawn prog args...)` (a task, not waited for), `(wait task)`, `(kill task)`, `pid` | A bare name is `/bin`'s; the shell is rc (`rc -c`); a task is its number (0-15); `kill` is the `kill` note; an exit status that's a number (rc's `exit 3`: the code 1, the message `3`) is that number |
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
  collection while the prompt waits too.  The collector's code is in the fifth bank, entered by a far call.
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
* **The module**: the core (all of danlang) is one program of five banks (a module may have eight since phase
  7): the evaluator, its special forms, the dispatch and the built-ins that run the machine in the first; the
  reader, the printer, the list built-ins, equality and order in the second; the numbers (and, as yet, `fn`, the
  type tests and `error`) in the third; strings, hashes and the errors' messages in the fourth; streams and the
  system library (and the collector) in the fifth.  What every bank calls is in the task's RAM (the heap, the
  output, the evaluation stack): the most of that code is kept in the fourth bank and copied to the RAM as hylang
  starts (`hylang.cfg`'s DATA4), so the first bank's room is the evaluator's.  `+`, `-`, `1+`, `1-`, `zero?`,
  `one?` and the comparisons work fixnums in the first bank (`bi_fast`), without a far call.  The Hydra layers
  are library modules beside it.
* **Budgets** (at 3.58 MHz, the library loaded; each from the REPL's echo to its `=>`, a difference of two lines'
  times so the REPL's own work drops out): start-up from the snapshot to the first prompt 300,000 cycles (286,000);
  a parameter looked up 300 (279); a call of a function of two arguments 4,000 (3,683); a tail loop's step (`if`,
  `zero?`, `-`, the call) 6,500 (6,059); `map` with a function of one argument 4,000 an item (3,652); a full
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

Then the Hydra layers: its built-ins, the `sys-` functions, the device libraries, and the prompt.
