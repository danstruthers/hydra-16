# First steps with the Hydra-16

A tutorial for a first hour with the rebuilt system: switching it on, the shell, files and disks, tasks and
windows, the two languages, and a program of your own in assembly and in C.  It needs no board: the emulator runs
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
Hydra-16 reborn: kernel 0.1, ABI 1
POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
...
task 1: init
init: up in task 01
...
HyForth (Forth 2012), bye to end
/>
```

The prompt is the current directory and `>`.  In the emulator, Ctrl-A x quits, Ctrl-A r presses the reset button,
and Ctrl-A h lists the rest.

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

Make a file, and edit it with `edit`, the line editor (`a` adds lines till a line of just `.`; `p` prints, `w`
writes, `q` quits):

```
/> edit /ram/todo
/ram/todo: new file
*a
buy a card
fix the clock
.
*p
   1 buy a card
   2 fix the clock
*w
/ram/todo: 25 bytes
*q
/> cat /ram/todo
buy a card
fix the clock
```

Redirection and pipes are rc's: `echo hi >/ram/f`, `cat /ram/f | wc -c`, `ls /rom/bin | sort -r`.  The tools are
[using/tools.md](using/tools.md).

## 4. A card

In the emulator, a card is an image file.  `../sim/tools/hydrafs.js` makes one and puts files on it:

```
node ../sim/tools/hydrafs.js mkfs card.img 8 MYCARD
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

The console has windows, as rio has on Plan 9: each is a whole console with its own shell.  Ctrl-] c makes one (and
shows it), Ctrl-] and a digit shows that window, Ctrl-] n the next.  A window that isn't shown runs on; its output is
kept, and shown again when you come back.  `echo $window` says which one you're in.

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

[using/hylang.md](using/hylang.md) is its guide.  And rc itself, for an rc session: `rc` ([using/rc.md](using/rc.md)).

## 7. Sound

`scom` plays a short song on the YM2151 (in the emulator, its notes are counted, not heard): `play
/rom/songs/test.zsm` plays a ZSM file, the format the Commander X16's tools (and Furnace) export.

## 8. A program of your own

The SDKs are in `reborn/sdk`: `asm` for assembly and `c` for C.  Copy a sample to a folder of your own and build it:

```
mkdir hi
cp sdk/asm/samples/hi/hi.s hi/
node build.js prog hi
```

That makes `hi/hi.hyx`, a program for the Hydra.  To run it, give the emulator the folder as `/pc` (what the PC tool,
`../sim/tools/hydrapc.js`, does for a board over the serial line):

```
node sim/run.js -i --pc-dir hi
```

```
/> /pc/hi.hyx one two
Hello, one!
Hello, two!
I'm task 5, in /.
/> cp /pc/hi.hyx /ram/bin/hi
/> hi three
Hello, three!
I'm task 5, in /.
```

`/ram/bin` is the first member of `/bin`'s union, so a program there runs by its name.  A C program is the same: a
folder of `.c` files (`cp sdk/c/samples/hello/hello.c myc/`, then `node build.js prog myc`):

```
/> /pc/hello.hyx
hello from C
```

[../sdk/asm/README.md](../sdk/asm/README.md) and [../sdk/c/README.md](../sdk/c/README.md) are the SDKs' guides, and
[programming/README.md](programming/README.md) the programmer's guide: the system calls, memory, tasks and notes,
files and namespaces, and writing a server or a driver.

## Where next

* [using/README.md](using/README.md): the guides for rc, the tools, HyForth and hylang.
* `/rom/doc/api.md` on the Hydra: every system call, its registers and errors.
* [status.md](status.md): where the system stands, and what each part measured.
