# 7. The tools

The programs in `/bin`: the core tools in Plan 9's way, the editors, the task tools, the debugger, the assembler and
disassembler, and the system's.  They're RAM programs on the ROM disk (`/rom/bin`), each read into its task's RAM as
it starts (about 50 ms for one of 2.4K): most are near 2K, and the ROM disk keeps them in 1K clusters, where a module
of the paged ROM took a bank.  The paged ROM keeps what must run in place (the drivers, init, rc, the languages, `play`;
`/dev/mod/NAME`, bound into `/bin` by `#m/bin`).  `/bin` is a union, so a program of your own in `/ram/bin`,
`/sram/bin` or a card's `/bin` comes before the ROM's of the same name.

## How they behave

They behave as Plan 9's do:

* Flags come first (`ls -l`, `-abc` together), then names.  A tool that reads files reads fd 0 when it's given none, so
  each works in a pipeline (`cat f | wc`).
* A name that fails is said on fd 2 as `tool: name: why` (`ls: /none: not found`), the rest go on, and the tool ends
  with code 1.  A bad flag or too few names is `usage: ...`, and the status `usage`.
* `$status` after a tool is its exit message, or its code; empty or `0` is success.

Every tool runs the same way at rc's prompt and at HyForth's.  At HyForth's prompt, a tool whose name is also a Forth
word needs a `%` before it (`% free` when `memory.fl`'s `free` is loaded).

## Contents

1. [Files and text](#files-and-text)
2. [The editors: edit and ed](#the-editors-edit-and-ed)
3. [Tasks](#tasks)
4. [db: the debugger](#db-the-debugger)
5. [as: the assembler](#as-the-assembler)
6. [dis: the disassembler](#dis-the-disassembler)
7. [The system](#the-system)
8. [Others](#others)

The disks' tools (`df`, `mkfs`, `fsck`, `label`) are in [storage](04-drivers/storage.md#the-tools), and `play` in
[sound](04-drivers/sound.md#play-songs-scores-and-wav-files).

---

## Files and text

| Tool | What it does |
| :--- | :----------- |
| `ls [-ld] [name ...]` | A directory's entries, a line each (a `/` after a directory's); a file's own name.  `-l`: the mode, the device letter and instance, the length, the time, the name: `--rw-rw-rw- fr       13 2000-01-01 00:00 a`.  `-d`: a directory itself, not its entries.  None: the current directory |
| `cat [file ...]` | Each file to fd 1 (none: fd 0) |
| `cp [-r] from to`, `cp [-r] from ... dir` | Each file copied to a file made (one there already is emptied first), or into a directory by its own name.  `-r`: a directory's whole tree.  A file isn't copied onto itself |
| `mv from to`, `mv from ... dir` | Moved, or renamed: in its own directory a rename (a file there by the new name goes first); to another, a file is copied then removed, and a directory can't be moved |
| `rm [-rf] name ...` | Each file removed, or empty directory.  `-r`: a directory's tree too, the deepest first; `-f`: quiet about what can't be |
| `mkdir [-p] dir ...` | Each directory made.  `-p`: the directories it's in too, as needed, and no complaint about one there |
| `rmdir dir ...` | Each empty directory removed |
| `touch name ...` | Each file made empty if it isn't there, or its time changed if it is |
| `du [-a] [name ...]` | The kilobytes each tree takes (its files' lengths rounded up to whole KB), a line a directory, the deepest first; `-a`: a line a file too |
| `pwd` | The current directory |
| `cmp file1 file2` | Compared byte by byte: the first that differs said (`file1 file2 differ: byte 5`), or nothing, and the status |

```
/> mkdir /ram/notes
/> echo first >/ram/notes/a; echo second >>/ram/notes/a
/> cp /ram/notes/a /ram/notes/b; ls -l /ram/notes
--rw-rw-rw- fr       13 2000-01-01 00:00 a
--rw-rw-rw- fr       13 2000-01-01 00:00 b
/> rm -r /ram/notes
```

### Text

| Tool | What it does |
| :--- | :----------- |
| `echo [-n] [words]` | The words, a space between each, and a new line (`-n`: none) |
| `wc [-lwc] [file ...]` | Lines, words and bytes (`-l`, `-w`, `-c`: only those), a line a file, and a total for several |
| `head [-N] [file ...]` | Each file's first N lines (10) |
| `tail [-N] [file]` | The last N lines (10); the file's last 16K are what it looks at |
| `grep [-chilnsv] [-e] pattern [file ...]` | The lines that match a regular expression, Plan 9's (`.`, `[a-z]`, `[^a]`, `^`, `$`, `*`, `+`, `?`, `\|`, `( )`): `-c` the count, `-l` the files' names, `-n` with line numbers, `-i` case ignored, `-v` the lines that don't, `-s` no messages, `-h` no file names; status `no matches` when none did |
| `sort [-bfnru] [file ...]` | The lines in order: by bytes; `-n` by the number each starts with; `-f` case folded; `-b` leading blanks ignored; `-r` reversed; `-u` one of each run of equal lines.  Stable; some 20K of lines at most |
| `uniq [-c] [file]` | Each run of the same line once; `-c` with its count |
| `tee [-a] [file ...]` | Fd 0 to fd 1 and to each file (`-a`: added to its end) |
| `xd [file ...]` | Bytes in hex, 16 a line after their offset, then as text: `0000010  68 65 6c 6c 6f 0a    hello.` |
| `more [file ...]` | A screen at a time (22 lines, then `--more--`): Enter for the next, `q` to stop.  Its keys come from the console even when fd 0 is the file |
| `ed [file]` | The line editor, ed's way: `p`, `a`, `i`, `c`, `d`, `w [name]`, `q` (twice if the text's changed), `Q`, `h`, with line numbers (`1,5p`, `$`).  Lines are typed after `a`, `i` or `c` until a line of just `.`; Ctrl-C comes back to its `*` prompt |

```
/> ls /rom/bin | sort -r | head -3
xmodem
xd
wc
/> grep -n Hydra /rom/README
1:The Hydra-16's ROM disk
5:machine.  It's a HydraFS volume, as a card's is, in the paged ROM's banks after
```

---

## The editors: edit and ed

`edit [+N] [file ...]` is the screen editor, nano's way (`+N`: the next file at its line N; BASIC's `EDIT` uses
it): what you type goes in at the cursor, and the Ctrl keys and the Meta keys (Esc then a key, or Alt with it:
`M-`) are commands.  The two lines at the bottom name the commonest, and
`^G` shows them all.  Up to 6 files are open at once, each in a buffer of its own; their text is in your task's RAM
banks, so a file can be as big as they are (a few hundred K: `kdev.s`'s 140K reads in some 6 seconds).  A file
whose lines end CR LF is written with CR LF again.

| Keys | What they do |
| :--- | :----------- |
| `^O`, `^S` | Save (`^O` asks for the name: Enter keeps it) |
| `^X` | Close the file (if it's changed, saved or not: `y`, `n`); with the last closed, `edit` ends |
| `^R`; `M-,` `M-.` | Open another file; go to the file before, or after |
| `^W`; `M-W`, `M-Q` | Find (letters either case; round from the other end if need be); find again, find backwards |
| `M-R` | Replace: each match shown, `y` yes, `n` no, `a` all the rest |
| `M-G` | Go to a line (`$`: the last) |
| `^K`, `M-6`, `^U` | Cut the line (`^K` again: the next joins it in the cut buffer), copy it, paste (into any file); with the mark set (`M-A`), the block from the mark to the cursor |
| `M-U`, `M-E` | Undo, redo (a line typed, a cut, a paste: one step each) |
| the arrows, Home, End, PgUp, PgDn | Move (and `^B` `^F` `^P` `^N`, `^A` `^E`, `^Y` `^V`); `M-\` and `M-/` the text's start and end |
| Backspace; Del, `^D` | Rub out; delete |
| `M-I`, `M-X`, `^L` | Auto-indent on and off; the help lines off and on; the screen drawn again |

Ctrl-C does nothing in `edit` (it's the console's interrupt), and `^\` and `^]` never reach a program, so nano's keys
there are Meta keys here.  The screen is the window's size (`consctl`'s: the smaller of the terminals it's shown on; with no console `$COLUMNS` and `$LINES`, else 80 by 24), and `edit` draws itself again as it changes.  `ed` is
the line editor ([text](#text)), for scripts and a terminal without a screen.

---

## Tasks

| Tool | What it does |
| :--- | :----------- |
| `ps [-a]` | The tasks in use: number, state, parent, CPU time (seconds, to a tenth), note group, name; `-a`: their arguments too.  A stopped task's state is `stopped` |
| `top` | The same each second, with the CPU each took in it, the screen drawn again; Ctrl-C ends it |
| `kill [-i] task ...` | Each task (its number) ended with the kill note, which nothing catches; `-i`: interrupted instead (Ctrl-C's note), which a program may catch |
| `slay [-i] name ...` | Each task running a program of that name, as `kill` |
| `sleep seconds` | Nothing for that long; Ctrl-C ends it |
| `ns [task]` | A task's namespace (none: this one's) as the binds and mounts that make it |
| `new-window [-g] [command ...]` | A window made and shown, in this one's group (`-g`: a group of its own, another shell session), running the command (rc's: `new-window 'ls -l; sleep 5'`), or with none the shell (`/lib/shell`'s, as Ctrl-] c starts); `$window` is its number.  It isn't waited for, and the window goes when its program ends |
| `input` | The Vera X's keyboard and mouse (its input controller, the X16's SMC): their keys into the console, as the serial terminal's, and the mouse into `/dev/vid/mouse`.  init starts it; with no controller it ends at once |

The tasks' own files are under `/proc/N`: `status`, `args`, `cwd`, `env`, `ns`, `fd` (its open files: `0 rw #c 291
#c/cons`), `regs`, `mem` and `ram` (its memory), `note` (write `interrupt`, `kill`, `hangup` or a number to send
it one), and `ctl` (`kill`, `interrupt`, `note N`, `stop` and `start`: a stopped task doesn't run till it's started
again; `step`, `next`, `break` and `nobreak`: the debugger's: [db](#db-the-debugger)).  Another task's memory and registers are anyone's but the kernel task's and a driver's.

```
/> sleep 100 &
/> ps
task  state   parent     cpu group  name
...
   5  sleep    4         0.0     4  sleep
...
/> echo stop >/proc/5/ctl; cat /proc/5/status
sleep stopped 4 0 4
/> echo kill >/proc/5/ctl
```

---

## db: the debugger

`db program [arg ...]` starts a program stopped at its first instruction, in a task and note group of its own (so
Ctrl-C reaches `db`, which stops it); `db -p task` stops a task that's running, where it is.  Then it takes
commands, a line each, at its `db>` prompt.  An address is hex (`0830`, `$0830`) or a symbol, with `+` or `-` a hex
offset (`main+14`); a count is decimal.

| Command | What it does |
| :------ | :----------- |
| `r` | Its registers, and the instruction at its PC |
| `s [n]` | n instructions (1), a step at a time, into a subroutine at a `JSR` |
| `n [n]` | The same, but a `JSR`'s subroutine runs whole (a `JSR` into the kernel's jump table always does) |
| `c` | On, till a breakpoint, a `BRK`, its end, or Ctrl-C |
| `u addr` | Steps (as `n`) till its PC is addr |
| `b [addr]` | A breakpoint at addr (in RAM: a `BRK` written there while it runs), or the list |
| `x [addr]` | The breakpoint at addr gone, or all of them |
| `d [addr] [n]` | n instructions (12) from addr (its PC; then on from the last), disassembled as `as` writes them (the asm library's, as `dis`'s): a symbol for the address an operand names, where each is after a `;` |
| `m [addr] [n]` | n bytes (64) from addr, in hex and as text |
| `w addr byte ...` | Bytes (hex) written at addr |
| `l file` | Symbols from an ld65 label file (`ld65 -Ln`: the build makes one for each program, `obj/.../NAME.lbl`; `as -l`'s), or `/lib/as/hydra.inc`'s: the system calls' names and r0-r15 |
| `q` | Quit: a program `db` started is killed; a task it stopped runs on |

```
/> db /sd/0/sample/hi Ann Bob
task 4
PC=0830 A=30 X=FF Y=48 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0830  A5 02     lda $02
db> l /pc/hi.lbl
8 symbols
db> l /lib/as/hydra.inc
105 symbols
db> b main+14
db> c
breakpoint 1
PC=0844 A=41 X=FF Y=48 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0844  A9 52     lda #$52                 ; main+14
db> n 4
0846  85 02     sta r0                   ; main+16
0848  A9 08     lda #$08                 ; main+18
084A  85 03     sta r0+1                 ; main+1A
PC=084C A=08 X=FF Y=48 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
084C  20 53 F9  jsr PUTS                 ; main+1C
db> x
db> c
Hello, Ann!
Hello, Bob!
I'm task 4, in /, in window 0.
task 4 ended: code 0
```

A step can't be taken in the kernel: a task stopped in a call (`(in a call: its state 6)`, as Ctrl-C finds a
program in `SLEEP`) is run on with `c`.  Breakpoints are in its memory only while it runs, so `d` and `m` show its
own bytes; one in a ROM can't be set (`u` reaches an address there, a step at a time).  A `BRK` of the program's own
stops it too; `c` from there gives it its note (`sys: brk`), as it would have had.  A subroutine that reads the bytes
after its `JSR` (its arguments) can't be stepped over with `n`; step into it with `s`.  `db` works through
`/proc/N/ctl` and `mem`: `echo step >/proc/N/ctl` does the same by hand, on a task that's stopped.

---

## as: the assembler

`as [-bl] file.s|- [out|-]` assembles a program for the 65C02 on the Hydra itself, from the same source ca65 takes on
a PC: a RAM program, `out` (`file` without its `.s`, if there's no `out`), that runs as any program does.  `-l`
writes its labels too, `out.lbl` (ld65's `-Ln` form, which `db`'s `l` reads); `-b` writes the bytes alone,
no header, from `.org`'s address (`$0800` if there's none): a ROM's image, say.  A source `-` is standard input (a
pipe, or `< file`), and then the output is `-` too unless it's named; an output `-` is standard output: `dis prog |
as - copy` makes a program from its own disassembly in one line.  `-l` needs a named output.

The SDK's files are in `/lib/as`: `hydra.inc`, `hyx2.inc`, `macros.inc`, `toollib.inc` and `toollib.s`,
`srvlib.inc` and `srvlib.s`, `nslib.s`, and three of its samples, `hi.s`, `tick.s` and `upper.s`.  `.include`
and `.incbin` find a file as it's named, then beside the file that names it, then in `/lib/as`.  A program made
from the same source by ca65 and ld65 (`sdk/asm/hyx2.cfg`) is the same, byte for byte.

What it takes is ca65's, as the SDK's sources use it:

| Kind | What `as` takes |
| :--- | :--- |
| Instructions | The W65C02S's, each mode written as ca65 writes it; `a:` or `z:` before an address makes it absolute or zero page.  An address is on the zero page if it's known by then and under `$100` (a label further on is absolute) |
| Labels | `name:`; cheap locals, `@name:`, between one normal label and the next; unnamed ones, `:`, reached as `:+` `:++` ... and `:-` `:--` ...; constants, `name = expr` and `name := expr` |
| Expressions | 32 bits, with ca65's operators and their order (`* / .mod & ^ << >>`, `+ - \|`, the comparisons, `&& \|\| .and .or .xor .not`, unary `- ~ < > ^ !`), `*` (here), `$hex`, `%binary`, `'c'`, and `.lobyte` `.hibyte` `.bankbyte` `.loword` `.hiword` `.strlen` `.defined` `.blank` `.match` |
| Data | `.byte` (and strings), `.word`, `.addr`, `.dword`, `.res`, `.asciiz`, `.incbin "file" [, start [, count]]` |
| Files | `.include` |
| Conditions | `.if`, `.ifdef`, `.ifndef`, `.ifblank`, `.ifnblank`, `.elseif`, `.else`, `.endif` |
| Macros | `.macro name params` ... `.endmacro`, `.exitmacro`; an argument in `{ }` may have commas in it |
| Segments | `.segment "NAME"`, `.zeropage`, `.code`, `.rodata`, `.data`, `.bss`, `.pushseg`, `.popseg` |
| Checks | `.assert expr, error\|warning, "text"`, `.error`, `.warning` |
| Taken and left | `.import`, `.export`, `.global` (and their `zp` kinds), `.setcpu`, `.feature`, `.macpack`, `.debuginfo`, `.list`, `.case` ... |

Not there: `.proc` and `.scope`, `.repeat`, `.struct`, `.sprintf` and `.ident` (but in a branch that's not
taken), and objects to link: one source file and what it includes make one program.  Its segments are laid out as
`hyx2.cfg` lays them out: `ZEROPAGE` from `$22` (the program's `$5E` bytes), then from `$0800` `HEADER`, `CODE`,
`RODATA`, `DATA` and `BSS`, to `$8000` at most, with ld65's names for them (`__DATA_LOAD__`, `__BSS_RUN__`,
`__BSS_SIZE__`, `__RAM_LAST__` ...) and `HYX2_RAM` defined, as `hyx2.inc` needs.

The assembler is the **asm library**'s (`modules/asm`, `spec/asm.def`: [dis](#dis-the-disassembler)); `as` is its command line.  BASIC's
`ASM` blocks are assembled by the same code, a line at a time ([HyBASIC](06-languages/hybasic.md#inline-assembly)).

An error is said as `as: file:line: what`, and `as` ends with status 1; a warning is said, and the program made.  It
reads its source three times (the segments' sizes, then each symbol's value, then the bytes), each file read from
the disk once and kept in the task's RAM banks; a pass that finds errors is the last, so another pass's errors show
once they're mended.  `hi.s`, with `hydra.inc` (some 47K of source), takes about 4 seconds; `upper.s`, with
`toollib.s` too (83K), about 9.

```
/> as -l /sd/0/sample/as/hi.s /ram/hi
/> /ram/hi Ann
Hello, Ann!
I'm task 4, in /, in window 0.
/> db /ram/hi Ann
...
db> l /ram/hi.lbl
8 symbols
```

---

## dis: the disassembler

`dis [-cnw] [-l labels] [-o addr] file` is `as`'s inverse: a program written out as a source in `as`'s language, on
stdout, which `as` assembles to the same bytes (`dis prog | as - copy`, in one line).  `file` is a RAM program (HYX2,
as `as` and the build make them), or with `-o addr` raw bytes from addr (as `as -b` makes them).

* **Its code** is found by following it from where it starts (a program's `main`, or addr): each instruction to the
  next, and to where it jumps, branches or calls, till one that goes no further (`rts`, `jmp` ...).  What isn't
  reached is data: `.byte`, its text in quotes.  `-c`: every byte from the start is an instruction, in turn.
* **Its labels**: each place an instruction names in it, its symbol's name from `-l`'s label file (`as -l`'s, the
  build's `obj/.../NAME.lbl`) when one names just that place, else `Lnnnn` (`main` at a program's entry); a place
  just past a symbol is the symbol and an offset (`buf+1`).  Symbols outside it are defined first (`arg = $22`), its
  BSS's are labels in `.bss`, and the system calls and `r0`-`r15` are `hydra.inc`'s (`/lib/as/hydra.inc`; `-n`: not).
* **A program's header** is `HYX2_PROGRAM "name", main` when that makes the same bytes (`as`'s programs'), else its 48
  bytes; its data goes in `.data`, its BSS in `.bss`.
* **Its lines** are compact, a tab then the instruction (a 20K program's source is some 100K, which `as` reads into
  RAM banks whole); `-w`, the SDK's sources' columns.

```
/> dis /sd/0/sample/tick
; /sd/0/sample/tick, as dis read it: as assembles this to its bytes again
.include "hydra.inc"

.include "hyx2.inc"

        HYX2_PROGRAM "tick", main
.code
main:
        stz B0895
        ...
        jsr NOTIFY
L0841:
        lda #$C8
        ldx #$00
        jsr SLEEP
...
/> dis -l /pc/db.lbl /rom/bin/db >/ram/db.s; as /ram/db.s /ram/db2; cmp /rom/bin/db /ram/db2
```

The instructions are the **asm library**'s (`modules/asm`, `spec/asm.def`), which `db`'s `d` and HyForth's `disasm`
use too, so the three write them alike, and its assembler makes its encoding from the same table (`w65c02.inc`):
lower case, an address in hex or its name, `a:` before an absolute address under `$100` where the instruction has a
zero page form (`as` would take that one), an opcode the W65C02S doesn't define as the NOP it runs (`.byte` and its
bytes).  C has it as `dis_insn` (`asm.h`, with the symbol files' `lbl_load` ...), assembly as `ASMCALL ASM_DIS`
(`asmlib.inc`).

The library is one bank of the paged ROM: the assembler (`FILE`, `as`'s whole run; and a source
given a line at a time, BASIC's way: `BEGIN`, then for each pass `PASS`, the caller's symbols with `DEFINE`, the
lines with `LINE`, and `END`; then `SYMBOL`, `IMAGE` and `DONE`), and the disassembler (`DIS`).  It has
no RAM of its own: the assembler's state is in RAM its caller lends it while it runs (`ASM_RAM`, `$5000`-`$7DFF`),
and its zero page (`$22` on) is the caller's, put back as each call ends.

---

## The system

| Tool | What it does |
| :--- | :----------- |
| `date [-n]` | The clock (`2026-10-03 15:04:05`), or `-n` seconds since 2000-01-01.  Set it with `echo 2026-10-03 15:04:05 >/dev/time`, which sets the DS1747 too if there's one |
| `free` | RAM: each task's own banks (16 for each RAM module, 8K each), and the shared RAM: what segments have, what's free |
| `mods` | The paged ROM's modules: bank(s) (`4+33`: a small module sharing bank 4, its header at `$A000 + $3300`), type (program, driver, library; `boot` a driver started at boot), name |
| `hwtest` | The system starts again into the hardware test (as a T typed during POST does); the reset button ends it |

The kernel's messages (the boot's, POST's, a driver's) are `/dev/kmesg`, its last 4K.

---

## Others

| Tool | What it does |
| :--- | :----------- |
| `xmodem -r file`, `xmodem -s [-k] file` | A file received or sent with XMODEM over the serial line, with any terminal program on the PC: `-r` receives, `-s` sends (`-k`: 1K blocks) |
| `calc [-b base] [-d digits] [expression ...]` | An expression worked out exactly, with the Hydra's numbers (the number libraries', hylang's), its value in decimal or in the base `-b` names (hylang's base strings: `x`, `#x`, `b`, `c`, `16r` ...): `calc 2/3 + 0.5` is `7/6`, `calc sqrt 2` `1.41421356237`, `calc -b x 255` `FF`.  Numbers are read in decimal (`0.5`, `2i`), or in a base by their own prefix (`#xFF`); `+ - * /` (exactly), `%` (an integer's remainder), `^`, parentheses; `sqrt exp log sin cos tan atan` (`-d` digits: 12), `abs floor round truncate fib numerator denominator re im rational random`, `gcd(a, b)`, `pow(a, b)`, `complex(a, b)`, `fixed(a, places)`; `pi`, `e`, `i`.  rc's `^ * ( ) #` need quotes: `calc '2^100'`.  With no expression, each line of its input |
| `forth`, `hylang`, `basic`, `rc` | The languages and the shell: [HyForth](06-languages/hyforth.md), [hylang](06-languages/hylang.md), [HyBASIC](06-languages/hybasic.md), [rc](05-rc.md) |

`play` is [sound](04-drivers/sound.md#play-songs-scores-and-wav-files)'s.
