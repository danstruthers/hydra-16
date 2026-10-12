# 1. The Hydra-16 at a glance

The Hydra-16 is a computer you can build: a single board around a WDC W65C02S, the modern CMOS version of the 6502 that
ran the Apple II, the Commodore 64 and the BBC Micro.  What makes it different from those machines is that its memory
system was designed for **multitasking**.  The board itself keeps sixteen separate programs' memory apart, so the
operating system can switch from one program to another with a single register write, and a program can't trample
another's memory by accident.

On top of that hardware runs **HydraOS**, an operating system in the style of Plan 9 (Bell Labs' successor to Unix):
every device is a file, every program has its own view of the file tree, and the system is built from small pieces
that each run in a task of their own.  It has a shell, three programming languages, an assembler, a debugger, a
screen editor and a music player, all running on the 65C02.

This chapter explains the ideas the rest of the book is built on, then takes you through an hour with the running
system.  You don't need a board for any of it: the emulator runs the same ROM images the board does.

## Contents

1. [The machine in one page](#the-machine-in-one-page)
2. [The ideas behind it](#the-ideas-behind-it)
3. [The software, in layers](#the-software-in-layers)
4. [The repository](#the-repository)
5. [A first hour](#a-first-hour)
6. [Where to go next](#where-to-go-next)

---

## The machine in one page

| | |
| :-- | :-- |
| **CPU** | WDC W65C02S at 3.58 MHz (a jumper gives 7.16 MHz) |
| **Tasks** | 16, each with its own 32K of RAM (zero page and stack included) and its own bank selections, kept apart by the hardware |
| **RAM** | 512K of task RAM (32K for each task); 2 MB of shared RAM on the board; up to 15 memory cards of 2 MB each, all seen through an 8K window |
| **ROM** | 4 MB of paged ROM in 16K banks (the operating system's modules and its ROM disk); a 128K BIOS ROM in 8K pages (the kernel) |
| **Interrupts** | 16 interrupt lines in priority order, each with its own handler address |
| **On the board** | A 65C22 VIA (timers, GPIO, an I2C bus, an SPI bus with eight device headers), a 65C51 ACIA (an RS-232 serial port), a YM2151 FM sound chip with a stereo mixer, a socket for a DS1747 clock |
| **Expansion** | Six card slots on an 8-bit bus, eight SPI headers (SD cards), a GPIO and I2C header, two memory card connectors |
| **Video** | The Vera X card in slot 0: the Commander X16's VERA chip (VGA at 640x480, two layers, 128 sprites, 256 colours, a 16-voice synthesizer, PCM audio, an SD card slot) |
| **Power** | A standard ATX PC power supply |
| **Software** | HydraOS 1.0: a preemptive multitasking kernel, Plan 9's files and namespaces, a windowing console, HydraFS on SD cards, the rc shell, HyForth, hylang and HyBASIC, an assembler, a debugger, a screen editor, a music player |

---

## The ideas behind it

### A 65C02 in brief

If you've programmed a 6502, skip ahead.  If not, here's what you need to follow this book.

The 65C02 is an 8-bit processor with a 16-bit address bus: it can address 64K of memory, from `$0000` to `$FFFF` (the
`$` means hexadecimal).  It has three 8-bit registers, **A** (the accumulator) and **X** and **Y** (index registers),
an 8-bit stack pointer **S**, a program counter, and a status register **P** whose bits are its flags (carry, zero,
negative, overflow, interrupt disable, decimal).

Two parts of memory are special.  **Page zero**, `$0000-$00FF`, can be reached with shorter, faster instructions, and
pairs of its bytes can be used as pointers; programs keep their most used variables there.  **Page one**,
`$0100-$01FF`, is the stack.  At the top of memory, `$FFFA-$FFFF` hold three addresses the processor reads when it
starts and when it's interrupted: the NMI vector, the RESET vector and the IRQ vector.

Everything else, ROM, RAM and the I/O chips' registers, is somewhere in the same 64K: the 6502 has no separate I/O
instructions, so a device is read and written like memory.

### Sixteen tasks in hardware

On a normal 6502 machine, every program shares one 64K.  If two programs run "at once", they have to agree on who
uses which memory, and they have to share page zero and the stack, which makes switching between them slow and
fragile.

The Hydra-16 gives each of sixteen **tasks** its own lower 32K.  The board has one 512K RAM chip for this, and a
4-bit register called **`T`** (at `$FFF0`) drives that chip's top four address lines.  So when `T` is 3, addresses
`$0000-$7FFF` reach task 3's 32K of the chip; when `T` is 4, the same addresses reach task 4's.  Each task has its own
page zero and its own stack.

```
           T = 0      T = 1      T = 2             T = 15
$7FFF   +---------+---------+---------+   ...   +---------+
        | task 0's| task 1's| task 2's|         |task 15's|
        |  32K    |  32K    |  32K    |         |  32K    |
        |         |         |         |         |         |
$0200   |         |         |         |         |         |
$0100   |  stack  |  stack  |  stack  |         |  stack  |
$0000   |zero page|zero page|zero page|         |zero page|
        +---------+---------+---------+   ...   +---------+
          all of them one 512K chip (U7), T picking the 32K
```

Switching tasks is one write: the kernel saves the running task's registers on its own stack, writes the next task's
number to `T`, and restores that task's registers from its stack, which is now in place.  No memory is copied.

### More memory than 64K: the bank window

32K of RAM per task isn't much.  The upper half of the address space gives more, through windows that show part of a
much larger memory at a time.  The first is the **bank window** at `$8000-$9FFF`: 8K that shows one **bank** of
memory, chosen by the task's **RAM bank register**, a byte written at address `$0000`.

* Bank numbers `$00-$EF` are **task banks**, on memory cards.  Each task has its own: task 3's bank `$05` and task 4's
  bank `$05` are different memory, so tasks stay apart here too.  Each memory card gives each task 16 banks (128K);
  the board takes up to 15 cards.
* Bank numbers `$F0-$FF` are **shared banks**, on the board itself: the same memory for every task, so tasks can share
  data.  There are 256 of them (2 MB), shown 16 at a time; which 16 is chosen by another register, **`U`** (`$FFF1`).

Each task's RAM bank register is its own: when `T` changes, so does which bank is in the window, with no further
writes.  The operating system hands out banks so programs don't have to agree among themselves.

### The paged ROM

The second window, `$A000-$DFFF`, shows 16K of the **paged ROM**: 4 MB of flash in eight chips, 256 banks of 16K.
Each task chooses its own ROM bank with its **ROM bank register**, a byte written at address `$0001`.

This is where the operating system lives.  The drivers, the shell and the languages are **modules** in the paged ROM,
each in banks of its own or, if it's small, sharing a bank with others, and each **runs in place**: a task that runs
the shell sets its ROM bank register to the shell's bank and jumps to it.  Nothing is copied into RAM, so a task keeps
nearly all of its 32K for its data, and sixteen tasks can run the same module at once.  The rest of the paged ROM is a
small read-only disk, `/rom`, with the tools (programs read into RAM to run) and files the system needs.

### The BIOS ROM and its pages

The top 8K, `$E000-$FFFF`, is the **BIOS ROM**, where the kernel lives.  It's paged too: a 128K flash chip, 16 pages of
8K, the page chosen by a fourth register, **`W`** (`$FFF3`).  Changing `W` changes the code being run under the
processor's feet, so the kernel keeps the same small block of code at the same address on every page, and moves
between pages only through it.  The kernel's 192 system calls are a **jump table** on page 0, at `$F800`: a program calls
the system with `JSR` to a call's slot.

### Interrupts with priorities

A 6502 has a single interrupt input, so a normal machine's interrupt handler has to ask each device in turn which one
interrupted.  The Hydra-16 has **sixteen interrupt lines** in priority order (the VIA first, then the serial port, then
the card slots and the sound chip), a pair of priority encoders that find the highest one waiting, and a small RAM of
sixteen handler addresses that the processor reads instead of the ROM's IRQ vector.  So each line goes straight to its
own handler.  The kernel gives each line to the driver that owns it.

### I/O and the card slots

The I/O chips' registers are at `$FF00-$FFEF`: fifteen **ports** of 16 bytes each.  Port 0 is the VIA, port 1 the
serial port, port 4 the sound chip, and the rest belong to the six card slots, two ports and two interrupt lines each.
`$FFF0-$FFF3` are the four registers `T`, `U`, `V` and `W` (`V` chooses which handler address a software interrupt
uses).

### The whole map

```
$FFFF +-------------------------------+
      | $FFFE-$FFFF: the IRQ vector   |  from the vector RAM: the active line's handler
      | $FFFA-$FFFD: NMI, RESET       |  from the BIOS ROM
      | $FFF0-$FFF3: T U V W          |  the pseudo-registers
      | $FF00-$FFEF: I/O ports 0-14   |  the VIA, the ACIA, the YM2151, the slots
$E000 | BIOS ROM, 8K page (W)         |  the kernel
      +-------------------------------+
$DFFF |                               |
      | paged ROM, 16K bank ($01)     |  the system's modules, the ROM disk
$A000 |                               |
      +-------------------------------+
$9FFF | RAM bank, 8K ($00)            |  a task bank, or a shared bank (with U)
$8000 |                               |
      +-------------------------------+
$7FFF |                               |
      | the task's own 32K (T)        |  its program, data, stack and page zero
      |                               |
$0000 +-------------------------------+
```

[Chapter 2](02-hardware/README.md) describes how the board does all this, chip by chip.

---

## The software, in layers

### The base

The **base** is what every system on the Hydra-16 shares, and what the kernel needs to be useful.  It's in the
repository's `base/` folder:

* **The kernel**, the whole BIOS ROM: the boot and the power-on self test (POST), the scheduler, the interrupt path,
  tasks and memory, files and namespaces, and the 192 system calls.  It runs as task 0.
* **`ser`**, the console driver: the serial port as a file.  It runs as task F (15).
* **`kdev`**, the kernel's devices: the list of modules a program is started from, the tasks (`/proc`), pipes, the
  environment.  It runs as task E (14).
* **`wozmon`**, a monitor: Steve Wozniak's Apple 1 monitor, to look at memory, change it and run code.  It runs as
  task 1, the first program, called **init**.

The base boots on its own and gives you the monitor's prompt.  It exists so that someone can write a different
operating system on the same kernel: [chapter 3](03-base/README.md) is the base.

### HydraOS

**HydraOS** is the operating system built on the base, in the repository's `reborn/` folder (it was rebuilt from
scratch in 2026, hence the name).  It uses the same BIOS ROM, byte for byte, and its own paged ROM:

| Task | Module | What it does |
| :--- | :----- | :----------- |
| 0 | the kernel | The scheduler, interrupts, system calls, memory, files and namespaces |
| 1 | `init` | Sets up the RAM disks and the namespace, starts the shell in window 0, and restarts it if it ends |
| 2 and up | programs | The shells, the tools, the languages, your programs |
| A (10) | `vid` | The Vera X video card (it ends at once if there's no card) |
| B (11) | `gpio` | The GPIO header and the I2C bus |
| C (12) | `snd` | Sound: the YM2151, and the Vera X's synthesizer and PCM |
| D (13) | `kdev` | The kernel's devices (the base's) |
| E (14) | `storage` | SD cards, the RAM disks, the ROM disk, and the HydraFS file system |
| F (15) | `cons` | The console: the serial port, windows, the PC tool's `/pc` folder |

`cons` is the base's `ser` with a windowing system on top: they share the same serial-port code.

### The Plan 9 ideas

HydraOS borrows three ideas from Plan 9, and they explain most of how it works.

**Every device is a file.**  The serial console is `/dev/cons`; you read it to get a line typed, and write it to print.
The sound chip is `/dev/snd` and `/dev/sndctl` (`echo note 0 60 >/dev/sndctl` plays middle C).  The tasks are
directories in `/proc`.  A device is a **server**, a driver running in its own task that answers file requests, so the
same few system calls (`OPEN`, `READ`, `WRITE`, `CLOSE`) reach every device.

**Every program has its own namespace.**  The file tree a program sees is built by **binds** and **mounts** ("put this
directory here"), and a program can change its own without affecting anyone else's.  `/bin`, where programs are found,
is a **union** of several directories (the RAM disk's, an SD card's, the ROM's), searched in order, so there are no
search paths.

**Notes, not signals.**  When you press Ctrl-C, the console sends a **note** called `interrupt` to the programs in
that window's **note group**.  A program can catch a note with a handler, or let the default end it.

### Programs and languages

A **program** is either a module in the paged ROM (it runs in place) or a file, usually ending in `.hyx`, that the
system loads into a task's RAM at `$0800` and runs.  You can write programs in assembly (with ca65 on a PC, or with
`as` on the Hydra itself) or in C (with cc65 on a PC).

HydraOS has three languages of its own: **HyForth** (Forth 2012, and the login shell), **hylang** (a Lisp) and
**HyBASIC** (in the QuickBASIC style).  All three, and C, share one number system: exact integers of any size, exact
fractions and decimals, in any base.

---

## The repository

| Folder | What's there |
| :----- | :----------- |
| `docs/` | This book; `docs/design/` the design notes and plans |
| `board/` | The KiCad 9 schematics and PCBs: the main board, the memory daughter card, the bus breakout card, the Vera X glue board |
| `base/` | The base: the kernel (`kernel/`), the system calls' specification (`spec/`), `ser`, `kdev` and `wozmon` (`modules/`), the emulator (`sim/`), its tests, its ROM images (`bin/`) |
| `reborn/` | HydraOS: its modules, its ROM disk (`romfs/`) and SD card (`sdcard/`), the SDKs (`sdk/`), its tests, its ROM images (`bin/`) |
| `old/` | The Hydra's first operating system (the 1.8C software), kept as it was |

The ROM images are kept in Git, so you can program a board without building anything: `base/bin/` has the base's,
`reborn/bin/` HydraOS's.  To build them yourself you need [Node.js](https://nodejs.org) 18 or later and
[cc65](https://cc65.github.io/).

---

## A first hour

This is a tour of the running system.  Everything here works the same on a board; we'll use the emulator.

### Starting it

You need Node.js (18 or later) and the repository.  From the repository's `reborn/` folder:

```
$ node sim/run.js -i
```

That boots HydraOS's images (`reborn/bin/`) in the emulator, in real time, with your terminal as the Hydra's serial
console.  (To build the images yourself first, `node build.js`, which needs cc65 too.)  The boot prints the kernel's
banner, POST's results, each driver starting, and then the shell's prompt:

```
Hydra-16: kernel 0.1, ABI 1
POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000
POST ok
RAM modules: 02
task F: cons
...
HydraOS 1.0 for the Hydra-16
init: up in task 01
...
HyForth (Forth 2012), bye to end
/>
```

The emulator takes keys after **Ctrl-A**: Ctrl-A x quits, Ctrl-A r presses the reset button, Ctrl-A h lists the rest.

If you'd rather not use a terminal, `node sim/web.js` (also from `reborn/`) builds `obj/web/hydra-16.html`: the whole
emulator as one web page.  Open it in a browser, and the serial console is a terminal in the page, with the Vera X's
screen beside it and a Sound button.

### The shell

The prompt, `/>`, is the current directory and `>`.  The login shell is **HyForth working as a shell**: a line that
starts with a Forth word or a number is Forth, and any other line is a command for **rc**, the Plan 9 shell.  So both
work at the same prompt:

```
/> 2 3 + .
5
/> : sq dup * ;
/> 7 sq .
49
/> echo hello | wc
      1       1       6
```

Forth works on a stack: `2 3 + .` pushes 2 and 3, adds them, and prints the result.  `: sq dup * ;` defines a new
word, `sq`, that squares a number.  `echo hello | wc` is an rc command line: `echo` prints its arguments, `|` sends
them to `wc`, which counts the lines, words and characters.

The console edits a line as you type: Backspace, the arrow keys, Home and End, Ctrl-U to clear the line, and Up and
Down for the lines you typed before.  **Ctrl-C** stops whatever's running.

### Files

Everything is in one tree of names:

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
/> ls /rom
README
bin/
doc/
lib/
songs/
```

| Directory | What's there |
| :-------- | :----------- |
| `/rom` | The ROM disk: read only, always there |
| `/ram` | Your own area of the RAM disk (each shell has its own) |
| `/sram` | The shared RAM disk, the same for every program |
| `/sd/0` | The SD card in SPI device 0 (`/sd/1` the card in device 1, and so on) |
| `/bin` | The programs: a union of `/ram/bin`, the cards' `bin`, `/rom/bin` and the ROM's modules |
| `/dev` | The devices: `cons`, `sndctl`, `vid`, `gpio`, `i2c` ... |
| `/proc` | The tasks, a directory each |
| `/env` | The environment variables, a file each |
| `/pc` | A folder on your PC, with the PC tool (or the emulator's `--pc-dir`) |

`cd /rom` goes into a directory and `cd` alone goes back to `/`; `cat /rom/README` prints a file.  Redirection and
pipes are rc's: `echo hi >/ram/f` writes a file, `cat /ram/f | wc -c` counts its characters.

To make a file, use **edit**, the screen editor: `edit /ram/todo`, type some lines, then **Ctrl-O** and Enter to save
and **Ctrl-X** to leave.  The bottom two lines of the screen list the other keys, and Ctrl-G shows them all.

### The SD card

The emulator puts HydraOS's **SD card** in SPI device 0, so it's at `/sd/0`.  It holds what isn't part of the system:
the sample programs, the sample songs and the benchmarks.

```
/> ls /sd/0
bench/
sample/
songs/
/> ls /sd/0/sample/c
chorus
code
ctest
hello
jukebox
keys
ntest
philo
prodcons
race
round
shapes
sketch
tones
upper
/> /sd/0/sample/c/hello world
hello from C, world
```

On a board, write the same card to a real SD card with `node sim/tools/sdwrite.js` (see
[chapter 8](08-emulator.md)).

### Tasks and windows

Every program runs in a task of its own.  `ps` lists them:

```
/> ps
task  state   parent     cpu group  name
   0  ready   -          2.4     0  kernel
   1  wait    -          0.1     1  init
   2  wait     1         0.4     2  forth
   3  event    1         0.0     1  wstart
   4  ready    2         0.0     2  ps
  11  idle    -          0.0    11  gpio
  12  idle    -          0.0    12  snd
  13  idle    -          0.2    13  kdev
  14  idle    -          1.7    14  storage
  15  idle    -          0.9    15  cons
```

`ps` itself is task 4, started by the shell (task 2).  `sleep 100 &` runs a program in the background (`ps` shows its
task, say 5); `kill 5` ends it; `/proc/5/status` is its state as a file.

The console has **windows**, as Plan 9's rio has: each one a whole console with its own shell, one shown at a time on
your terminal.  The windows' keys start with **Ctrl-]**: Ctrl-] c starts a new window with a shell, Ctrl-] and a digit
shows that window, Ctrl-] w lists them, Ctrl-] s splits the screen and starts a shell below, Ctrl-] ? lists all the
keys.  A window you aren't looking at keeps running, and its output is there when you come back.

The multitasking demos show tasks working together, sharing memory and taking turns with semaphores:

```
/> bind -a /sd/0/sample/c /bin
/> race
/> philo
/> round
```

`bind -a` adds the card's samples to the end of `/bin`, so they run by name.  `race` shows several tasks losing each
other's updates to a shared counter, then not losing any once they take turns with a mutex; `philo` is the dining
philosophers; `round` is a four-voice round, each voice a task keeping its own time (turn on the emulator's sound with
`node sim/run.js -i --sound` to hear it).

### The languages

**HyForth** is the shell you're in already.  **hylang** is a Lisp:

```
/> hylang
hylang> (+ 1/3 1/6)
=> 1/2
hylang> (map (fn {n} {* n n}) {1 2 3})
=> {1 4 9}
hylang> exit
```

**HyBASIC** is BASIC in the QuickBASIC style, with numbered lines or none:

```
/> basic
> PRINT 1 / 3 + 1 / 6; 2 ^ 70
 1/2  1180591620717411303424
> 10 FOR i = 1 TO 3: PRINT i; i / 4: NEXT
> RUN
 1  1/4
 2  1/2
 3  3/4
> SYSTEM
```

The numbers are exact: `1/3 + 1/6` is `1/2`, not 0.4999.  [Chapter 6](06-languages/README.md) covers all three.

### A program of your own

The SDKs are in `reborn/sdk`.  To build a sample in a folder of your own (this needs cc65):

```
$ mkdir hi
$ cp sdk/asm/samples/hi/hi.s hi/
$ node build.js prog hi
$ node sim/run.js -i --pc-dir hi
```

`--pc-dir hi` gives the Hydra your folder as `/pc`, as the PC tool does for a real board over the serial line:

```
/> /pc/hi.hyx one two
Hello, one!
Hello, two!
I'm task 5, in /, in window 0.
```

The Hydra can assemble it itself, too: `as /sd/0/sample/as/hi.s /ram/hi` makes the same program.

---

## Where to go next

* **Building a board?**  [Chapter 2](02-hardware/README.md), then [building it](02-hardware/building.md).
* **Writing an operating system of your own?**  [Chapter 3](03-base/README.md), the base.
* **Using the system?**  [The shell](05-rc.md), [the languages](06-languages/README.md) and [the tools](07-tools.md).
* **Writing programs?**  [The programming guide](09-programming/README.md), after [the system calls](03-base/calls.md).
* **Working on a PC?**  [The emulator](08-emulator.md).
