# The tools

The programs in `/bin`.  Most are modules in the paged ROM, which run in place (`/dev/mod/NAME`, bound into `/bin`
by `#m/bin`); a few are files on the ROM disk, read into RAM to run (`/rom/bin`: `mkfs`, `fsck`, `label`, `grep`,
`sort`, `db`, `edit`, `calc`, `scom`).  `/bin` is a union, so a program of your own in `/ram/bin`, `/sram/bin` or a card's `/bin` comes
before the ROM's of the same name.

They behave as Plan 9's do:
* Flags come first (`ls -l`, `-abc` together), then names.  A tool that reads files reads fd 0 when it's given
  none, so each works in a pipeline (`cat f | wc`).
* A name that fails is said on fd 2 as `tool: name: why` (`ls: /none: not found`), the rest go on, and the tool
  ends with code 1.  A bad flag or too few names is `usage: ...`, and the status `usage`.
* `$status` after a tool is its exit message, or its code; empty or `0` is success.

Every tool here runs the same way at rc's prompt and at HyForth's (whose shell gives a line whose first word isn't a
Forth word to rc).  At HyForth's prompt, a tool whose name is also a Forth word needs a `%` before it (`% free`
when `memory.fl`'s `free` is loaded).

Contents: [Files](#files) · [Text](#text) · [The screen editor](#the-screen-editor) · [Tasks](#tasks) ·
[The debugger](#the-debugger) · [The assembler](#the-assembler) · [The system](#the-system) · [Disks](#disks) · [Others](#others)

## Files

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

## Text

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
sort
scom
mkfs
/> grep -n Hydra /rom/README
1:The Hydra-16's ROM disk
5:machine.  It's a HydraFS volume, as a card's is, in the paged ROM's banks after
```

## The screen editor

`edit [file ...]` is the screen editor, nano's way: what you type goes in at the cursor, and the Ctrl keys and the
Meta keys (Esc then a key, or Alt with it: `M-`) are commands.  The two lines at the bottom name the commonest, and
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
the line editor (Text, above), for scripts and a terminal without a screen.

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
again; `step`, `next`, `break` and `nobreak`: the debugger's, below).  Another task's memory and registers are anyone's but the kernel task's and a driver's.

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

## The debugger

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
| `d [addr] [n]` | n instructions (12) from addr (its PC; then on from the last), disassembled |
| `m [addr] [n]` | n bytes (64) from addr, in hex and as text |
| `w addr byte ...` | Bytes (hex) written at addr |
| `l file` | Symbols from an ld65 label file (`ld65 -Ln`: the build makes one for each program, `obj/.../NAME.lbl`) |
| `q` | Quit: a program `db` started is killed; a task it stopped runs on |

```
/> db /rom/sample/hi Ann Bob
task 4
PC=0830 A=30 X=FF Y=48 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0830  A5 02     LDA $02
db> l /pc/hi.lbl
8 symbols
db> b main+14
db> c
breakpoint 1
PC=0844 A=41 X=FF Y=48 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0844  A9 52     LDA #$52         main+14
db> n 4
0846  85 02     STA $02          main+16
0848  A9 08     LDA #$08         main+18
084A  85 03     STA $03          main+1A
PC=084C A=08 X=FF Y=48 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
084C  20 53 F9  JSR $F953        main+1C
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

## The assembler

`as [-bl] file.s [out]` assembles a program for the 65C02 on the Hydra itself, from the same source ca65 takes on a
PC: a RAM program, `out` (`file` without its `.s`, if there's no `out`), that runs as any program does.  `-l`
writes its labels too, `out.lbl` (ld65's `-Ln` form, which `db`'s `l` reads); `-b` writes the bytes alone,
no header, from `.org`'s address (`$0800` if there's none): a ROM's image, say.

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

An error is said as `as: file:line: what`, and `as` ends with status 1; a warning is said, and the program made.  It
reads its source three times (the segments' sizes, then each symbol's value, then the bytes), each file read from
the disk once and kept in the task's RAM banks; a pass that finds errors is the last, so another pass's errors show
once they're mended.  `hi.s`, with `hydra.inc` (some 47K of source), takes about 4 seconds; `upper.s`, with
`toollib.s` too (83K), about 9.

```
/> as -l /lib/as/hi.s /ram/hi
/> /ram/hi Ann
Hello, Ann!
I'm task 4, in /, in window 0.
/> db /ram/hi Ann
...
db> l /ram/hi.lbl
8 symbols
```

## The system

| Tool | What it does |
| :--- | :----------- |
| `date [-n]` | The clock (`2026-10-03 15:04:05`), or `-n` seconds since 2000-01-01.  Set it with `echo 2026-10-03 15:04:05 >/dev/time`, which sets the DS1747 too if there's one |
| `free` | RAM: each task's own banks (16 for each RAM module, 8K each), and the shared RAM: what segments have, what's free |
| `mods` | The paged ROM's modules: bank(s), type (program, driver, library; `boot` a driver started at boot), name |
| `hwtest` | The system starts again into the hardware test (as a T typed during POST does); the reset button ends it |

The kernel's messages (the boot's, POST's, a driver's) are `/dev/kmesg`, its last 4K.

## Disks

The disks are under `/dev/sd`, a directory each: `0`-`f` the SD cards (by their SPI device), `x` the ROM disk, `r`
the RAM disk (each shell's own area of it is its `/ram`), `s` the shared RAM disk (`/sram`), and `v` the Vera X's SD
card (the card on its own SD header, through the VERA's SPI controller: `/sd/v`, a card as `0`-`f` are, and nearly
twice as fast).  Each has `data` (the disk's bytes) and `ctl`, which reads as the disk and its file system:

```
/> cat /dev/sd/r/ctl
ram 256 KB 512 blocks
hydrafs label=RAM
free 252 KB of 255 KB
banks $00-$1f
```

| Tool | What it does |
| :--- | :----------- |
| `df` | Each disk started: its kind, its file system's size and what's free, its label |
| `mkfs [-fp] disk [label ...]` | A new, empty HydraFS on the disk (`0`-`f`, `r`, `s`): `-f` a full format; `-p` in a partition of its own after a card's others |
| `fsck [-f] disk` | The disk's HydraFS checked (`-f`: and its faults fixed): what was lost, unmarked, or used twice |
| `label disk [text ...]` | The disk's label set, or said |

Through the ctl files directly: `start SIZE [FROM-TO]` and `stop` on a RAM disk (`echo start 128K 1-1 >>'#d/r/ctl'`:
from RAM module 1's banks; `>>`, as a stopped disk's directory isn't listed), `init` on a card (after it's changed),
`format`, `label`, `check`, `sync`.  The cards' file systems are at `/sd/N`; a card's `bin` and `lib`, if it has them, join
`/bin` and `/lib`.

A card's writes are kept back a block at a time: the block a file's last write changed stays in the storage driver's
buffer till another block's wanted, the file's closed, or `echo sync >/dev/sd/0/ctl` (any card's `ctl`) writes it,
so a program writing a few bytes at a time isn't slowed by the card.  Close a file (end the program writing it), or
`sync`, before taking its card out or switching off: a block still kept back is lost then (and `init` drops it, as
the card may be another).  The RAM disks' writes go at once.

## Others

| Tool | What it does |
| :--- | :----------- |
| `play [-l] song [n]` | A ZSM song (the X16's format; Furnace exports it) on the YM2151 (and with a Vera X its PSG and PCM parts): once, its loop n more times, or `-l` till Ctrl-C; or a score (a name ending in `.mml`: below); or a WAV file on the Vera X's PCM (8 or 16 bits, mono or stereo, to 48,828 Hz: from a card, 8 bits mono to about 11 kHz).  `/rom/songs` has a few; `scom` plays one |
| `play -o score.mml song.zsm` | The score compiled into a ZSM file instead |
| `play [-lx] -m ch mml ...` | A line of MML (a score's channel's: below) on channel ch, the channel's own instrument if the line names none: `play -m 0 t180 o4 l8 c d e f g` |
| `play [-lx] -c ch notes ...` | A chord: each note on the next channel from ch, the commands before each with it (`play -c 0 o4 l2 I0 c e g`); `-x`, either in the Commander X16's MML (`play -x -m 0 T120 O4 L8 CDEFG`) |
| `xmodem -r file`, `xmodem -s [-k] file` | A file received or sent with XMODEM over the serial line, with any terminal program on the PC: `-r` receives, `-s` sends (`-k`: 1K blocks) |
| `calc [-b base] [-d digits] [expression ...]` | An expression worked out exactly, with the Hydra's numbers (the number libraries', hylang's), its value in decimal or in the base `-b` names (hylang's base strings: `x`, `#x`, `b`, `c`, `16r` ...): `calc 2/3 + 0.5` is `7/6`, `calc sqrt 2` `1.41421356237`, `calc -b x 255` `FF`.  Numbers are read in decimal (`0.5`, `2i`), or in a base by their own prefix (`#xFF`); `+ - * /` (exactly), `%` (an integer's remainder), `^`, parentheses; `sqrt exp log sin cos tan atan` (`-d` digits: 12), `abs floor round truncate fib numerator denominator re im rational random`, `gcd(a, b)`, `pow(a, b)`, `complex(a, b)`, `fixed(a, places)`; `pi`, `e`, `i`.  rc's `^ * ( ) #` need quotes: `calc '2^100'`.  With no expression, each line of its input |
| `forth`, `hylang`, `basic`, `rc` | The languages and the shell: [hyforth.md](hyforth.md), [hylang.md](hylang.md), [basic.md](basic.md), [rc.md](rc.md) |

The sound device is `/dev/snd` (register and value pairs), `/dev/sndctl` (`claim N [P]`, `release N [P]`, `volume
N`, `reset`; and a channel's commands as text: `patch CH P`, `note CH N`, `off CH`, `level CH V`, `pan CH
left|right|both`, `bend CH B`, `drum CH N`, `freq CH HZ`, `glide CH N`, `wave CH pulse|saw|triangle|noise
[WIDTH]`, `sens CH PMS AMS`, `reg R V`; the YM2151's `lfo RATE PMD AMD WAVE` and `noise N|off`; so `echo note 0
60 >/dev/sndctl` plays middle C), `/dev/bell` and `/dev/psg`.  Channels 0-7 are the YM2151's, 8-23 (with a Vera X)
its PSG's voices: the same commands, and `wave`; `claim`'s P is their mask, bit n channel 8 + n.  The
VIA's port A is `/dev/gpio` (pins `0`-`7`, `port`, `ctl`, `ca1`) and `/dev/i2c` the I2C bus on two of its pins;
`/dev/spi` the SPI devices; `/dev/seg` names shared segments; `/dev/vid` is the Vera X (its screen, VRAM, the PSG,
`pcm` and `pcmctl`, its PCM, `mouse` and `mousectl`, its mouse, and `draw`, drawing on its bitmap:
[../programming/video.md](../programming/video.md)); `/pc` a folder on the PC (through
the PC tool, `sim/tools/hydrapc.js`, which is the terminal too, and tells the Hydra its window's size; with
`--win32-input`, in Windows Terminal, Ctrl-Tab reaches the Hydra once Windows Terminal's own binding for it is gone).  A window's size is
`consctl`'s `size` line (`grep size /dev/consctl`): the smaller of the terminals it's shown on.  Its chrome (the
bar, its header and footer: on the screen, by default) shows its title (`echo title >/dev/label`) and status line
(`echo status text >/dev/wctl`); `/lib/windows` has the formats, and `wctl` changes them (`echo chrome serial on
>/dev/wctl` puts them on the serial port's terminal too).

### Scores

A score is music as text, the language of the PC's score compiler (`sim/tools/hysong.js`, whose header has it
whole), and `play` plays one as it is, compiling it as it goes into the same register writes at the same ticks as
`hysong.js` would put in a ZSM file (`play -o` writes that file):

```
; A score: ; to the line's end is a comment
#tempo 120                        ; quarter notes a minute (120); #rate N: its ticks a second (200)
@piano { gm 0 }                   ; an instrument: one of the driver's 163 patches, or a voice's operators
@lead { wave pulse 24 env 2 14 12 18 }   ; a PSG instrument: a waveform, an envelope (in the song's ticks)
A @piano o4 l8 c d e f g4 r4 c2   ; channel 0 (A-H: 0-7, the YM2151's); a channel's lines are joined in order
B @piano o3 l2 c [g e]2 c         ; [ ... ]N: repeated N times
I @lead o5 l8 e a b > c           ; channel 8 (I-X: 8-23, the Vera X's PSG's voices 0-15)
```

Notes `c` to `b` (`+` or `#` sharp, `-` flat), a length (1 a whole note ... 64; 3 6 12 24 48 triplets), dots, `^`
ties; `r` a rest; `x N` a General MIDI drum; `o` `>` `<` the octave; `l` the default length; `q` the part of a note
held (eighths); `v` the volume (0-127); `p l|r|c|0` the speakers; `k` transpose; `D` detune (64ths); `M` and `L`
the LFO; `N` the noise; `y reg,val` a register; `_` a slide to the next note, `&` legato; `I N` the driver's patch
N as the instrument.  The PSG's channels (I-X, with a Vera X) take the same notes, lengths, rests, `o` `l` `q` `v`
`p` `k` `D`, slides and legato, and instruments of their own: `wave W [WIDTH]` (`pulse`, `saw`, `triangle` or
`noise`, or 0-3; a pulse's width 0-63, 63 a square) and `env A D S R`: A ticks rising from silence to the note's
volume, D falling by S (the PSG's 0.5 dB steps, 0-63) to what it holds till its key off, then R falling to silence
(all 0, none: a note on, then off).  `I N` there is waveform N, `y` the PSG's registers (0-63), and `v` the next
note's volume; `x`, `M`, `L` and `N` are the YM2151's alone.  `/rom/songs/vera.mml` uses both chips.  A score is
read whole into memory (some 17K at most); a mistake in it is said with its channel (`play: x.mml: channel 2: no
such drum`).  A line (`-m`) or a chord (`-c`) is the same language, and `t N`
sets its tempo, before its first note; on a PSG channel (`play -m 8 c d e`) a line plays the channel's own waveform,
at its own level, on both speakers unless `p` says, and a chord stays on its first channel's chip.  `-x` takes the
X16's MML (FMPLAY's, and PSGPLAY's on a PSG channel): upper-case notes, `T` the tempo, `V` 0-63 (doubled; the PSG's:
its volume), `P` 1-3 (left, right, both), `S` 0-7 (the gap after a note; `S0` legato), `K` (the next note keyed
on), `I` a patch (the PSG's: its waveform register, 0-255), `O`, `L`, `R`, `<`, `>` as above; each line starts
afresh (T120 O4 L4), not where the last left off.  The languages' words run it: C's `snd_mml` and `snd_chord`, HyForth's and hylang's `snd-mml` and `snd-chord`.
