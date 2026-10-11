# HyForth

HyForth (`forth`) is the Hydra-16's Forth: a **Forth 2012** system whose core is in the paged ROM, with the rest of the
standard's word sets as pre-compiled libraries it loads as they're wanted.  It passes the Forth 2012 test suite's tests
of Core, Core Extension, Exception, Facility, File Access, Programming-Tools, Search-Order, String, Double-Number,
Locals, Memory-Allocation and Block.  It's also **the login shell**: at its prompt, a line is Forth if its first word
is a Forth word or a number, and an rc command line otherwise.

Its sources are `reborn/modules/forth` (the core) and `reborn/forthlib` (the libraries).

## Contents

1. [Starting it](#starting-it)
2. [The basics](#the-basics)
3. [Libraries](#libraries)
4. [HyForth as a shell](#hyforth-as-a-shell)
5. [Files](#files)
6. [The Hydra's words](#the-hydras-words)
7. [The terminal and keys](#the-terminal-and-keys)
8. [Sound](#sound)
9. [Devices](#devices)
10. [Tools: words, see, disasm](#tools-words-see-disasm)
11. [Memory-Allocation, Locals and Blocks](#memory-allocation-locals-and-blocks)
12. [Numbers](#numbers)
13. [Errors and Ctrl-C](#errors-and-ctrl-c)
14. [Memory](#memory)
15. [How it's built](#how-its-built)

---

## Starting it

| Typed at the shell's prompt | What runs |
| :--- | :--- |
| `forth` | Forth at the console: each line typed is run, then ` ok` |
| `forth -l` | Forth as a login shell: its namespace made, `startup.fs` and `profile.fs` run, then its prompt (`/> `) |
| `forth file.fs a b` | A script: the file run, then forth ends (with code 1 after an error).  A file whose first line is `#!/bin/forth` runs by its name too |

`bye` ends a plain forth, and `exit` the shell (`exits ( n -- )` with a code).  A script's arguments are `argc ( -- n )`
and `arg ( n -- c-addr u )`: `forth file.fs a b` has 3, the file's name `0 arg`.

---

## The basics

Words run left to right; numbers go on the data stack, and words take their arguments from it.  Names are in lower case
and found in either case (`DUP` is `dup`).

```
/> 2 3 + .
5
/> 1 2 3 .s
<3> 1 2 3
/> . . .
3 2 1
```

**Numbers** are 16 bits, read in `base` (decimal at the start).  A prefix gives another: `$ff` hex, `#10` decimal,
`%101` binary, `'a'` a character's code.  `hex` and `decimal` set `base`; `.` prints signed, `u.` unsigned.

```
/> $ff . #10 . %101 . 'a' .
255 10 5 97
/> -1 u.
65535
```

**Definitions** are Forth 2012's: `: name ... ;`, `constant`, `variable`, `value` (and `to`), `create` and `allot`,
`defer` and `is`, `marker`.  The control structures (`if` `else` `then`, `begin` `until`, `begin` `while` `repeat`, `do`
`loop`, `?do`, `case` `of` `endof` `endcase`) work in definitions.

```
/> : sq ( n -- n*n ) dup * ;
/> 7 sq .
49
/> : count-to ( n -- ) 0 ?do i . loop ;
/> 3 count-to
0 1 2
/> see sq
: sq dup * ;
```

**Compile-only words** (the return stack's `>r`, `r>`, `i`; the control structures; `;`, `literal`, `postpone`, `."`)
give an error outside a definition, and forth goes on.  `.( text)` is `."`'s form for the prompt.

```
/> 1 >r
>r: compile only
```

---

## Libraries

The core is Forth 2012's Core word set.  As it starts, forth loads `/lib/forth/startup.fs`, which loads Core Extension,
Exception, File Access and Programming-Tools; `forth -l` adds the shell's.  The rest are loaded when they're wanted:
`lib name` at the prompt, or `require name.fl` in a program.

```
/> libs
forth coreext exception file tools shell
/> lib hydra
/> lib random
/> -lib random
/> libs
forth coreext exception file tools shell hydra (random)
```

`libs` lists the core, then each library; `(random)` is one `-lib` took out of the search (its code stays, so what was
compiled with it still runs), and `lib random` puts it back.  `lib name` loads `name.fl`, else `name.fs`, from the
current directory or `/lib/forth`.  A `marker` made before a library takes it out with the rest.

| Library | Words |
| :--- | :--- |
| `coreext.fl` | Core Extension: `?do`, `case`, `value`, `defer`, `:noname`, `pick`, `roll`, `.(`, `c"`, `s\"` ... (at the start) |
| `exception.fl` | `catch`, `throw` (at the start) |
| `file.fl` | File Access: `open-file`, `read-file`, `read-line`, `write-file`, `include`, `require` ... (at the start) |
| `tools.fl` | Programming-Tools: `.s`, `?`, `dump`, `see`, `words`, `[if]`, `synonym` ...; `libs`, `lib`, `-lib` (at the start) |
| `shell.fl` | [The shell](#hyforth-as-a-shell) (`forth -l`'s) |
| `facility.fl` | Facility: `key?`, `ms`, `time&date`, `page`, `at-xy`, structures; [the terminal's words and keys](#the-terminal-and-keys) |
| `string.fl` | String: `compare`, `search`, `/string`, `-trailing`, `sliteral`, `substitute`, `replaces` ... |
| `search.fl` | Search-Order: `wordlist`, `set-order`, `also`, `only`, `previous`, `definitions` ...; `library name` ... `end-library` |
| `double.fl` | Double-Number: `d+`, `d-`, `d.`, `d.r`, `d<`, `d=`, `dmax`, `m*/`, `2constant`, `2value`, `2rot` ... |
| `memory.fl` | [Memory-Allocation](#memory-allocation-locals-and-blocks): `allocate`, `free`, `resize` |
| `locals.fl` | [Locals](#memory-allocation-locals-and-blocks): `{: ... :}`, `(local)`, `locals\|` |
| `block.fl` | [Block](#memory-allocation-locals-and-blocks): `block`, `buffer`, `update`, `flush`, `load`, `list`, `thru` ... |
| `hydra.fl` | [The Hydra's words](#the-hydras-words): the `sys-` words, `sh`, `run`, banks, directories, notes, `argc`, `arg`, `sys`, `ctl` |
| `hydra.fs` | The system's constants and error codes (`o_rdwr`, `e_noent` ...), in a word list of their own, `hydra` |
| `disasm.fl` | `disasm`; with it, `see` of an assembly word shows its instructions |
| `bits.fl` | `tbit`, `sbit`, `cbit` |
| `random.fl` | `random ( u -- u' )` (0 to u-1), `rand`, `rand32`, `rseed` |
| `sound.fl` | [Sound](#sound) |
| `numbers.fl` | [Numbers](#numbers): exact numbers of any size on a number stack |
| `gpio.fs` `i2c.fs` `spi.fs` `cons.fs` `proc.fs` `clock.fs` `disk.fs` `pc.fs` `video.fs` | [Devices](#devices), in Forth source |

Your own libraries are `.fs` files: put them where `lib` looks, or in a card's `/lib/forth` (a card's `/lib` is bound
before the ROM's, so a card's `startup.fs` or `profile.fs` takes the place of the ROM's too).

---

## HyForth as a shell

Under `forth -l`, a line whose first word is a Forth word or a number is Forth; any other line goes to rc, whole (`rc
-c`), so pipes, redirections, quotes and `$name` are rc's.  `%` at a line's start sends it to rc whatever its first
word is.

```
/> ls /rom/lib/forth
bits.fl
clock.fs
...
/> echo hello | wc -c
      6
/> foo
rc: foo: not found
```

Each rc line is an rc of its own, so a variable it sets ends with it ([at HyForth's prompt](../05-rc.md#at-hyforths-prompt)).

**The shell's own words.**  An rc line runs in a task of its own, which can't change forth's directory or namespace, so
these are Forth words that read their arguments as rc does: `cd [dir]`, `bind [-a|-b] [-c] new old`, `mount`,
`unmount`, `newns`.

**The prompt** is a format: `s" %v%d> " prompt` (the start's).  `%v` the card (`0:`), `%d` the directory on it, `%p` the
whole path, `%t` the task, `%w` the window, `%%` a `%`.  While a definition goes on over more than one line, the prompt
is `prompt2`'s.

```
/> s" %t:%p%% " prompt
4:/% cd /rom/lib
4:/rom/lib% s" %v%d> " prompt
/rom/lib>
```

**Statuses.**  `status ( -- n )` is the last rc line's code, and `$status` rc's, the program's message.  A line ending in
`&` isn't waited for: its task is `$apid`, and `wait ( task -- )` waits for it.

```
/> nosuchprogram
rc: nosuchprogram: not found
/> status .
1
/> echo $status
not found
```

**The environment**: `getenv ( c-addr1 u1 -- c-addr2 u2 )`, `setenv ( c-addr1 u1 c-addr2 u2 -- )` (the name first),
`unsetenv ( c-addr u -- )`.  Forth's environment is the shell's, so what it sets the rc lines see:

```
/> s" greeting" s" hello" setenv
/> echo $greeting
hello
```

**Programs as values:**

| Word | Stack | Does |
| :--- | :--- | :--- |
| `sh` | `( c-addr u -- n )` | An rc command line run, waited for: its code |
| `run` | `( c-addr u -- n )` | A program run (its name and arguments, no rc), waited for |
| `sh-out` | `( c-addr u -- c-addr2 u2 )` | An rc line's output, as a string (`status` its code) |
| `output-of` | `( xt -- c-addr u )` | What a word writes, as a string |
| `\|` | `( xt "line" -- )` | A word's output into the rest of the line: `' words \| wc -l` |
| `piped` | `( xt c-addr u -- )` | The same, the command line a string |
| `spawn` | `( c-addr u -- task )` | A program not waited for |
| `send` | `"n line"` | The line typed in window n, as if at its keyboard |

```
/> s" echo hi there" sh-out type
hi there
/> ' words | wc -l
    115
```

The strings `sh-out` and `output-of` give are in the dictionary's free space: use them before anything's added to it
(`move` one somewhere to keep it).

---

## Files

File Access's words work on the system's files: a fileid is an fd, and an ior is 0 or -512 less the system's error
code, whose text `ior>text` gives (in `hydra.fl`).

```
/> s" /none" r/o open-file . .
-544 0
/> s" /none" r/o open-file nip ior>text type
not found
/> create buf 64 allot
/> s" /rom/README" r/o open-file throw value fd
/> buf 40 fd read-file throw buf swap type
The Hydra-16's ROM disk
================
/> fd close-file throw
```

`include file` and `require file` load source; a name with no `/` that isn't in the current directory is
`/lib/forth`'s.  In `hydra.fl`, Gforth's words for directories: `open-dir ( c-addr u -- dirid ior )`, `read-dir (
c-addr u1 dirid -- u2 flag ior )`, `close-dir`, `get-dir ( c-addr u1 -- c-addr u2 )`, `set-dir ( c-addr u -- ior )`,
`=mkdir ( c-addr u mode -- ior )`.  `ctl ( c-addr1 u1 c-addr2 u2 -- )` writes the text c-addr2 u2 to the file c-addr1
u1 in one write, as a device's `ctl` wants (`s" /dev/sndctl" s" volume 100" ctl`).

---

## The Hydra's words

`lib hydra` (`hydra.fl`) has a word for **each system call**, `sys-` and its name in lower case (`sys-getpid`,
`sys-sleep`, `sys-banks-alloc` ...; [the call reference](../09-programming/calls-reference.md) gives each one's stack),
and `require hydra.fs` the system's constants (`o_rdwr`, `e_noent` ..., in their own word list).

| Words | Does |
| :--- | :--- |
| `note ( task n -- )`, `note-group ( group n -- )` | A note sent to a task, or a note group |
| `on-note ( xt -- )` | The notes that come (but Ctrl-C and kill) given to `xt ( n -- flag )`: true, forth goes on; false, as Ctrl-C |
| `pause` | The other tasks' turn |
| `bank! ( bank -- )`, `bank@`, `bank-window ( -- addr )`, `seg-bank!` | A RAM bank of the task's at `$8000` ([memory](#memory)) |
| `code-banks ( flag -- )` | False: colon definitions from now on compiled into the dictionary; true (as forth starts): into the task's banks |
| `sys ( addr a x y -- a x y p )` | Machine code called with its registers, its flags after |
| `sh`, `run`, `ctl`, `argc`, `arg`, `ior>text`, the directories' | As above |

```
/> : h ( n -- flag ) ." note " . true ;
/> ' h on-note sys-getpid 16 note 7 .
note 16 7
```

---

## The terminal and keys

The console is an ANSI terminal.  `lib facility` has, beside `page` and `at-xy ( x y -- )`: `clear-line`, `clear-below`,
`cursor-up ( n -- )` (and `-down`, `-right`, `-left`), `cursor-save`, `cursor-restore`, `cursor-off`, `cursor-on`,
`form ( -- rows cols )`; colours as C's conio numbers them, `color ( n -- )` and `bgcolor ( n -- )`, with `black` ...
`white` and `bright`; `plain`, `bold`, `dim`, `underline`, `blink`, `reverse`; `beep`.  Keys: `key` and `key?`, and
`ekey`, `ekey?`, `ekey>char`, `ekey>fkey` with `k-up`, `k-down` ... `k-f12` for the special keys.

```
: hello  page  red color bold ." Hello"  plain  0 2 at-xy ;
```

---

## Sound

`lib sound` drives the sound driver ([sound](../04-drivers/sound.md)), with C's and hylang's names, the channel first:

| Word | Does |
| :--- | :--- |
| `snd-reset`, `snd-volume ( v -- )` | Everything cleared; the master volume (0-200) |
| `snd-claim ( mask -- )`, `snd-release`, `snd-claim-psg ( mask -- )`, `snd-release-psg` | Claim the YM2151's channels (bit n, channel n), or the PSG's (bit n, channel 8 + n) |
| `snd-patch ( ch n -- )`, `snd-note ( ch note -- )`, `snd-off ( ch -- )` | A patch; a note on (MIDI's: 60 middle C); off |
| `snd-level ( ch v -- )`, `snd-pan ( ch pan -- )`, `snd-bend ( ch n -- )` | Volume (0-127), speakers (1 left, 2 right, 3 both), pitch bend |
| `snd-drum ( ch n -- )`, `snd-freq ( ch hz -- )`, `snd-glide ( ch n -- )` | A General MIDI drum; a frequency; a slide to a note |
| `snd-wave ( ch w width -- )` | A PSG voice's waveform (0 pulse, 1 saw, 2 triangle, 3 noise) and width |
| `snd-lfo ( rate pmd amd wave -- )`, `snd-sens ( ch pms ams -- )`, `snd-noise ( n -- )` | The YM2151's LFO, sensitivity, noise |
| `snd-reg ( reg value -- )`, `snd-regs ( c-addr -- )` | A register written; all 256 as written |
| `note-of ( c-addr u -- n )`, `tune ( c-addr u ch tempo -- )` | A note's name as its number; a string of notes and beats played |
| `snd-play ( c-addr u times -- status )`, `snd-mml ( ch c-addr u -- status )`, `snd-chord ( ch c-addr u -- status )` | `play` run: a song, a line of MML, a chord |

```
/> s" C#4" note-of .
61
/> s" C4 1 E4 1 - 1 G4 2" 0 240 tune
```

---

## Devices

The device libraries are Forth source over the devices' files, with hylang's names; each uses only the words forth
starts with, so each is also an example of driving its device by hand.  A failure THROWs the system's error.

| Library | Words |
| :--- | :--- |
| `gpio` | `gpio ( pin -- level )`, `gpio! ( pin level -- )`, `gpio-in ( pin -- )`, `gpio-out`, `gpio-port ( -- byte )`, `gpio-port!`, `gpio-ddr! ( byte -- )`, `gpio-ca1! ( rise? -- )`, `gpio-ca2! ( n -- )`, `gpio-wait ( -- count )` (CA1's next edge), `gpio-state ( -- c-addr u )` |
| `i2c` | `i2c-read`, `i2c-write ( addr reg c-addr u -- )`, `i2c-speed ( khz -- )`, `i2c-reg-size ( n -- )`, `i2c-devices`, `i2c? ( addr -- flag )` |
| `spi` | `spi ( dev c-addr u -- )` (a transaction: what came back in the bytes' place), `spi-read ( dev c-addr u -- )`, `spi-mode ( dev mode -- )` |
| `cons` | `window ( -- n )`, `windows ( -- c-addr u )`, `new-window ( c-addr u -- )`, `new-group`, `show-window ( n -- )`, `window-label`, `window-status`, `window-ctl ( c-addr u -- )` |
| `proc` | `task-args`, `task-cwd`, `task-env`, `task-ns`, `task-regs ( task -- c-addr u )`, `task-mem ( task addr c-addr u -- )`, `task-ram ( task bank offset c-addr u -- )` |
| `clock` | `set-date ( c-addr u -- )`, `rtc ( -- c-addr u )` (`running`, `stopped`, `none`) |
| `disk` | `disk-ctl ( disk -- c-addr u )` (a disk by its letter), `disk-start`, `disk-stop ( disk -- )`, `cards ( -- mask )` |
| `video` | The Vera X: `bitmap`, `pen`, `plot`, `line`, `box`, `bar`, `circle`, `disc`, `text`, `clear`; the turtle (`cs`, `fd`, `rt` ...); `vpoke`, `vpeek`, `vram!`, `vram@`, `palette!`, `sprite!`, `sprite-at`, `sprite-off`, `vsync`, `border`, `mouse`, `mouse-wait` ([video](../04-drivers/video.md#drawing)) |
| `pc` | `pc? ( -- flag )`: the PC tool answers |

```
/> lib gpio
/> 2 gpio .
1
/> 4 1 gpio!
/> lib i2c
/> i2c-devices
50
/> 1 i2c-reg-size  $50 0 s" hello" i2c-write  $50 0 pad 5 i2c-read  pad 5 type
hello
/> lib proc
/> sys-getpid task-cwd type
/rom/lib
```

(Pin 2 high; a memory chip at I2C address `$50`, its registers a byte's address.)

---

## Tools: words, see, disasm

`words` lists the first word list in the search order, newest first: each word's code address in hex, three flags, and
its name.  `l` a literal (a constant), `i` immediate, `a` or `f` assembly or Forth.

```
/> 5 constant five
/> words
 41EC l-f five            41D5 --f h               409F --a sys-rtc
 ...
 A271 --a swap            A268 --a drop            A256 --a dup
```

`see name` shows a definition; with `lib disasm`, an assembly word's instructions, and `disasm ( addr n -- )` any code's,
each as `as` writes it.  `dump ( addr u -- )` shows memory, `.s` the stack, `? ( addr -- )` a cell.

```
/> lib disasm
/> see 2drop
code 2drop
 A2E3  E8        inx
 A2E4  E8        inx
 A2E5  60        rts
end-code
```

---

## Memory-Allocation, Locals and Blocks

**`lib memory`**: `allocate ( u -- a-addr ior )`, `free ( a-addr -- ior )`, `resize ( a-addr u -- a-addr2 ior )`.  The
heap is the top of the dictionary's space.  At the shell's prompt, Forth's `free` then shadows the `free` program: `%
free` runs the program.

**`lib locals`**: a definition's named values, Forth 2012's way.  Between `{:` and `:}`: arguments taken from the stack
(the last is its top), then after `|` values that start undefined, then after `--` a comment.  A local's name pushes its
value, `to name` sets it.

```
/> lib locals
/> : lt7 {: a b :} b a ; 7 8 lt7 . .
7 8
/> : lt12 {: a | b c :} 20 to b a 21 to a 22 to c a c b ; 19 lt12 .s
<4> 19 21 22 20
```

**`lib block`**: Forth's blocks, each 1024 bytes of a file, `blocks.fb` in the current directory (made when first
wanted) or the one `s" name" open-blocks` names.  `n block` gives a block's buffer, `update` marks it changed, `flush`
writes the changed ones; `n load` interprets a block, `a b thru` blocks a to b, `n list` shows one.  Blocks are kept in
the task's RAM banks too (128 of them).

```
/ram> lib block
/ram> 1 block 1024 bl fill  s" 2 3 + . blk @ . \ the rest 7 ." 1 block swap move  update flush
/ram> 1 load
5 1
```

---

## Numbers

**`lib numbers`**: the Hydra's numbers in Forth, the same as hylang's and HyBASIC's ([the numbers](README.md#the-numbers-they-share)):
integers of any size, fixed decimals, rationals and complex numbers, all exact, and the math functions.  They have a
stack of their own, the **number stack** (64 numbers, in a RAM bank), and the words have hylang's names with an `n`
before them where Forth has the name.  A number in your text that Forth doesn't read as a cell or a double goes on the
number stack: `1.25`, `2/3`, `#xFF`, `2i`, `1+2i`, `100000000000000000000`, and a cell past 16 bits (`70000`).

```
/> lib numbers
/> 1/3 1/6 n+ n.
1/2
/> 0.1 0.2 n+ 0.3 n= .
-1
/> 2 s>n 100 s>n npow n.
1267650600228229401496703205376
/> 2 s>n nsqrt n.  npi n.
1.41421356237 3.14159265359
/> : harm ( n -- ) ( N: -- h ) 0 s>n 1+ 1 do 1 s>n i s>n n/ n+ loop ;  20 harm n.
55835135/15519504
/> 255 s>n 255 s>n 255 s>n s" {} is {x} and {#b}" nformat
255 is FF and #b11111111
```

| What | Words (the number stack's effect after `N:`) |
| :--- | :--- |
| The number stack | `ndepth ( -- u )`, `ndrop`, `ndup`, `nover`, `nswap`, `nrot`, `n.s` |
| Arithmetic | `n+`, `n-`, `n*`, `n/` (exact: `10/4` is `5/2`), `n/mod`, `nnegate`, `nabs`, `ngcd`, `npow ( N: x y -- x^y )` |
| Comparisons and tests | `n=`, `n<`, `n> ( -- flag ) ( N: x y -- )`, `n0=`, `n0<`, `ncompare`; `int?`, `fixed?`, `rational?`, `complex?` |
| Cells and doubles | `s>n ( n -- )`, `d>n ( d -- )`; `n>s ( -- n )`, `n>d ( -- d )` |
| Conversions | `truncate`, `nfloor`, `nround`, `to-fixed ( u -- )`, `to-rational`, `rational.n`, `rational.d`, `complex`, `nrandom`, `nseed`, `nfib` |
| Bits (any size) | `nand`, `nor`, `nxor`, `ninvert`, `nlshift`, `nrshift ( u -- )`, `nbit? ( u -- flag )` |
| The math functions | `nsqrt`, `nexp`, `nlog`, `nsin`, `ncos`, `ntan`, `natan`, `npi`; `digits ( -- a-addr )`, their precision (1-100 significant digits: 12) |
| Text | `n.`, `n.base ( c-addr u -- )`, `n>str ( -- c-addr u )`, `>n ( c-addr u -- flag )`, `nformat ( c-addr u -- )` |
| Bytes | `nbytes ( -- c-addr u )`, `nfrom-bytes ( c-addr u -- )` |
| Variables | `nvariable name`, `n@`, `n!`, `nconstant name`, `nvalue name`, `nliteral` |

**The base**: `base` is the numbers' base as it is the cells': after `hex`, `n.` shows `FF`.  `s" b" set-base` selects
any of hylang's bases by its name: `s" #x"` hexadecimal with its prefix shown, `s" c"` balanced ternary, `s" [01]"`
digits of your own ...; `get-base` gives it.

Division by zero is THROW -10, a number past the libraries' limits -11, the number stack empty -45 or full -44.  Each
word is a call of the library: about 1.7 ms for `1 s>n n+`, more as the numbers grow.

---

## Errors and Ctrl-C

An error says what it was (with the file and line, in a file being included) and empties both stacks; the prompt comes
back.  A word that isn't there is `name ?` in a plain forth (under `forth -l` the line goes to rc, which says it isn't
a program); the standard's errors have their text (`stack underflow`, `division by zero`), and the system's are its own
(`not found`).  `catch` and `throw` are Forth 2012's.

**Ctrl-C** stops the word running (THROW -28, `interrupt`), whatever it's doing: a loop, a wait for a key, a program `sh`
or `run` started (which it ends too).

---

## Memory

* **The dictionary** is forth's RAM after its own variables, up to `$7F00`: about 18K free at `forth -l`'s prompt
  (`unused`).
* **A colon definition's code goes to a RAM bank**, taken as they're wanted: the dictionary keeps only its header and 6
  bytes, so 100 definitions of a couple of hundred bytes take about 1.2K of the dictionary, not 22K.  A call to a word in
  another bank costs about 90 cycles more.
* **Libraries** load into the dictionary.  `-lib` doesn't free anything; a `marker` does (`marker -work` ... `-work`).
* **The task's banks** (16 of 8K for each memory card) are at `$8000` through `bank!`: take them with `sys-banks-alloc` so
  your program and the libraries don't use the same ones.  Forth keeps the index of its words in the task's last bank.
* A definition whose code is in a bank can't select another with `bank!` (its code would vanish from under it): it
  THROWs -21.  Compile such a word with code banks off:

```
false code-banks
: fill-bank ( bank -- ) bank@ swap bank!  bank-window 8192 0 fill  bank! ;
true code-banks
```

---

## How it's built

* **The core** (`modules/forth`) is a module of the paged ROM, run in place: the inner interpreter, Core's words in
  assembly, the text interpreter, the compiler, and the words that load files.  Its code is shared by every task
  running forth.
* **Subroutine threaded**: a word's execution token is its code's address, and a colon definition compiles to a run of
  `jsr` instructions (literals and `if`'s test inline, a few short words copied in whole).  The **data stack** is in the
  program's zero page, its low and high bytes apart, indexed by `.X`; the return stack is the 65C02's.
* **The libraries** (`forthlib/*.s`) are assembled by the build into `.fl` files: pre-compiled images loaded into the
  dictionary at `here` and relocated, with their headers.  A library calls the core by its labels, never another library;
  a library is for the core it was built with.
* **The index**: a hash of every word's name, kept in a RAM bank, so a word is found without reading the whole
  dictionary.
* **Benchmarks**: HyForth is the Hydra's fastest language: over twenty benchmarks, hylang takes 1.9 times its time and
  HyBASIC 7.4 times ([how fast](README.md#how-fast)).
