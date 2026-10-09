# First steps with the Hydra-16

A tutorial for a first hour with HydraOS: switching it on, the shell, files and disks, tasks and windows, the
languages, sound, and a program of your own in assembly and in C.  It needs no board: the emulator runs
the same ROM images the board does, and everything here works the same on one.

You need [Node.js](https://nodejs.org) (18 or later) and a clone of the repository; for the programs at the end,
[cc65](https://cc65.github.io/) too (the build looks in `$CC65_BIN`, `$CC65_HOME/bin`, then the PATH).

## 1. Switch it on

From `reborn/` in the repository:

```
node build.js
node sim/run.js -i
```

The first builds the BIOS ROM and the paged ROM's chips (`bin/`); the second boots them in the emulator, its
terminal the Hydra's serial console, in real time.  (On the board, the same images go into the BIOS ROM's socket and
the paged ROM's, and a terminal at 9600 baud is the console.)  The boot shows POST, the drivers starting, and then
the shell's prompt:

```
Hydra-16: kernel 0.1, ABI 1
POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
...
task 1: init
HydraOS 1.0 for the Hydra-16
init: up in task 01
...
HyForth (Forth 2012), bye to end
/>
```

The prompt is the current directory and `>`.  In the emulator, Ctrl-A x quits, Ctrl-A r presses the reset button,
and Ctrl-A h lists the rest.  `node sim/run.js -i --vera --view` gives it a Vera X card too: its screen, which shows
the console as the terminal does, is at http://localhost:8016.

Or with no terminal at all: `node sim/web.js` makes `obj/web/hydra-16.html`, the emulator as one web page (the page,
the emulator and the ROM images in one file).  Open it in a browser: the serial console is a terminal in the page,
with the Vera X's screen beside it, a Sound button, and SD cards kept in the browser (its Setup).

## 2. The shell

The login shell is HyForth over rc: a line whose first word is a Forth word or a number is Forth; any other line is
an rc command line, run by rc.  So both of these work at the same prompt:

```
/> 2 3 + .
5
/> : sq dup * ;
/> 7 sq .
49
/> echo hello | wc
      1       1       6
```

The console edits the line as you type: Backspace, the arrows, Home and End, Ctrl-U, and Up and Down for the lines
before.  Ctrl-C stops what's running.

## 3. Files

Everything is a file in one tree of names.  `ls /` shows its top:

```
/> ls /
bin/
dev/
env/
lib/
mnt/
pc/
proc/
ram/
rom/
sd/
sram/
tmp/
/> cd /rom
/rom> ls
README
bench/
bin/
doc/
lib/
sample/
songs/
/rom> cat README
The Hydra-16's ROM disk
...
```

`/rom` is the ROM disk, read only, there with no card.  `/ram` is your own area of the RAM disk (each shell has
one), `/sram` the shared RAM disk, `/sd/0` the card in SD device 0, `/dev` the devices, `/proc` the tasks.  `cd`
alone goes back to `/`.

Make a file with `edit`, the screen editor: `edit /ram/todo`, then type its lines (Enter after each), Ctrl-O and
Enter to save it, Ctrl-X to leave.  The two lines at the bottom name the other keys (`^K` cut, `^U` paste, `^W`
find ...), and Ctrl-G shows them all ([using/tools.md](using/tools.md#the-screen-editor)).  `ed` is the line
editor, ed's way.

```
/> edit /ram/todo
/> cat /ram/todo
buy a card
fix the clock
```

Redirection and pipes are rc's: `echo hi >/ram/f`, `cat /ram/f | wc -c`, `ls /rom/bin | sort -r`.  The tools are
[using/tools.md](using/tools.md).

## 4. A card

In the emulator, a card is an image file.  `sim/tools/hydrafs.js` makes one and puts files on it:

```
node sim/tools/hydrafs.js mkfs card.img 8 MYCARD
node sim/run.js -i --sd card.img
```

The card is `/sd/0`; what's written there is in `card.img`, which `hydrafs.js ls card.img` lists:

```
/> echo a note >/sd/0/note.txt
/> ls -l /sd/0
--rw-rw-rw- f0        7 2000-01-01 00:00 note.txt
```

A card's `bin` and `lib` join `/bin` and `/lib`, so programs and libraries on it run by name.  `df` shows the
disks, `mkfs`, `fsck` and `label` work on them.  The clock starts at 2000-01-01 with no DS1747; `date` shows it,
and `echo 2026-10-06 12:00:00 >/dev/time` sets it.

## 5. Tasks and windows

Each program runs in a task of its own (16 in all).  `ps` lists them; `&` runs one in the background:

```
/> sleep 100 &
/> ps
task  state   parent     cpu group  name
   0  ready   -         10.1     0  kernel
   1  wait    -          0.1     1  init
   2  wait     1         0.3     2  forth
   ...
   5  sleep    4         0.0     4  sleep
   ...
/> kill 5
```

`/proc/5` has a task's state as files: `status`, `args`, `fd` (its open files), `regs`, `mem`, and `ctl`, which
takes `stop`, `start`, `kill`.

The console has windows, as rio has on Plan 9: each is a whole console, kept whole while it isn't shown.  Windows
come in groups, a group a shell session: Ctrl-] c starts one (and shows it), with a shell; a program's own windows
(`echo new >/dev/wctl`) join its window's group.  Ctrl-] and a digit shows that window, Ctrl-] n and Ctrl-] p the
next and previous group, Ctrl-] Tab (or Ctrl-Tab, where the terminal sends it) the group's next window, and Ctrl-] x
hangs up the window shown.  A window that isn't shown runs on; its output is kept, and shown again when you come
back.  `echo $window` says which one you're in, and `cat /dev/wctl` lists them: number, group, size, `*` the one
shown.  `new-window top` runs a program in a window of its own, in this group (it goes when the program ends);
`new-window -g` starts another shell session.  Ctrl-] w lists the windows to choose from (a window's key, or the
arrows and Enter).  The keys can be changed: `echo key prefix ctrl-a >/dev/wctl` makes Ctrl-A the prefix, as
screen's.  `/dev/snarf` is the console's cut buffer, one for every window: `echo date >/dev/snarf`, then Ctrl-] y
in any window types it there.  Ctrl-] [ (or Shift-PgUp) looks back through a window's scrollback: the arrows and
PgUp move, Space marks a line, Enter copies the lines from it to the cursor into `/dev/snarf`, q goes back.
`echo history 128 >/dev/wctl` keeps 128 more lines in that window.  Ctrl-] s (or v) splits the window: a new
shell below it (or beside it), both shown at once; Ctrl-] and an arrow moves between them, Ctrl-] z shows one alone
and back, and `echo layout grid >/dev/wctl` (or `rows`, `columns`, `tabs`) arranges the group's windows.  Ctrl-] ?
lists the keys.

**Tasks working together.**  The C SDK's multitasking demos start four or five copies of themselves, each in a task
of its own, and draw what they do as they do it: memory they share (a shared segment), and semaphores to take turns
and wait for each other.  Put them in `/bin` first, then run them one at a time (each needs the tasks):

```
/> bind -a /sd/0/sample/c /bin
/> race                lost updates on a shared counter, then none with a mutex
/> chorus              four tasks print on one console: tangled, then whole lines, then in turn
/> philo               the dining philosophers (Ctrl-C ends it); philo -d deadlocks, and says so
/> prodcons            producers and consumers through a ring: counting semaphores
/> round               a four-voice round, each voice a task keeping its own time
```

[The C SDK's guide](../sdk/c/README.md#the-multitasking-demos) says what each shows, and their sources are in
`sdk/c/samples`.

## 6. The languages

HyForth is the shell already: definitions, `.s`, `words`, files with `include`, and libraries with `require`
(`require tools.fl`).  [using/hyforth.md](using/hyforth.md) is its guide.

hylang is the lisp, danlang on the Hydra:

```
/> hylang
hylang (danlang on the Hydra-16)
Type 'exit' to Exit

hylang> (+ 1/3 1/6)
=> 1/2
hylang> (map (fn {n} {* n n}) {1 2 3})
=> {1 4 9}
hylang> (ls "/rom")
=> {"README" "bench" "bin" "doc" "lib" "sample" "songs"}
hylang> exit
```

[using/hylang.md](using/hylang.md) is its guide.  BASIC is QuickBASIC's kind, the Hydra's own: a line typed runs at
once, a line with a number first goes into the program, and `RUN` runs it (or `basic prog.bas` a file, where line
numbers are optional and `SUB`s and `FUNCTION`s have variables of their own):

```
/> basic
> PRINT 1 / 3 + 1 / 6; 2 ^ 70; SQR(-4)
 1/2  1180591620717411303424  2i
> 10 FOR i = 1 TO 3: PRINT i; i / 4: NEXT
> RUN
 1  1/4
 2  1/2
 3  3/4
> SYSTEM
```

[using/basic.md](using/basic.md) is its guide.  The numbers are the same in every language, and in C: exact, of any
size, in any base.  `calc` works one out at rc: `calc 2/3 + 0.5` is `7/6`, `calc -b x 255` is `FF`.  And rc itself,
for an rc session: `rc` ([using/rc.md](using/rc.md)).

## 7. Sound

`scom` plays a short song on the YM2151 (in the emulator, hear it with `node sim/run.js -i --sound`, then the
Sound button at http://localhost:8016): `play
/sd/0/songs/test.zsm` plays a ZSM file, the format the Commander X16's tools (and Furnace) export.  `play -m 0 't180 o4
l8 c d e f g'` plays a line of the score language (its notes, lengths and octaves: [using/tools.md](using/tools.md),
"Scores"), and `echo note 0 60 >/dev/sndctl` a note (`echo off 0 >/dev/sndctl` ends it).  With a Vera X, channels
8-23 are its PSG's voices (`echo note 8 69 >/dev/sndctl`), and `play` plays a WAV file on its PCM.

## 8. A program of your own

The SDKs are in `reborn/sdk`: `asm` for assembly and `c` for C.  Copy a sample to a folder of your own and build it:

```
mkdir hi
cp sdk/asm/samples/hi/hi.s hi/
node build.js prog hi
```

That makes `hi/hi.hyx`, a program for the Hydra.  To run it, give the emulator the folder as `/pc` (what the PC tool,
`sim/tools/hydrapc.js`, does for a board over the serial line):

```
node sim/run.js -i --pc-dir hi
```

```
/> /pc/hi.hyx one two
Hello, one!
Hello, two!
I'm task 5, in /, in window 0.
/> cp /pc/hi.hyx /ram/bin/hi
/> hi three
Hello, three!
I'm task 5, in /, in window 0.
```

`/ram/bin` is the first member of `/bin`'s union, so a program there runs by its name.  The Hydra assembles one itself
too: `as /sd/0/sample/as/hi.s /ram/bin/hi` makes the same program (the SDK's include files and three of its samples are in
`/lib/as`; [using/tools.md](using/tools.md#the-assembler)), and `edit` is there for a source of your own.  A C program
is the same: a folder of `.c` files (`cp sdk/c/samples/hello/hello.c myc/`, then `node build.js prog myc`):

```
/> /pc/hello.hyx
hello from C
```

[../sdk/asm/README.md](../sdk/asm/README.md) and [../sdk/c/README.md](../sdk/c/README.md) are the SDKs' guides, and
[programming/README.md](programming/README.md) the programmer's guide: the system calls, memory, tasks and notes,
files and namespaces, and writing a server or a driver.

## Where next

* [hydra-16.md](hydra-16.md): the guide to the whole system, hardware and software, with links to the rest.
* [using/README.md](using/README.md): the guides for rc, the tools, HyForth, hylang and BASIC.
* `/rom/doc/api.md` on the Hydra: every system call, its registers and errors.
* [programming/README.md](programming/README.md): the programmer's guide, for programs of your own.
* [hardware.md](hardware.md): the board, its cards and the Vera X, in full.
* [status.md](status.md): where the system stands, and what each part measured.
