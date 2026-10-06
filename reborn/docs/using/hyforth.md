# Using HyForth

HyForth is the Hydra-16's Forth: a Forth 2012 system whose core is in the paged ROM, with the rest of the
standard's word sets as libraries it loads into its dictionary as they're wanted.  It's also a shell: `forth -l`
runs a line as Forth when its first word is a Forth word or a number, and as an rc command line otherwise.  This
guide is for using it; [../hyforth.md](../hyforth.md) is its design (and why each piece is as it is), and
[../forth-status.md](../forth-status.md) where it stands.  The first HyForth, the old system's, had a guide of its
own (`docs/using/hyforth.md` at the repository's top); this one is the new system's.

Contents: [Starting it](#starting-it) · [The basics](#the-basics) · [Libraries](#libraries) ·
[The shell](#the-shell) · [Files](#files) · [The Hydra's words](#the-hydras-words) ·
[The terminal and keys](#the-terminal-and-keys) · [Sound](#sound) · [Devices](#devices) · [Tools](#tools) ·
[Memory, locals and blocks](#memory-locals-and-blocks) · [Errors and Ctrl-C](#errors-and-ctrl-c) · [Memory](#memory)

## Starting it

| Typed at rc's prompt | What runs |
| :--- | :--- |
| `forth` | Forth at the console: each line typed is run, then ` ok` |
| `forth -l` | Forth as a login shell: its namespace made, `startup.fs` and `profile.fs` run, then its prompt (`/> `) |
| `forth file.fs a b` | A script: the file run, then forth ends (with code 1 after an error).  A file whose first line is `#!/bin/forth` runs by its name too (`./file.fs a b`) |

`bye` ends a plain forth, and `exit` the shell (`exits ( n -- )` with a code).  To make HyForth every window's
shell, put a line `/bin/forth -l` in `/lib/shell` on a card (`/sd/0/lib/shell`) or the shared RAM disk: init reads
it for window 0, and each window made (Ctrl-] c) gets the same.

A script's arguments are `argc ( -- n )` and `arg ( n -- c-addr u )` (in `hydra.fl`): `forth file.fs a b` has 3,
the file's name `0 arg`.  At the console, and under `forth -l`, there are none.

## The basics

Words are run left to right; numbers go on the data stack, and words take their arguments from it.  Names are in
lower case and found in either case (`DUP` is `dup`).

```
/> 2 3 + .
5
/> 1 2 3 .s
<3> 1 2 3
/> . . .
3 2 1
```

**Numbers** are 16 bits, read in `base` (decimal at the start).  A prefix gives another: `$ff` hex, `#10`
decimal, `%101` binary, `'a'` a character's code.  `hex` and `decimal` set `base`; `.` prints signed, `u.`
unsigned.

```
/> $ff . #10 . %101 . 'a' .
255 10 5 97
/> -1 u.
65535
```

**Definitions** are Forth 2012's: `: name ... ;`, `constant`, `variable`, `value` (and `to`), `create` and
`allot`, `defer` and `is`, `marker`.  The control structures (`if` `else` `then`, `begin` `until`, `begin`
`while` `repeat`, `do` `loop`, `?do`, `case` `of` `endof` `endcase`) work in definitions.

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

**Compile-only words.**  The words Forth 2012 leaves undefined outside a definition (the return stack's: `>r`,
`r>`, `i`, `unloop` ...; the control structures'; `;`, `literal`, `postpone`, `."` ...) give an error there,
`-14`, and forth goes on.  `.( text)` is `."`'s form for the prompt.

```
/> 1 >r
>r: compile only
```

## Libraries

The core is Forth 2012's Core word set.  As it starts, forth loads `/lib/forth/startup.fs`, which loads Core
Extension, Exception, File Access and Programming-Tools; `forth -l` adds the shell's.  The rest are loaded when
they're wanted: `lib name` at the prompt, or `require name.fl` in a program.

```
/> libs
forth coreext exception file tools shell
/> lib hydra
/> lib random
/> -lib random
/> libs
forth coreext exception file tools shell hydra (random)
```

`libs` lists the core, then each library; `(random)` is one `-lib` took out of the search (its code stays, so
what was compiled with it still runs), and `lib random` puts it back where it was.  `lib name` loads `name.fl`,
else `name.fs`, from the current directory or `/lib/forth`.  A `marker` made before a library takes it out with
the rest.

| Library | Words |
| :--- | :--- |
| `coreext.fl` | Core Extension: `?do`, `case`, `value`, `defer`, `:noname`, `pick`, `roll`, `.(`, `c"`, `s\"` ... (at the start) |
| `exception.fl` | `catch`, `throw` (at the start) |
| `file.fl` | File Access: `open-file`, `read-file`, `read-line`, `write-file`, `include`, `require` ... (at the start) |
| `tools.fl` | Programming-Tools: `.s`, `?`, `dump`, `see`, `words`, `[if]`, `synonym` ...; `libs`, `lib`, `-lib` (at the start) |
| `shell.fl` | [The shell](#the-shell) (`forth -l`'s) |
| `facility.fl` | Facility: `key?`, `ms`, `time&date`, `page`, `at-xy`, structures; [the terminal's words and keys](#the-terminal-and-keys) |
| `string.fl` | String: `compare`, `search`, `/string`, `-trailing`, `sliteral`, `substitute`, `replaces` ... |
| `search.fl` | Search-Order: `wordlist`, `set-order`, `also`, `only`, `previous`, `definitions` ...; `library name` ... `end-library`, a source library's words in a word list of its own (`hydra.fs`'s) |
| `double.fl` | Double-Number: `d+`, `d-`, `d.`, `d.r`, `d<`, `d=`, `dmax`, `m*/`, `2constant`, `2value`, `2rot` ... |
| `memory.fl` | [Memory-Allocation](#memory-locals-and-blocks): `allocate`, `free`, `resize` |
| `locals.fl` | [Locals](#memory-locals-and-blocks): `{: ... :}`, `(local)`, `locals\|` |
| `block.fl` | [Block](#memory-locals-and-blocks): `block`, `buffer`, `update`, `flush`, `load`, `list`, `thru` ... |
| `hydra.fl` | [The Hydra's words](#the-hydras-words): the sys- words, `sh`, `run`, banks, directories, notes, `argc`, `arg`, `sys`, `ctl` |
| `hydra.fs` | The system's constants and error codes (`O_RDWR`, `E_NOENT` ...), in a word list of their own, `hydra` |
| `disasm.fl` | `disasm`; with it, `see` of an assembly word shows its instructions |
| `bits.fl` | `tbit`, `sbit`, `cbit` |
| `random.fl` | `random ( u -- u' )` (0 to u-1), `rand`, `rand32`, `rseed` |
| `sound.fl` | [Sound](#sound): the YM2151's words, `note-of`, `tune` |
| `gpio.fs` `i2c.fs` `spi.fs` `cons.fs` `proc.fs` `clock.fs` `disk.fs` `pc.fs` | [Devices](#devices), in Forth source |

Your own libraries are `.fs` files: put them where `lib` looks, or on a card's `/lib/forth` (the cards' `/lib` is
bound before the ROM's, so a card's `startup.fs` or `profile.fs` takes the place of the ROM's too).

## The shell

Under `forth -l`, a line whose first word is a Forth word or a number is Forth; any other line goes to rc, whole
(`rc -c`), so pipes, redirections, quotes and `$name` are rc's.  `%` at a line's start sends it to rc whatever its
first word is.

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

**Words of the shell.**  An rc line runs in a task of its own, which can't change forth's directory or namespace,
so these are Forth words that read their arguments as rc does: `cd [dir]` (none: `$home`), `bind [-a|-b] [-c] new
old`, `mount`, `unmount`, `newns`.

**The prompt** is a format: `s" %v%d> " prompt` (the start's).  `%v` the card (`0:`), `%d` the directory on it,
`%p` the whole path, `%t` the task, `%w` the window, `%%` a `%`.  While a definition is being compiled over more
than one line the prompt is the second one, `prompt2`'s (a tab at the start).

```
/> s" %t:%p%% " prompt
4:/% cd /rom/lib
4:/rom/lib% s" %v%d> " prompt
/rom/lib>
```

**Statuses.**  `status ( -- n )` is the last rc line's code, and `$status` rc's, the program's message.  A line
ending in `&` isn't waited for: its task is `$apid`, and `wait ( task -- )` waits for it.

```
/> nosuchprogram
rc: nosuchprogram: not found
/> status .
1
/> echo $status
not found
```

**The environment**: `getenv ( c-addr1 u1 -- c-addr2 u2 )`, `setenv ( c-addr1 u1 c-addr2 u2 -- )` (the name
first), `unsetenv ( c-addr u -- )`.

```
/> s" greeting" s" hello" setenv
/> echo $greeting
hello
```

**Programs as values.**

| Word | Stack | Does |
| :--- | :--- | :--- |
| `sh` | `( c-addr u -- n )` | An rc command line run, waited for: its code |
| `run` | `( c-addr u -- n )` | A program run (its name and arguments, no rc), waited for |
| `sh-out` | `( c-addr u -- c-addr2 u2 )` | An rc line's output, as a string (`status` its code) |
| `output-of` | `( xt -- c-addr u )` | What a word writes, as a string |
| `\|` | `( xt "line" -- )` | A word's output the rest of the line's input: `' words \| wc -l` |
| `piped` | `( xt c-addr u -- )` | The same, the command line a string |
| `spawn` | `( c-addr u -- task )` | A program not waited for |
| `send` | `"n line"` | The line typed in window n, as if at its keyboard |

```
/> s" echo hi there" sh-out type
hi there
/> ' words | wc -l
    115
```

The strings `sh-out` and `output-of` give are in the dictionary's free space: use them before anything's added to
it (`move` one somewhere to keep it).

## Files

File Access's words work on the system's files: a fileid is an fd, and an ior is 0 or -512 less the system's
error code, whose text `ior>text` gives (in `hydra.fl`).

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
c-addr u1 dirid -- u2 flag ior )` (a name at a time), `close-dir`, `get-dir ( c-addr u1 -- c-addr u2 )` (the
current directory), `set-dir ( c-addr u -- ior )`, `=mkdir ( c-addr u mode -- ior )`.  `ctl ( c-addr1 u1
c-addr2 u2 -- )` writes the text c-addr2 u2 to the file c-addr1 u1 in one write, as a device's ctl wants (`s"
/dev/sndctl" s" volume 100" ctl`).

## The Hydra's words

`lib hydra` (`hydra.fl`) has a word for each system call, `sys-` and its name in lower case (`sys-getpid`,
`sys-sleep`, `sys-banks-alloc` ...; `/rom/doc/api.md` lists them, with each one's stack), and the system's
constants in Forth are `require hydra.fs` (`O_RDWR`, `E_NOENT` ..., in their own word list).

| Words | Does |
| :--- | :--- |
| `note ( task n -- )`, `note-group ( group n -- )` | A note sent to a task, or a note group (Plan 9's) |
| `on-note ( xt -- )` | The notes that come (but Ctrl-C and kill) given to `xt ( n -- flag )`: true, forth goes on; false, as Ctrl-C |
| `pause` | The other tasks' turn |
| `bank! ( bank -- )`, `bank@`, `bank-window ( -- addr )`, `seg-bank!` | A RAM bank of the task's at `$8000` (`sys-banks-alloc` gives them) |
| `sys ( addr a x y -- a x y p )` | Machine code called with its registers, its flags after |
| `sh`, `run`, `ctl`, `argc`, `arg`, `ior>text`, the directories' | As above |

```
/> : h ( n -- flag ) ." note " . true ;
/> ' h on-note sys-getpid 16 note 7 .
note 16 7
```

## The terminal and keys

The console is an ANSI terminal.  `lib facility` has, beside `page` and `at-xy ( x y -- )`: `clear-line`,
`clear-below`, `cursor-up ( n -- )` (and `-down`, `-right`, `-left`), `cursor-save`, `cursor-restore`,
`cursor-off`, `cursor-on`, `form ( -- rows cols )`; colours as C's conio numbers them, `color ( n -- )` and
`bgcolor ( n -- )`, with `black` ... `white` and `bright`; `plain`, `bold`, `dim`, `underline`, `blink`,
`reverse`; `beep`.  Keys: `key` and `key?`, and `ekey`, `ekey?`, `ekey>char`, `ekey>fkey` with `k-up`,
`k-down` ... `k-f12` for the special keys.

```
: hello  page  red color bold ." Hello"  plain  0 2 at-xy ;
```

## Sound

`lib sound` drives the YM2151 (`#a`), with C's and hylang's names, the channel first: `snd-reset`,
`snd-volume ( v -- )` (0-200), `snd-claim ( ch -- )`, `snd-release`, `snd-patch ( ch n -- )`, `snd-note ( ch
note -- )` (a MIDI number: 60 middle C), `snd-off ( ch -- )`, `snd-vol ( ch v -- )`, `snd-pan ( ch pan -- )`
(1 left, 2 right, 3 both), `snd-bend ( ch n -- )`, `snd-drum ( ch n -- )` (General MIDI's drum n), `snd-reg (
reg value -- )`.  `note-of ( c-addr u -- n )` turns a note's name into its
number, and `tune ( c-addr u ch tempo -- )` plays a string of notes and their beats (`-` a rest) at a tempo,
beats a minute; Ctrl-C ends it.  Songs (ZSM files) are the `play` program's.

```
/> s" C#4" note-of .
61
/> s" C4 1 E4 1 - 1 G4 2" 0 240 tune
```

## Devices

The device libraries are Forth source over the devices' files, with hylang's names; each uses only the words
forth starts with, so each is also an example of driving its device by hand.  A failure THROWs the system's
error; a text one gives is in its buffer till its next.

| Library | Words |
| :--- | :--- |
| `gpio` | `gpio ( pin -- level )`, `gpio! ( pin level -- )`, `gpio-in ( pin -- )`, `gpio-out`, `gpio-port ( -- byte )`, `gpio-port!`, `gpio-ddr! ( byte -- )`, `gpio-ca1! ( rise? -- )`, `gpio-ca2! ( n -- )`, `gpio-wait ( -- count )` (CA1's next edge), `gpio-state ( -- c-addr u )` |
| `i2c` | `i2c-read`, `i2c-write ( addr reg c-addr u -- )`, `i2c-speed ( khz -- )`, `i2c-reg-size ( n -- )`, `i2c-devices`, `i2c? ( addr -- flag )` |
| `spi` | `spi ( dev c-addr u -- )` (a transaction: what came back in the bytes' place), `spi-read ( dev c-addr u -- )`, `spi-mode ( dev mode -- )` |
| `cons` | `window ( -- n )`, `windows ( -- c-addr u )`, `new-window`, `show-window ( n -- )` |
| `proc` | `task-args`, `task-cwd`, `task-env`, `task-ns`, `task-regs ( task -- c-addr u )`, `task-mem ( task addr c-addr u -- )`, `task-ram ( task bank offset c-addr u -- )` |
| `clock` | `set-date ( c-addr u -- )` (`s" 2026-10-04 12:00:00" set-date`), `rtc ( -- c-addr u )` (`running`, `stopped`, `none`) |
| `disk` | `disk-ctl ( disk -- c-addr u )` (a disk by its letter: `[char] x` the ROM disk, `[char] 0` card 0), `disk-start`, `disk-stop ( disk -- )`, `cards ( -- mask )` |
| `pc` | `pc? ( -- flag )`: the PC tool answers (`/pc`'s files are files for everything else) |

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

(Pin 2 high; a memory chip at I2C address $50, its registers a byte's address.)

## Tools

`words` lists the first word list in the search order, newest first: each word's code address in hex, three
flags, and its name.  `l` a literal (a constant), `i` immediate, `a` or `f` assembly (the core's or a library's)
or Forth (yours).

```
/> 5 constant five
/> words
 41EC l-f five            41D5 --f h               409F --a sys-rtc
 ...
 A271 --a swap            A268 --a drop            A256 --a dup
```

`see name` shows a definition; with `lib disasm`, an assembly word's instructions, and `disasm ( addr n -- )`
any code's.  `dump ( addr u -- )` shows memory, `.s` the stack, `? ( addr -- )` a cell.

```
/> lib disasm
/> see 2drop
code 2drop
 A2E3  E8        inx
 A2E4  E8        inx
 A2E5  60        rts
end-code
```

## Memory, locals and blocks

**`lib memory`**: `allocate ( u -- a-addr ior )`, `free ( a-addr -- ior )`, `resize ( a-addr u -- a-addr2 ior )`.
The heap is the top of the dictionary's space, so what's allocated is space the dictionary hasn't; freeing gives
it back.  At the shell's prompt, Forth's `free` then shadows the `free` program: `% free` runs the program.

**`lib locals`**: a definition's named values, Forth 2012's way.  Between `{:` and `:}`: arguments, taken from the
stack (the last is its top), then after `|` values that start undefined, then after `--` a comment.  A local's
name pushes its value, `to name` sets it; it hides a word or number of that name till the definition's end.

```
/> lib locals
/> : lt7 {: a b :} b a ; 7 8 lt7 . .
7 8
/> : lt12 {: a | b c :} 20 to b a 21 to a 22 to c a c b ; 19 lt12 .s
<4> 19 21 22 20
```

**`lib block`**: Forth's blocks, each 1024 bytes of a file, `blocks.fb` in the current directory (made when it's
first wanted) or the one `s" name" open-blocks` names.  `n block` gives a block's buffer (read in), `update` marks
it changed, `flush` writes the changed ones; `n load` interprets a block (`\` skips to its 64-character line's end),
`a b thru` blocks a to b, `n list` shows one.

```
/ram> lib block
/ram> 1 block 1024 bl fill  s" 2 3 + . blk @ . \ the rest 7 ." 1 block swap move  update flush
/ram> 1 load
5 1
```

## Errors and Ctrl-C

An error says what it was (with the file and line, in a file being included) and empties both stacks; the prompt
comes back.  A word that isn't there is `name ?` in a plain forth (under `forth -l` the line goes to rc, which
says it isn't a program); the standard's errors have their text (`stack underflow`, `division by zero`, `compile
only` ...), and the system's are its own (`not found`).  `catch` and `throw` are Forth 2012's.

Ctrl-C stops the word running (THROW -28, `interrupt`), whatever it's doing: a loop, a wait for a key, a program
`sh` or `run` started (which it ends too).

## Memory

The dictionary is forth's RAM after its own variables, up to `$7F00`: about 18K free at `forth -l`'s prompt
(`unused`).  Libraries load into it; `-lib` takes a library out of the search but keeps its code, so it doesn't
free anything, while a `marker` does (`marker -work` ... `-work` takes back everything made since).  `allocate`'s
heap (`lib memory`) takes its space from the top, so `unused` counts what's below it.  PAD and the other buffers
are forth's own.  The task's RAM banks (16 of 8K for each memory module) are at `$8000`
through `bank!`: take them with `sys-banks-alloc` so your program and libraries don't use the same ones.
forth keeps the index of its words (by which it finds a name without reading every one) in the task's last
bank, which it takes as it starts, so a program that writes to a bank it didn't take can spoil forth's search for
words.
