# The Hydra-16 and HydraOS: the whole system in one place

The Hydra-16 is a multitasking 65C02 computer: a W65C02S whose address decoding gives each of 16 tasks its own RAM, zero
page, stack and bank selections, so a task switch is one register write.  HydraOS is its operating system, Plan 9's way:
one kernel in the BIOS ROM, everything else modules of the paged ROM run in place in tasks of their own, every device a
file server, and each task a namespace of its own.

This guide covers every part of it, hardware and software, with a short account of each and links to the documents that
cover it in full.  Read it top to bottom for the whole picture, or go to a part from the contents.  It describes HydraOS
1.0 and what's come since (October 2026); [status.md](status.md) says where each part stands.  To print:
[hydra-16.pdf](hydra-16.pdf), the guide, the tutorial, the guides, the programmer's guide and the hardware reference as
one book (`node tools/docpdf.js` makes it again).

## Contents

1. [The machine in one page](#1-the-machine-in-one-page)
2. [Getting started](#2-getting-started)
3. [The hardware](#3-the-hardware)
4. [The ROMs](#4-the-roms)
5. [The kernel: tasks, interrupts, memory, notes](#5-the-kernel-tasks-interrupts-memory-notes)
6. [Calling the system](#6-calling-the-system)
7. [Files, devices and namespaces](#7-files-devices-and-namespaces)
8. [The console: windows, the serial port, the screen](#8-the-console-windows-the-serial-port-the-screen)
9. [Storage: disks and HydraFS](#9-storage-disks-and-hydrafs)
10. [The shells](#10-the-shells)
11. [The languages](#11-the-languages)
12. [Programs: assembly and C](#12-programs-assembly-and-c)
13. [Sound](#13-sound)
14. [Video: the Vera X](#14-video-the-vera-x)
15. [The tools](#15-the-tools)
16. [POST, the hardware test and the kernel's messages](#16-post-the-hardware-test-and-the-kernels-messages)
17. [On the PC: the build, the emulator, the tests, the PC tools](#17-on-the-pc-the-build-the-emulator-the-tests-the-pc-tools)
18. [The source tree](#18-the-source-tree)
19. [Status and the design](#19-status-and-the-design)
20. [All the documents](#20-all-the-documents)

---

## 1. The machine in one page

| | |
| :-- | :-- |
| **CPU** | WDC W65C02S, 3.58 MHz as built (7.16 with jumper J8) |
| **Tasks** | 16, each with its own 32K of RAM (`$0000-$7FFF`, zero page and stack included) and its own RAM and ROM bank selections: a task switch is one write to `T` |
| **Memory** | 512K of task RAM; 2 MB of shared RAM and up to 15 task RAM modules of 2 MB (three as built: 48 banks of 8K for each task), through an 8K window at `$8000`; 4 MB of paged ROM in 16K banks at `$A000`; a 128K BIOS ROM in 8K pages at `$E000` |
| **Interrupts** | 16 prioritised IRQ lines, each with its own vector |
| **Devices** | A 65C22 VIA (the tick, SPI, I2C, GPIO), a 65C51 ACIA (the serial console), a YM2151 (stereo FM sound), SD cards on SPI, a DS1747 clock in U7 if one's fitted |
| **Video** | The Vera X in slot 0: the Commander X16's VERA (VGA at 640x480, two layers, 128 sprites, 256 colours, a 16-voice PSG, PCM) |
| **Expansion** | 6 slots (an 8-bit bus), 8 SPI device headers, a GPIO and I2C header |
| **Kernel** | In the BIOS ROM: a preemptive scheduler (200 ticks a second), one interrupt path, calls between tasks, memory and banks, shared segments, semaphores, notes, files' channels, namespaces; one jump table, made from a specification |
| **Modules** | Everything else: the drivers, init, the shells, the tools and the languages, each a module of the paged ROM run in place in a task of its own |
| **Files** | Plan 9's: every device a file server; 16 fds a task; a namespace each, of binds and mounts with union directories; pipes; `/proc`, `/env` |
| **Storage** | HydraFS on SD cards (`/sd/N`), RAM disks (`/ram`, `/sram`), and the ROM disk (`/rom`) |
| **Shells** | HyForth over rc, the login shell; rc, Plan 9's; hylang and BASIC as shells too |
| **Languages** | HyForth (Forth 2012), hylang (danlang, a lisp, with a bytecode machine and native code), BASIC (Microsoft's, by way of EhyBASIC) |
| **Programs** | In assembly (ca65 on a PC, or `as` on the Hydra) or C (cc65): modules of the paged ROM, or programs in files, read into RAM at `$0800` |
| **Tools** | The core tools (Plan 9's way), `edit` (a screen editor), `ed`, `db` (a debugger), `as`, `play` (ZSM songs, scores, WAV files), `xmodem`; on the PC an emulator that runs the real ROMs (with the Vera X and the sound), the regression tests, and the PC tool (`/pc`, and the terminal) |

The tasks as the system starts:

```
 task 0  kernel      the scheduler, IRQs, calls, memory, notes, channels, namespaces (the BIOS ROM's)
 task 1  init        the RAM disks, the namespace, window 0's shell, wstart (a shell for each window made)
 task 2+ programs    the shells, the tools, yours: in place from the paged ROM, or in RAM from $0800
 task A  vid         #v  /dev/vid: the Vera X (with no card it ends as it starts)
 task B  gpio        #g  /dev/gpio, #i  /dev/i2c
 task C  snd         #a  /dev/snd, sndctl, bell, psg
 task D  kdev        the kernel's devices: the root, null, ticks, modules, /proc, pipes, /env, segments
 task E  storage     #S  /dev/spi, #d  /dev/sd, #f  HydraFS: /rom, /ram, /sram, /sd/N
 task F  cons        #c  /dev/cons and the windows, #P  /pc
```

More: [the system in one page](programming/README.md#the-system-in-one-page) (the programmer's guide), [the hardware's
overview](hardware.md#overview).

---

## 2. Getting started

**In the emulator** (no board needed: it runs the same images): from `reborn/`, with Node.js 18 or later and cc65,

```
node build.js            the BIOS ROM and the paged ROM's chips, into bin/
node sim/run.js -i       the Hydra's serial console in your terminal (Ctrl-A x quits, Ctrl-A h helps)
```

The images are in Git too (`bin/`), so `node sim/run.js -i` works without cc65.  [The tutorial](tutorial.md) is the
first hour: the shell, files and disks, windows, the languages, sound, and a program of your own.

**On the board:**
* Program the chips (an EPROM programmer; there's no write path on the board): `bin/bios.bin` into the BIOS ROM's socket
  (U6, an SST39SF010 or bigger), and `bin/prom0.bin`, `prom1.bin`, `prom2.bin`, `prom3.bin` into U31, U32, U34 and U36,
  the first four paged ROM sockets in their chip-select order, each at offset 0 ([the paged
  ROM](hardware.md#the-paged-rom)).
* Jumpers: J4 (RDY) fitted, and one CPU clock: J7, 3.58 MHz ([connectors and
  jumpers](hardware.md#connectors-and-jumpers)).
* A terminal on the DE-9 (J3), 9600 baud, 8 bits, no parity, 1 stop bit, RTS/CTS, through a straight-through cable ([the
  ACIA](hardware.md#acia-65c51-u3-port-1-irq-line-1)).  The PC tool (`sim/tools/hydrapc.js`) is a terminal that also
  serves a folder of the PC as `/pc`.
* An SD card adapter on J18 (SPI device 0), if you have one: the card is `/sd/0`.

The boot prints POST's lines, the drivers starting, and HyForth's prompt, `/>`.

---

## 3. The hardware

The full reference is [hardware.md](hardware.md): the board (V1) as its schematic describes it, every chip, signal,
connector and jumper, and the cards.  In short:

**The memory system.**  `$0000-$7FFF` is the task's own RAM: one 512K chip (U7) with `T` on its top four address lines,
so 16 tasks each have 32K, zero page and stack included.  `$00` and `$01` are each task's bank registers (writes; reads
come from its RAM): `$00` picks the 8K bank at `$8000-$9FFF` (a task RAM module's, 16 banks a task each, or a shared
bank, `$F0-$FF`: 16 of the board's 2 MB, the 128K that `U` picks), and `$01` the 16K bank of the paged ROM at
`$A000-$DFFF` (4 MB in eight SST39SF040s).  `$E000-$FFFF` is the BIOS ROM, its 8K page chosen by `W`.  On the V1 board a
bank number's bits 2 and 3, and 6 and 7, trade places on the way to the chips: software doesn't notice, but the ROM
images are laid out for it.  [The memory map](hardware.md#the-cpu-view-memory-map), [the
pseudo-registers](hardware.md#the-pseudo-registers-t-u-v-w), [task RAM and the bank
registers](hardware.md#task-ram-and-the-bank-registers), [the paged RAM window](hardware.md#the-paged-ram-window), [the
paged ROM](hardware.md#the-paged-rom), [the BIOS ROM](hardware.md#the-bios-rom).

**I/O, interrupts, clocks and reset.**  `$FF00-$FFEF` is 15 ports of 16 bytes (the VIA, the ACIA, the YM2151, and two
for each slot); `$FFF0-$FFF3` are `T`, `U`, `V` and `W`.  Sixteen IRQ lines in priority order (the VIA, the ACIA, slot
0, the YM2151, the other slots), each with its own vector from a vector RAM (its index the line `^ 7`).  A 14.318 MHz
crystal divided down: the CPU clock by a jumper, the YM2151's 3.58 MHz, the ACIA's 1.79 MHz.  Reset resets the CPU and
the chips, not the pseudo-registers, the bank registers or RAM.  [I/O space](hardware.md#io-space),
[interrupts](hardware.md#interrupts), [clocks](hardware.md#clocks), [reset, power and bus
control](hardware.md#reset-power-and-bus-control).

**The devices.**  The VIA: port A on a header (GPIO, the I2C bus on PA0/PA1, CA1 and CA2), port B the SPI bus to eight
device headers (and eight more for cards), timer 1 the system's tick, timer 2 the serial port's pacing.  The ACIA: the
serial console, RS-232 on a DE-9 (its clock 2.9% slow, so 9600 is about 9,320).  The YM2151 and its YM3012 DAC, on a
mixer with each slot's audio and a line input.  [On-board devices](hardware.md#on-board-devices).

**Cards, connectors and the board as built.**  Six slots, 62-pin card edges ([expansion
slots](hardware.md#expansion-slots)); memory daughter cards (a task RAM module each); a bus breakout card for a logic
analyzer; and the Vera X, whose carrier card is still to be made ([companion cards](hardware.md#companion-cards), [the
Vera X](hardware.md#the-vera-x-slot-0)): till then it's wired through the breakout card
([vera-wiring.md](vera-wiring.md)).  The board's quirks are its [V1 errata](hardware.md#v1-errata).

**The schematics** are KiCad 9's, in `../../board/`: the main board's sheets (`AddressDecode`, `BankedROM`, `Buffers`,
`Clocks`, `Connectors`, `FFF_Registers`, `IRQ_P_E`, `Mixer`, `SharedMem`, `Sound`, `ZPMirrorRAM`), the memory daughter
card's and the bus breakout card's.  `include/hw.inc` is the board as the software sees it, its names made from the
reference.

---

## 4. The ROMs

**The BIOS ROM** (`bin/bios.bin`: 16 pages of 8K, the kernel and nothing else).  Every page starts with the reset stub
at `$E000` (`W` isn't reset), has the COMMON block at `$FD00` (the IRQ entry and exit, the kernel's far call between
pages) and its vectors at `$FFFA`.  Page 0 is what runs often (the IRQ path, the scheduler, calls between tasks,
`kcopy`, a task's side of the calls that wait, the console's calls) and the jump table at `$F800`; page 1 the kernel
task's side of the task calls, memory, semaphores, notes and the clock; page 2 files and the environment; page 3
namespaces and the loader (`SPAWN`); page 4 POST and the debugger's steps; the rest is room.  Page 0 is the scarce one.
[The kernel's pages](conventions.md#the-kernels-pages).

**The paged ROM** (`bin/prom0.bin` ...: a 512K image for each chip it fills; four now, some 124 of its 256 banks).  Bank
0 holds the module directory and the ROM disk's partition table; bank 1 the hardware test (the old system's, unchanged);
the modules from bank 2, each at `$A000` of its first bank (about fifty: the drivers, init, the shells, the tools, the
languages; a module may span two to eight banks); then the ROM disk's HydraFS volume (`/rom`: the programs that run from
RAM, the languages' libraries, songs, the SDK's samples and include files, the calls' reference, some 480K).
`modules/rom.txt` lists the modules, `romfs/romfs.txt` the ROM disk's files.  [Modules](programming/modules.md).

---

## 5. The kernel: tasks, interrupts, memory, notes

**Tasks.**  Sixteen, task 0 the kernel's own.  `SPAWN` starts a program by its path through the caller's namespace (a
module of the paged ROM runs in place; a program in a file is read into the new task's RAM at `$0800` by the task
itself); `WAIT` takes a child's exit status (a code and a message: Plan 9's); a task's parent, its note group (the
console's Ctrl-C reaches a group), and `/proc/N` (its state, args, cwd, fds, environment, namespace, registers and
memory as files; its `ctl` stops, starts and steps it).  [Tasks](programming/tasks.md).

**Scheduling and interrupts.**  Preemptive, round robin, 200 ticks a second (the VIA's timer 1); a task may hold the CPU
for a while (`PREEMPT_OFF`), sleep, or wait for an event.  Every interrupt comes through one path in the COMMON block to
its line's owner, a driver; the longest stretch with interrupts off is held under 200 cycles, so 115200 baud keeps up.
[The conventions' interrupts](conventions.md#interrupts).

**Memory.**  A task's 32K: its program and data, a break (`BREAK`) for more; RAM banks (`BANKS_ALLOC`, 8K each through
the window); shared segments of the shared RAM, by name (`/dev/seg`); semaphores (16, counting ones and mutexes).
[Memory](programming/memory.md).  The C SDK's multitasking demos show tasks sharing a segment and taking turns with
semaphores, and draw them as they go: `race` (lost updates, then a mutex), `chorus` (the console shared: a mutex, a
baton), `philo` (the dining philosophers, and a deadlock), `prodcons` (a ring and counting semaphores) and `round` (four
tasks singing a round, each keeping its own time).  [The demos](../sdk/c/README.md#the-multitasking-demos).

**Notes** are Plan 9's signals: by name or number (`interrupt`, `kill`, `hangup` ...), to a task or its group, caught by
a handler or not.  **Calls between tasks**: a server answers in its own task; the kernel copies between tasks (`kcopy`);
a driver's `irq` entry runs in its own task too.  [Reaching other tasks](conventions.md#reaching-other-tasks).

---

## 6. Calling the system

A program calls the system with `jsr` to the call's slot in the jump table (`$F800` up, on BIOS ROM page 0), its
arguments in `.A`, `.X`, `.Y` and the call registers `r0`-`r15` (`$02-$21` of its zero page), its error in carry and
`.A`.  The calls are written down once, in `spec/api.def`, and the build makes from it the jump table, `hydra.inc`, C's
`hydracalls.h`, HyForth's `sys-` words, hylang's `sys-` functions, BASIC's `SYS "NAME"`, and the reference,
`/rom/doc/api.md` on the Hydra.  [Calling the system](programming/calls.md).

---

## 7. Files, devices and namespaces

**Files.**  A task has 16 fds, each naming a channel; `OPEN`, `CREATE`, `READ`, `WRITE`, `SEEK`, `CLOSE`, `STAT`,
`PIPE`, `DUP`, `FD2PATH` ...: the same calls whatever's behind them.  A directory reads as stat records.

**Devices** are file servers: drivers' (`#c` the console, `#d` the disks, `#f` HydraFS, `#S` SPI, `#g` GPIO, `#i` I2C,
`#a` sound, `#v` the Vera X, `#P` `/pc`) and the kernel's own (`#/` the root, `#n` null, zero and the kernel's messages,
`#t` the ticks and the time, `#m` the modules, `#p` `/proc`, `#e` `/env`, `#|` pipes, `#s` segments, `#r` raw RAM).  A
device is controlled by writing commands to its `ctl`.  Every server is built on srvlib.  [Files and the
namespace](programming/files.md), [servers and drivers](programming/servers.md).

**Namespaces.**  Each task's tree of names is built by binds and mounts, with union directories: `/bin` is the RAM
disks', a card's, the ROM disk's and the ROM's own programs together, the first found first, so no search paths.  The
default namespace is `/rom/lib/namespace` (`newns`); a child shares its parent's till one of them changes it.  [The
namespace](programming/files.md#the-namespace), [NAMESPACES.md](design/plans/NAMESPACES.md).

**The environment** is each task's own (8K), served at `/env`: rc's variables, and anyone's.

---

## 8. The console: windows, the serial port, the screen

The console driver (`cons`, task F) serves `#c`: **windows**, rio's way on a serial terminal: each a whole console with
its own shell, shown one at a time (Ctrl-] and a digit shows that one; Ctrl-] c makes a group, a shell session, and
Ctrl-] n and p go between groups, Ctrl-] Tab or Ctrl-Tab between a group's windows, Ctrl-] w lists them, Ctrl-] [
shows the scrollback (Space and Enter copy lines to `/dev/snarf`, the cut buffer, which Ctrl-] y pastes), Ctrl-] s
and v split a window into tiles shown together (`wctl`'s `layout rows`, `columns`, `grid`), a window can float over
the rest in a box (`float`), Ctrl-] ? lists the keys, and `wctl`'s `key` lines change them; `new-window` runs a program in a window of its own), a hidden
one running on, its output kept and shown again.  A read is a line, edited at the console (Backspace, the arrows, Home,
End, Ctrl-U, the lines before); `consctl` turns raw keys on; Ctrl-C (an interrupt) and Ctrl-\ (a kill) are notes to the
shown window's group.  **The serial port** runs at 9600 at boot, and to 115200 (`/dev/serctl`), every byte paced by VIA
timer 2.  **The screen**: with a Vera X, the shown window is on its screen too (`consctl`'s `screen`, `serial`,
`both`), or each terminal is a seat of its own (`seats`: its own window and keys, the keyboard's the screen's); a
click of the mouse focuses a window, and a program that asks (`?1000`) gets the mouse's reports.  [The tools](using/tools.md), [the screen](programming/video.md#the-consoles-terminal).

**Each window keeps its screen** in the console's RAM banks, written by a whole VT100 (the VT100's and VT102's
sequences, their reports, VT52 mode, the alternate screen, double width and height), so a window shown again is
painted exactly as it was; `/dev/text` reads it as text.  **Its size** is the smaller of the terminals it's shown
on, less their chrome (`consctl` reads with `size C R`; a raw reader gets `KEY_RESIZE`; the PC tool tells the
Hydra its window's size), and the line editor wraps at it.  **Its chrome**: the bar (the windows, the time), its
header and its footer, each a row drawn from a format (`wctl`'s `bar`, `header`, `footer`; `/lib/windows` has the
defaults), on the screen by default and on the serial port with `chrome serial on`; its title (`/dev/label`, OSC 2)
and its status line (`status`, or the VT320's) show there.  How they're built:
[WINDOWS.md](design/plans/WINDOWS.md).

---

## 9. Storage: disks and HydraFS

The storage driver (`storage`, task E) owns the SPI bus and the disks, at `/dev/sd`: `0`-`f` the SD cards by their SPI
device (through a cache of their blocks), `x` the ROM disk, `r` the RAM disk (each shell has its own area, `/ram`),
`s` the shared one (`/sram`), and `v` the Vera X's SD card (on the VERA's own SPI controller).  HydraFS is on each, the old system's file system, ported: directories, files to 4 GB, a
card's partitions ([HYDRAFS.md](design/plans/HYDRAFS.md) is its format).  The cards are at `/sd/N`, and a card's `bin`
and `lib` join `/bin` and `/lib`.  `df`, `mkfs`, `fsck` and `label` look after them; the PC's `sim/tools/hydrafs.js`
makes card images.  A card's writes are kept back a block at a time: close the file, or
`echo sync >/dev/sd/N/ctl`, before taking the card out.  [Disks](using/tools.md#disks), [DISKS.md](design/plans/DISKS.md).

---

## 10. The shells

**HyForth over rc** is the login shell (`forth -l`, as `/lib/shell` names it; a card's `/lib/shell` can name another): a
line whose first word is a Forth word or a number is Forth, any other an rc command line.  **rc** is Plan 9's: lists,
quoting, `if`, `for`, `while`, `switch`, functions, redirections, pipelines, background tasks, `$status`, globs, scripts
(`#!/bin/rc`); it's the shell of scripts and `system()`.  **`hylang -l`** and **`basic -l`** are shells too, by the same
rule (a line of the language's, or rc's).  Each window has a shell of its own, its namespace built as it starts, then
its profile.  [rc](using/rc.md), [HyForth](using/hyforth.md), [hylang](using/hylang.md), [BASIC](using/basic.md).

---

## 11. The languages

**HyForth** (`forth`): Forth 2012 (Core, Core Extension, Exception, Facility, File Access, Programming-Tools,
Search-Order, String, Double, Locals, Memory-Allocation, Block), passing the Forth 2012 test suite's tests of them; its
core in the ROM, the other word sets pre-compiled libraries from `/lib/forth`; a `sys-` word for every call; the Hydra's
words (tasks, notes, namespaces, banks, devices, sound); the old HyForth's pieces back.  [The guide](using/hyforth.md),
[its design](hyforth.md), [its status](forth-status.md).

**hylang** (`hylang`): danlang on the Hydra, a lisp with big integers, exact fractions, hashes and closures, and the
Hydra's files, tasks, memory and devices a function away; danlang's own regression suite passes on it, and a bytecode
machine and native code make it 1.9 times HyForth's time over twenty benchmarks.  [The guide](using/hylang.md), [its
design](hylang.md).

**BASIC** (`basic`): Microsoft BASIC 2A by way of EhyBASIC, the Hydra-16's own, as a program: files, sound and `PLAY`,
`SYS` (machine code, or any call by name), a RAM bank, scripts and pipelines, and a shell mode.  [The
guide](using/basic.md), [its design](basic.md).  Planned: a new BASIC for the Hydra, QuickBASIC's kind
([BASIC.md](design/plans/BASIC.md)), on hylang's numbers in every language ([NUMBERS.md](design/plans/NUMBERS.md)).

---

## 12. Programs: assembly and C

Every executable is a HYX2 module: a 48-byte header, then its code and data.  **A program in a file** is linked for
`$0800` and read into its task's RAM as it starts (a card, a RAM disk, `/pc`); **a module of the paged ROM** runs in
place, its data and BSS in its task's RAM, and may span two to eight banks; **a library module** is code other modules
call (`XCALL`).  [Modules and programs](programming/modules.md).

* **Assembly**: the SDK, `sdk/asm` (`hydra.inc`, `hyx2.inc`, `macros.inc`, `toollib`, `srvlib`, `nslib`, samples), with
  ca65 and ld65 on a PC (`node build.js prog DIR`), or **`as` on the Hydra itself**: the same language, the SDK's files
  in `/lib/as`, the same program, byte for byte.  [The assembly SDK](../sdk/asm/README.md), [the
  assembler](using/tools.md#the-assembler).
* **C**: cc65 with the Hydra's library under the standard one (files and stdio, the environment, `system`, `signal` over
  notes, conio, the sound's `snd.h`, the Hydra's own calls in `hydra.h`).  [The C SDK](../sdk/c/README.md).
* **Debugging**: `db` on the Hydra (a program started stopped, stepped, run to breakpoints, with ld65's symbols), and
  the emulator's call traces, breaks and monitor.  [The debugger](using/tools.md#the-debugger).

The samples are on the ROM disk (`/rom/sample`; `bind -a /rom/sample/c /bin` runs the C ones by name), the C SDK's
multitasking demos among them.  [The programmer's guide](programming/README.md) is the way in.

---

## 13. Sound

**The YM2151** (8 FM channels) is the sound driver's (`snd`, task C, `#a`): `/dev/snd` takes register and value pairs,
`/dev/sndctl` claims channels, sets the volume, and takes each channel's command as text
(`echo note 0 60 >/dev/sndctl`), so every language and rc make sound the same way; `/dev/bell` rings.  **With a Vera
X**, channels 8-23 are its PSG's voices (the same commands, and `wave`), and its **PCM** plays samples (`/dev/vid/pcm`,
`pcmctl`).  **`play`** plays the
X16's ZSM songs (their PSG and PCM parts too), scores in the score language (compiled as they play), a line of it or a
chord (`-m`, `-c`; the X16's MML with `-x`), and WAV files.  The languages' words: C's `snd.h`, HyForth's `lib sound`,
hylang's `(use "snd")`, BASIC's `SOUND` and `PLAY`.  In the emulator, `run.js -i --sound` plays it in a browser and
`--wav FILE` records it.  [Sound and scores](using/tools.md#others), [SOUND_PARITY.md](design/plans/SOUND_PARITY.md),
[SOUND.md](design/plans/SOUND.md).

---

## 14. Video: the Vera X

The Vera X's driver (`vid`, task A) finds the card as the system starts, sets it up for the console, and serves it at
`/dev/vid`: `ctl` (modes 80x60, 80x30, 40x30, a bitmap under the text, the cursor, claims), `term` (an ANSI terminal,
where the console writes), `vram`, `pal`, `sprites`, `font`, `frame` (a frame waited for), `psg`, `pcm` and `pcmctl`.  A
program can draw by writing those files, or **claim** the chip and write its registers itself, as an X16 program does.
Its keyboard and mouse are an input controller's, the X16's SMC on the I2C bus: the `input` program types its keys
into the console and gives the mouse to `/dev/vid/mouse` (Plan 9's), a sprite its pointer.  The driver draws on the bitmap (`/dev/vid/draw`: lines, boxes,
circles, text), with the same words in HyForth (`lib video`, a turtle too), hylang, C (`vera.h`, and cc65's TGI).
In the emulator, `--vera`
puts one in slot 0, `--smc` its controller, and `--view` shows its screen in a browser, its keys and mouse the
controller's.  The carrier card comes next.  [The screen](programming/video.md), [the
card](hardware.md#the-vera-x-slot-0), [VIDEO.md](design/plans/VIDEO.md).

---

## 15. The tools

The programs in `/bin` behave as Plan 9's do (flags first, fd 0 when given no names, errors as `tool: name: why`,
`$status`): files (`ls`, `cat`, `cp`, `mv`, `rm`, `mkdir`, `rmdir`, `touch`, `du`, `pwd`, `cmp`), text (`echo`, `wc`,
`head`, `tail`, `grep`, `sort`, `uniq`, `tee`, `xd`, `more`), the editors (`edit`, nano's way, its text in RAM banks;
`ed`, the line editor), tasks (`ps`, `top`, `kill`, `slay`, `sleep`, `ns`), the debugger (`db`), the assembler (`as`),
the system (`date`, `free`, `mods`, `hwtest`), the disks (`df`, `mkfs`, `fsck`, `label`), and others (`play`,
`xmodem`).  `/pc` is a folder of the PC, through the PC tool.  [The tools](using/tools.md).

---

## 16. POST, the hardware test and the kernel's messages

**POST** runs as the system starts: the `T` lines (each task's byte kept apart in its zero page, stack, OS area and
RAM), the shared RAM, the `W` lines (each BIOS ROM page knows its number), the `U` lines, and the first bank of each
shared RAM chip and of each task RAM module installed (its bank, data and address lines):

```
POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000
POST ok
```

A bad line is reported by its number (a bit set), and the memory calls leave a bad module or chip unused.  **The
hardware test** is the old system's, unchanged, in paged ROM bank 1: a `T` typed during POST, or `hwtest` at the shell,
starts it ([its guide](../../old/docs/using/wozmon.md#the-hardware-test)).  **The kernel's messages** (the boot's,
POST's, a driver's) are `/dev/kmesg`, its last 4K.

---

## 17. On the PC: the build, the emulator, the tests, the PC tools

* **The build**, `node build.js` (Node.js and cc65): the BIOS ROM, the paged ROM's chips, the ROM disk, the SDKs
  (`bin/sdk`), and a report of each part's size and room left; `--clock 2` for a 7.16 MHz board, `--acia wdc` for a WDC
  W65C51N; `node build.js prog DIR` for a program of your own.
* **The emulator**, `node sim/run.js`: the board cycle by cycle, running the real images.  `-i` is the serial console
  live (Ctrl-A x quits, r the reset button, b a monitor: steps, registers, memory, breaks, watches); `--sd card.img` a
  card; `--pc-dir DIR` a folder as `/pc`; `--vera` a Vera X (`--vera-sd card.img` a card in its SD slot), `--view` its screen in a browser; `--sound` the sound in a
  browser, `--wav FILE` in a file; `--trace-calls`, `--break`, `--watch` for debugging.  The top of `sim/run.js` lists
  them all; [the hardware reference](hardware.md#in-the-emulator) says what's modelled.
* **The tests**, `node sim/test.js`: 115 of them, each booting its own image and judged on its output, its time budgets
  and its own checks, as many at a time as the PC has cores; `--dl` runs them in the danlang emulator (`sim/dl`), the
  emulator written again in danlang.
* **The PC tools** (`sim/tools`): `hydrapc.js` (the PC tool: the terminal, and `/pc` over the serial line; `npm install`
  in `sim/` for its serial port), `hydrafs.js` (card images), `pcfs.js` (`/pc`'s server), `hysong.js` (scores to ZSM
  songs).

[The README](../README.md) has every command.

---

## 18. The source tree

| Folder | What's there |
| :----- | :----------- |
| `spec/` | The system calls and the error codes: the one source of the jump table, `hydra.inc`, the reference |
| `include/` | `hw.inc` (the board), `layout.inc` (where the kernel's state lives) |
| `kernel/` | The kernel: the BIOS ROM's pages |
| `modules/` | The paged ROM's modules, a folder each, and `rom.txt` |
| `forthlib/` | HyForth's libraries |
| `programs/` | The ROM disk's programs (`/rom/bin`) |
| `romfs/` | The ROM disk's files, and `romfs.txt` |
| `sdk/` | The assembly and C SDKs |
| `tools/` | The build's tools: the specification's outputs, the ROM images, the ROM disk, the budgets, hylang's snapshot |
| `sim/` | The emulator, the tests' runner, the browser view, the sound, the danlang emulator (`dl/`), the PC tools (`tools/`) |
| `tests/` | The tests, their modules and programs, and the Forth, hylang and BASIC suites |
| `bin/` | The ROM images (in Git), and the SDKs the build copies out |
| `docs/` | These documents |

[The tree](../README.md#the-tree) in full.  Beside `reborn/`: `../../board/` (the KiCad files) and `../../old/` (the old
system, frozen).

---

## 19. Status and the design

[status.md](status.md) is where everything stands, step by step against the plan, with what the spikes and budgets
measured and what's next; HyForth's own steps are [forth-status.md](forth-status.md).  [The design](design/README.md) is
how HydraOS came to be: the plan it was built to
([reimplementation-from-scratch.md](design/reimplementation-from-scratch.md)) and the design notes
([design/plans](design/README.md#plans-and-design-notes)).  The rules every source follows are
[conventions.md](conventions.md).

---

## 20. All the documents

| Document | What it covers |
| :------- | :------------- |
| [hydra-16.md](hydra-16.md) | This guide: the whole system in one place |
| [hydra-16.pdf](hydra-16.pdf) | The book to print: this guide, the tutorial, the guides, the programmer's guide and the SDKs', and the hardware reference |
| [tutorial.md](tutorial.md) | The first hour: switching it on, the shell, files and disks, windows, the languages, sound, a program of your own |
| [hardware.md](hardware.md) | The board, the cards and the Vera X, in full |
| [vera-wiring.md](vera-wiring.md) | The Vera X wired to the board through a bus breakout card: the glue logic, pin by pin, and bringing it up |
| [using/](using/README.md) | The guides: [rc](using/rc.md), [the tools](using/tools.md), [HyForth](using/hyforth.md), [hylang](using/hylang.md), [BASIC](using/basic.md) |
| [programming/](programming/README.md) | The programmer's guide: [calls](programming/calls.md), [memory](programming/memory.md), [tasks and notes](programming/tasks.md), [files and namespaces](programming/files.md), [servers and drivers](programming/servers.md), [modules](programming/modules.md), [video](programming/video.md) |
| [../sdk/asm/README.md](../sdk/asm/README.md), [../sdk/c/README.md](../sdk/c/README.md) | The SDKs: building and running programs in assembly and in C |
| [conventions.md](conventions.md) | The rules every source follows |
| [hyforth.md](hyforth.md), [hylang.md](hylang.md), [basic.md](basic.md) | The languages' designs |
| [status.md](status.md), [forth-status.md](forth-status.md) | Where everything stands, and what was measured |
| [design/](design/README.md) | The plan HydraOS was built to, and the design notes |
| [../tests/forth/README.md](../tests/forth/README.md), [../tests/hylang/README.md](../tests/hylang/README.md) | The Forth 2012 test suite, and danlang's |
| [../README.md](../README.md) | Building, running and testing, and the tree |
| `/rom/doc/api.md` (on the Hydra; the build's `obj/gen/api.md`) | Every system call: its registers, errors and words |

The old system's documents are in [../../old/docs](../../old/docs/README.md), frozen with it.
