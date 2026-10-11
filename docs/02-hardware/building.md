# Building it

How to go from a bare board to a Hydra-16 at a shell prompt: the tools, the order to solder in, the checks before
power, programming the ROMs, the first boot, what POST tells you, the hardware test, and what to do when something's
wrong.

## Contents

1. [What you need](#what-you-need)
2. [Assembly](#assembly)
3. [Before the chips go in](#before-the-chips-go-in)
4. [Programming the ROMs](#programming-the-roms)
5. [Fitting the chips and jumpers](#fitting-the-chips-and-jumpers)
6. [The first boot](#the-first-boot)
7. [POST: the power-on self test](#post-the-power-on-self-test)
8. [The hardware test](#the-hardware-test)
9. [When something's wrong](#when-somethings-wrong)

---

## What you need

* **The parts**: [the parts list](parts.md), and the board (`board/hydra-16.kicad_pcb`, sent to any PCB maker; it's a
  six-layer board, 304.8 x 243.8 mm, with ground, +5 V and ±12 V planes inside).
* **A soldering iron** with a fine tip, solder, flux, and desoldering braid.  Everything is through-hole except the four
  shared RAM chips, which are SOP-32 (1.27 mm pitch): easy with flux and a fine tip.
* **A multimeter.**  A logic probe or an oscilloscope helps a lot if something doesn't work.
* **A flash programmer** that handles the SST39SF040 (DIP-32) and the SST39SF010A (PLCC-32: with a PLCC-32 adapter),
  such as the XGecu T48 or TL866II+.
* **An ATX power supply**, and a switch, a push button and an LED for the front panel (J10), or a PC case's.
* **A terminal**: a PC with a serial port or a USB serial adapter, a **straight-through** DE-9 cable (female to female),
  and a terminal program (PuTTY, Tera Term, `screen`, `minicom`).
* **At least one memory card** ([the cards](cards.md#the-memory-daughter-card)): the system runs without one, but
  HydraOS needs task banks for most programs.
* **An SD card module** for J18 (one with its own 3.3 V regulator and level shifter), for HydraOS's file system.
* **The Vera X**, for a screen, keyboard and mouse: optional; the serial console is all you need.

---

## Assembly

Solder from the flattest parts up, so the board lies flat on the bench while you work.  **Don't put any chips in their
sockets yet.**

1. **The shared RAM** (U25, U27, U28, U29, SOP-32), first, while nothing else is in the way.  Mind pin 1 (the dot or
   bevel on the chip, the marked pad on the board).  Tack two opposite corners, check the alignment, then solder the
   rest; clean any bridges with braid and flux.  Check each pin under a magnifier.
2. **The resistors**: R1-R33 as the list gives them, and R29, a 0 ohm link (a piece of wire does).
3. **The IC sockets**: all the DIP sockets, and the PLCC-32 socket for U6.  Line each socket's notch up with the notch
   on the silkscreen: the socket is your guide when the chips go in.  Solder two opposite corners first, check it sits
   flat, then the rest.
4. **The small capacitors**: the 100 nF decoupling capacitors (104) at every chip, C47 and C48 (33 pF, at the crystal),
   C50 and C52 (3.3 nF), C49 (4.7 µF), C4 and C7-C10 (1 µF, at the MAX232).
5. **The resistor networks** RN1-RN4: pin 1, the common pin, is marked with a dot on the part and a square pad on the
   board.
6. **The crystal** Y1, and **U43**, the reset supervisor (TO-92: match the flat side to the silkscreen).
7. **The electrolytics**: C51 and C53-C56 (100 µF), C86-C89 (47 µF), C90 and C91 (10 µF): the long lead, +, to the
   square pad.  C92 and C93 (220 µF) are non-polar and go either way.
8. **The headers**: the clock jumpers J5-J8, J9, J10, J27, J29, J30, and the SPI headers J18-J25 (sockets).
9. **The connectors**: the memory card sockets J1 and J2, the DE-9 (J3), the audio jack (J26), the ATX connector (J11),
   and last the six slots (J12-J17), which stand tallest.  Solder one pin of each slot, check it's square to the board,
   then the rest.

Then look over the whole board for unsoldered pins, bridges and splashes, and clean off the flux.

---

## Before the chips go in

With **no chips** in their sockets and **no jumpers**:

1. **No shorts.**  Measure the resistance from +5 V to GND (a socket's VCC pin to its GND pin, or J10 pin 8 to pin 5):
   it should be well above a few ohms (the capacitors charge, so it rises).  Do the same for +12 V and -12 V (slot
   pins 17 and 13) to GND.  A dead short means a bridge: find it before power.
2. **The power.**  Plug in the ATX supply and turn it on by connecting J10 pin 11 to ground (the front panel's switch).
   Measure:
   * +5 V at every socket's VCC pin (pin 14 of a DIP-14, 16 of a DIP-16, 20 of a DIP-20, 32 of the RAMs and ROMs, 40
     of the CPU and VIA, and so on).  4.75 to 5.25 V.
   * +12 V and -12 V at IC9 (pins 8 and 4) and U42 (pins 4 and 11).
   * The power LED on J10 pin 12 lights.
3. Turn it off.

---

## Programming the ROMs

The ROM images are already built, in the repository:

| System | BIOS ROM (U6, SST39SF010A) | Paged ROM (SST39SF040) |
| :----- | :------------------------- | :--------------------- |
| **HydraOS** (`reborn/bin/`) | `bios.bin` | `prom0.bin` in **U31**, `prom1.bin` in **U32**, `prom2.bin` in **U34**, `prom3.bin` in **U36** |
| **The base alone** (`base/bin/`) | `bios.bin` | `prom0.bin` in **U31** |

* Burn each file whole, from offset 0, and verify it.  Each is exactly one chip (128K or 512K).
* The other paged ROM sockets stay empty.
* Mark each chip with its file and socket before it leaves the programmer: the four paged ROMs look alike.
* The images are built for **3.58 MHz** (J7) and a **Rockwell R65C51**.  For anything else, build them again
  (it needs cc65; [the emulator and the PC tools](../08-emulator.md) say how):
  * `node build.js --clock 2` for 7.16 MHz (J8; the YM2151 can't be used at that speed);
  * `node build.js --acia wdc` for a WDC W65C51N in U3.
* The paged ROM's images already allow for the board's swapped bank halves and crossed bank bits: burn them as they
  are.
* The BIOS ROM and the paged ROM must come from the **same build**: the modules are built against that kernel's calls,
  and the hardware test checks every chip against the checksums the build stored.

**The PLCC socket.**  U6's chip has one bevelled corner and a dot over pin 1; the socket has the same bevelled corner.
Press the chip in evenly.  To take it out, use a PLCC extractor (or two thin hooks in the socket's corner slots), never
a screwdriver.

---

## Fitting the chips and jumpers

With the power off:

1. **Every chip** in its socket, pin 1 to the notch: the CPU (U1), the VIA and ACIA (U2, U3), all the logic, U7 (an
   HM628512 or a DS1747), the ROMs, the sound chips.  The board works without the sound chips (U38, U41, U42, IC9) and
   without J28 (the SP720), if you don't have them yet.
2. **The jumpers**: **J7** (3.58 MHz) and **J30 on 1-2** (the serial port's CTS).  Only one of J5-J8, ever.
3. **A memory card** in J1/J2, set to **module 0** ([the cards](cards.md#the-memory-daughter-card)); more cards as
   modules 1, 2, and so on.
4. **An SD card module** on J18 (device 0), with a card, if you have one.
5. **The serial cable** from J3 to the terminal.

**The terminal's settings:** 9600 baud, 8 data bits, no parity, 1 stop bit (9600 8N1), hardware flow control
(RTS/CTS), and VT100 or xterm emulation.

---

## The first boot

Turn the power on.  Within a second the terminal shows the kernel's banner and POST's results.  With HydraOS's ROMs, the
drivers then start, one task each, and the login shell's prompt comes up:

```
Hydra-16: kernel 0.1, ABI 1
POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000 20:0/00/0000
POST ok
RAM modules: 03
task F: cons
task E: storage
task D: kdev
task C: snd
task B: gpio
task A: vid
task 1: init
HydraOS 1.0 for the Hydra-16
init: up in task 01
...
HyForth (Forth 2012), bye to end
/>
```

(`task A: vid` starts and ends at once without a Vera X.)  With the base's ROMs, Woz's monitor comes up instead:

```
Hydra-16: kernel 0.1, ABI 1
POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000
POST ok
RAM modules: 02
task F: ser
task E: kdev
task 1: wozmon

Hydra-16 monitor (Woz's): XXXX examine, XXXX.YYYY a block, XXXX: BB store, XXXXR run;
L instructions, K bytes; Escape the line, Ctrl-C back here.
T1 00>
```

Try it: `ls /rom` lists HydraOS's ROM disk, `ps` the tasks; `FF00.FF0F` in the monitor dumps the VIA's registers.
[Chapter 1's first hour](../01-overview.md#a-first-hour) goes on from here.

If nothing appears, or something's wrong, read on.

---

## POST: the power-on self test

POST runs first at every reset, before anything else: in task 0, with interrupts off, and with its own polled output,
so it works when little else does.  It checks what the task system depends on, then the paged RAM's lines.  All zeros
is good.

**The first line: the task system.**

| Field | Good | Checks |
| :---- | :--- | :----- |
| `ZP:` | `0` | Page zero (`$00FF`) is each task's own.  Tasks 8, 4, 2, 1 and 0 each write their number there, then read it back.  The value is a mask of the `T` lines (bit n = Tn) that don't keep them apart |
| `ST:` | `0` | The same for the stack page (`$0100`) |
| `OS:` | `0` | The same at `$03FF` |
| `HI:` | `0` | The same at `$7FF0`, the top of task RAM |
| `SH:` | `S` | Shared bank `$F0` is **S**hared between tasks 0 and 1 (`X`: it isn't: the bank registers or the shared RAM are at fault) |
| `W:` | `0` | A mask of the `W` lines that don't select their BIOS ROM page (each page has its number at `$FDFF`) |

A bad `T` line (the same bits in `ZP`, `ST`, `OS` and `HI`) is U48 (the `T` latch), U21 (its buffer to the memory
cards) or U7's A15-A18 (pins 31, 2, 30, 1).

**The second line: the paged RAM.**  Each field is a hex mask of **bad** lines (bit n set = line n bad):

* `U:x`: the `U` register's lines U0-U3, tested on shared bank `$F0`.
* `bb:x/dd/aaaa`: bank `bb`, tested at `$8000-$9FFF`:
  * `x`: the bank register's lines 0-3 (writes to bank `bb` XOR 1, 2, 4 and 8 mustn't land in `bb`);
  * `dd`: the data lines D0-D7 (a walking one);
  * `aaaa`: the address lines A0-A12 (`$8000 + 2^n` for each line n).

The banks tested are the first bank of each shared RAM chip (`F0`, `F4`, `F8`, `FC`, with U = 0), then the first bank of
each memory card (`00`, `10`, `20`, ...).  A missing chip shows as bad lines.  The test overwrites paged RAM, which
doesn't matter at reset.

**The third line** is `POST ok`, or `POST found faults:` and what they are: `T lines`, `shared RAM`, `W lines`, `RAM
(left unused)`.  The system starts anyway, as far as it can:

* A bad memory card is treated as not there, and a bad shared RAM chip's banks are never handed out.
* A bad `T` or `W` line is beyond help: tasks or BIOS pages land on each other, and the system may not start.

**Finding the chip and pin.**  The shared RAM chips by bank: `F0` is U25, `F4` is U28, `F8` is U27, `FC` is U29 (the
crossed bank bits swap the middle two: [the quirks](quirks.md#the-crossed-bank-bits)).  The HM628512's pins are in [the
architecture](architecture.md#the-ram-window).  For example:

* `F0:0/00/0001` is A0 (pin 12) on U25;
* `F8:0/04/0000` is D2 (pin 15) on U27;
* `10:0/00/0400` is A10 (pin 23) on memory card 1.

**The line after POST**, `RAM modules: 03`, is how many memory cards were found (in hex) and are good.

For a closer look at any fault, run the hardware test.

---

## The hardware test

A test of as much of the board as software can reach, for bringing a board up or chasing a fault.  It's a program of
its own in paged ROM bank 1, the same in both systems' ROMs.  It takes the machine over: interrupts off, its own polled
serial I/O at 9600 8N1, no calls into the system.  Its memory tests overwrite everything, so it ends with a reset.

**Starting it:**

* **From POST**: hold down `T` as you press reset (or type it while POST runs).  This works even when the system can't
  start: a bad shared RAM chip, an IRQ line held active.
* **From HydraOS**: `hwtest` at the shell.

**The menu.**  Type a test's key to run that test, or:

| Key | Does |
| :-- | :--- |
| `A` | All the tests, quick: every bank's marks, and every byte of a sample of the memory (about 15 seconds) |
| `F` | All the tests, full: every byte of every bank, in every task (about 3 minutes at 3.58 MHz with three memory cards) |
| `L` | All the quick tests, again and again until a key: a count of passes and failures after each (for an intermittent fault) |
| `R` | Reset: the machine starts again, through POST |

Each test prints its name, what it found, then `ok` or `FAIL` and the fault.  A run ends with `hwtest: all passed` or
`hwtest: failed: N`:

```
CPU ................ ok
RAM modules ........ 0 1 2 ok
shared RAM ......... FAIL bank 04 8000 bits 08
CPU clock .......... 3.58 MHz ok
SPI devices ........ SD cards 0 ok
hwtest: failed: 1
```

**The tests**, in the order `A` runs them.  The memory tests write a pattern that depends on the address, so a stuck or
crossed address line shows up as well as a bad data line.  "Bits" are hex masks of the bad bits.

| Key | Test | Checks | A fault shows |
| :-- | :--- | :----- | :------------ |
| `1` | CPU | A sample of the W65C02S's instructions and flags: binary and decimal arithmetic, and the 65C02's own (`STZ`, `TSB`/`TRB`, `BRA`, `PHX`, `RMB`/`SMB`, `BBR`/`BBS`, `(zp)`) | `check N` |
| `2` | T U V W registers | Each stores and reads back all 8 bits | the register and its bad bits |
| `3` | shared RAM | Each of the 256 banks (`U` 0-F, IDs `$F0-$FF`) has its own mark; every byte of the first bank of each chip (full: of every bank) | Address faults: each chip and the lines it doesn't see, with pins, e.g. `U29: A18 (pin 1)`.  The same A13-A16 line on all four chips is the `U` register's line itself (test 2).  `no mark`: a data fault or no chip.  Otherwise a byte fault: `bank UB`, where, the bad bits |
| `4` | RAM bank registers | Each task's `$00` (IC1, IC2): set to each bank and read at once; then all 16 tasks set to their own banks first and read afterwards; then lines 4-7 | `at once: task F: D>F` (set to `$FD`, it read bank `$FF`'s mark: a cell that doesn't hold its value); `all set, then read: ...` (right at once, wrong later: writing one register disturbs another).  `skipping banks ...` is information: task 0 alone reads those wrong, so the shared RAM is at fault (test 3) |
| `5` | task RAM | Each task's RAM is its own (a mark in each), then every task's page zero, stack page and `$0200-$7FFF` | the task, where, the bad bits; or `task F has task B's mark: U7 A17 (pin 30)` |
| `6` | RAM modules | Which memory cards there are; each bank of each has its mark in each task; every byte of each card's first bank (full: every bank, in every task) | the bank, the task, where, the bad bits |
| `7` | BIOS ROM | A CRC of each 8K page against the build's checksums | `page N`, and `(W line n)` if it reads as another page |
| `8` | paged ROM | A CRC of each 16K bank in use, and the bank lines | `bank NN`, or `bank line n` |
| `I` | interrupts | No IRQ line active with the devices quiet; the vector RAM (all 16 entries, two patterns); `BRK` through entry `V`; the VIA's, the ACIA's and the YM2151's interrupts on their lines, one at a time, then all at once in priority order | `an IRQ line is held active`; `vector entry N wrote .. read ..`; the device and `no IRQ` or the line it came on; `unasked: IRQ on line N` |
| `V` | VIA | Timer 1's latches, the shift register and IER read back; the timers counting and their flags | the part |
| `Y` | sound chip | The YM2151's busy flag (set by a write, then clear), and its timers' flags | `always busy (no chip?)`, `never busy`, `timer A`, ... |
| `K` | CPU clock | The CPU's clock against the YM2151's (`SND_CLK`): shows 0.89, 1.79, 3.58 or 7.16 MHz, which must be what the ROM is built for | the clock and what the ROM expects, or `not a clock the board has` |
| `S` | serial port | DCD and DSR active; the control and command registers; a programmed reset; a character's time at 9600 baud (Rockwell only) | the register, or the character's time in cycles |
| `P` | SPI devices | Information: each of devices 0-7 is sent an SD card's reset (CMD0); the SD cards that answer are shown | |
| `C` | I2C bus | SCL and SDA high when released and each low alone when pulled low; then information: the devices that answer, at `$08-$77` | the lines' levels |
| `X` | slot cards | Information: the slot ports (`1A` = slot 1, select A) that don't read as empty | |
| `H` | hold a task (probe) | Not a test, and not in `A`, `F` or `L`: every task's RAM bank register set to `$F0` + the task, then the task you type (0-F) held in `T`, its bank read over and over until a key.  Meanwhile, measure T0-T3 (IC1-IC4 pins 1, 15, 14, 13) and `RAMB0-7` (IC1 and IC2 pins 5, 7, 9, 11) with a probe | |

The CPU clock and serial tests time things with VIA timer 1, so a bad VIA shows up there too: run the tests in order
when chasing a fault.

---

## When something's wrong

Work from the bottom up: power, then reset and the clock, then the CPU fetching from the BIOS ROM, then the serial
port.  Every step can be checked with a multimeter or a logic probe.

**Nothing on the terminal at all:**

1. **Power**: +5 V at U1 pin 8 (VDD) and pin 21 (VSS, 0 V).
2. **Reset**: U1 pin 40 (RESB) low for a moment at power-up, then high.  If it stays low, U43 or the front panel's
   button (J10 pin 7) is holding it.
3. **The clock**: a square wave at U1 pin 37 (PHI2 in), 3.58 MHz.  None: check J7 is fitted (one jumper only), Y1 and
   U39 (pin 2 oscillates), U40, R22-R27.
4. **RDY** (U1 pin 2) high, and **BE** (pin 36) high.
5. **The CPU running**: SYNC (U1 pin 7) pulses.  If it doesn't, or the address lines don't change, the CPU isn't
   fetching: check the BIOS ROM (U6's orientation, that it's programmed, its CE at pin 22 pulsing), the address and
   data buffers (U58-U60), and the decoding.
6. **The serial port**: the terminal's settings (9600 8N1), the cable (straight-through, not null-modem), **J30 on
   1-2** with a cable that carries pin 7 (or 2-3 for a three-wire cable), the MAX232's capacitors (C4, C7-C10: check
   their polarity, if they're electrolytics) and its ±8 V or so at pins 2 and 6.  Probe U3 pin 10 (TxD) for activity.

**Garbled characters**: the terminal's rate isn't 9600, or the CPU clock isn't the ROM's (J7 for the standard images).
At other rates than 9600, set both ends.

**POST reports faults**: [POST](#post-the-power-on-self-test) says which chip and pin; reseat the chip, check its
socket's solder joints, swap it with a known-good one of the same kind.  An SOP shared RAM chip: reflow its pins.

**It stops after POST**, or a driver doesn't start: run the [hardware test](#the-hardware-test) (`T` at reset), `A` for
all the tests.  An IRQ line held active (test `I`) stops the system as soon as interrupts go on: it's usually a device
that isn't reset or a slot card.  Take the cards out and try again.

**No sound**: the YM2151's test (`Y`) passes but nothing comes out: check U41, U42, IC9 (±12 V at their supply pins),
and the jack's wiring.  `Y` says `always busy`: U38 isn't there or isn't seen (its CS from port 4).

**No SD card** (`ls /sd/0` fails): the hardware test's `P` shows which devices answer.  Check the module's power (+5 V
on its header) and that it has a 3.3 V regulator and level shifter, and that the card is formatted ([storage](../04-drivers/storage.md)
says how to make a card).

**Intermittent faults**: the hardware test's `L` runs the tests until a key, with a count of passes and failures.
Warm the board (a hair dryer) or tap chips while it runs, to find a bad joint.
