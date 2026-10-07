# The tools

The programs in `/bin`.  Most are modules in the paged ROM, which run in place (`/dev/mod/NAME`, bound into `/bin`
by `#m/bin`); a few are files on the ROM disk, read into RAM to run (`/rom/bin`: `mkfs`, `fsck`, `label`, `grep`,
`sort`, `scom`).  `/bin` is a union, so a program of your own in `/ram/bin`, `/sram/bin` or a card's `/bin` comes
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

Contents: [Files](#files) · [Text](#text) · [Tasks](#tasks) · [The system](#the-system) · [Disks](#disks) ·
[Others](#others)

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
| `grep [-chilnsv] [-e] pattern [file ...]` | The lines that match a regular expression, Plan 9's (`.`, `[a-z]`, `[^a]`, `^`, `$`, `*`, `+`, `?`, `|`, `( )`): `-c` the count, `-l` the files' names, `-n` with line numbers, `-i` case ignored, `-v` the lines that don't, `-s` no messages, `-h` no file names; status `no matches` when none did |
| `sort [-bfnru] [file ...]` | The lines in order: by bytes; `-n` by the number each starts with; `-f` case folded; `-b` leading blanks ignored; `-r` reversed; `-u` one of each run of equal lines.  Stable; some 20K of lines at most |
| `uniq [-c] [file]` | Each run of the same line once; `-c` with its count |
| `tee [-a] [file ...]` | Fd 0 to fd 1 and to each file (`-a`: added to its end) |
| `xd [file ...]` | Bytes in hex, 16 a line after their offset, then as text: `0000010  68 65 6c 6c 6f 0a    hello.` |
| `more [file ...]` | A screen at a time (22 lines, then `--more--`): Enter for the next, `q` to stop.  Its keys come from the console even when fd 0 is the file |
| `edit [file]` | The line editor, ed's way: `p`, `a`, `i`, `c`, `d`, `w [name]`, `q` (twice if the text's changed), `Q`, `h`, with line numbers (`1,5p`, `$`).  Lines are typed after `a`, `i` or `c` until a line of just `.`; Ctrl-C comes back to its `*` prompt |

```
/> ls /rom/bin | sort -r | head -3
sort
scom
mkfs
/> grep -n Hydra /rom/README
1:The Hydra-16's ROM disk
5:machine.  It's a HydraFS volume, as a card's is, in the paged ROM's banks after
```

## Tasks

| Tool | What it does |
| :--- | :----------- |
| `ps [-a]` | The tasks in use: number, state, parent, CPU time (seconds, to a tenth), note group, name; `-a`: their arguments too.  A stopped task's state is `stopped` |
| `top` | The same each second, with the CPU each took in it, the screen drawn again; Ctrl-C ends it |
| `kill [-i] task ...` | Each task (its number) ended with the kill note, which nothing catches; `-i`: interrupted instead (Ctrl-C's note), which a program may catch |
| `slay [-i] name ...` | Each task running a program of that name, as `kill` |
| `sleep seconds` | Nothing for that long; Ctrl-C ends it |
| `ns [task]` | A task's namespace (none: this one's) as the binds and mounts that make it |

The tasks' own files are under `/proc/N`: `status`, `args`, `cwd`, `env`, `ns`, `fd` (its open files: `0 rw #c 291
#c/cons`), `regs`, `mem` and `ram` (its memory), `note` (write `interrupt`, `kill`, `hangup` or a number to send
it one), and `ctl` (`kill`, `interrupt`, `note N`, `stop` and `start`: a stopped task doesn't run till it's started
again).  Another task's memory and registers are anyone's but the kernel task's and a driver's.

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
the RAM disk (each shell's own area of it is its `/ram`), `s` the shared RAM disk (`/sram`).  Each has `data` (the
disk's bytes) and `ctl`, which reads as the disk and its file system:

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

Through the ctl files directly: `start SIZE [FROM-TO]` and `stop` on a RAM disk (`echo start 128K 1-1
>>'#d/r/ctl'`: from RAM module 1's banks; `>>`, as a stopped disk's directory isn't listed), `init` on a card (after
it's changed), `format`, `label`, `check`.  The cards' file systems are at `/sd/N`; a card's `bin` and `lib`, if it
has them, join `/bin` and `/lib`.

## Others

| Tool | What it does |
| :--- | :----------- |
| `play [-l] song [n]` | A ZSM song (the X16's format; Furnace exports it) on the YM2151: once, its loop n more times, or `-l` till Ctrl-C.  `/rom/songs` has a few; `scom` plays one |
| `xmodem -r file`, `xmodem -s [-k] file` | A file received or sent with XMODEM over the serial line, with any terminal program on the PC: `-r` receives, `-s` sends (`-k`: 1K blocks) |
| `forth`, `hylang`, `rc` | The languages and the shell: [hyforth.md](hyforth.md), [hylang.md](hylang.md), [rc.md](rc.md) |

The sound device is `/dev/snd` (register and value pairs), `/dev/sndctl` (`claim N`, `release N`, `volume N`,
`reset`) and `/dev/bell`; the VIA's port A is `/dev/gpio` (pins `0`-`7`, `port`, `ctl`, `ca1`) and `/dev/i2c` the
I2C bus on two of its pins; `/dev/spi` the SPI devices; `/dev/seg` names shared segments; `/pc` a folder on the PC
(through the PC tool, `../sim/tools/hydrapc.js`, which is the terminal too).
