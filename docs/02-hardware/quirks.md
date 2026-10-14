# The quirks

What the board does that you wouldn't expect.  None of it matters to a program that goes through the system's calls:
the kernel and the build allow for all of it.  It matters when you write your own kernel, trace a fault to a chip, or
design a card.

## Contents

1. [The crossed bank bits](#the-crossed-bank-bits)
2. [The paged ROM's swapped halves](#the-paged-roms-swapped-halves)
3. [Registers that power up random](#registers-that-power-up-random)
4. [Write-only bank registers](#write-only-bank-registers)
5. [Changing W changes the code](#changing-w-changes-the-code)
6. [Interrupts: XOR 7, and no masks](#interrupts-xor-7-and-no-masks)
7. [No wait states](#no-wait-states)
8. [The serial port's clock](#the-serial-ports-clock)
9. [The YM2151's timer B](#the-ym2151s-timer-b)
10. [The clock chip in task F's RAM](#the-clock-chip-in-task-fs-ram)
11. [Only slot 0's audio is mixed](#only-slot-0s-audio-is-mixed)
12. [The power rails](#the-power-rails)
13. [The schematic's own quirks](#the-schematics-own-quirks)
14. [What changed from 1.8C to 1.8D](#what-changed-from-18c-to-18d)

---

## The crossed bank bits

On the board as built, one pair of bank bits trades places between the bank registers and the memory, in **both**
bank registers (`$00` for RAM, `$01` for ROM): **bits 2 and 3**.  Data bit 2 drives `RAMB3`/`ROMB3`, and bit 3 drives
`RAMB2`/`ROMB2` (IC1 and IC3, the registers' low nibbles).  Bits 4 to 7 reach the memory straight: on the drawing
the high nibbles' chips (IC2 and IC4) have data bits 7 and 6 crossed into their inputs *and* their outputs labelled
`RAMB6`/`RAMB7` the other way round, two crossings that cancel.  So a bank number's bit 5 is chip-select bit 0, bit 6
is bit 1 and bit 7 is bit 2, and chip k in the decoder's order holds banks 32k to 32k + 31.  (Bits 6 and 7 were
thought crossed too, until 2026-10-13, when the first ROM to put files in a third chip read an empty socket on
the board: bank `$40` reaches U34, bank `$80` nothing.)

So the bank a program selects as `b` is the hardware's bank `swap(b)`, with bits 2 and 3 exchanged.  It's always the
same memory for the same `b`, so **the software never needs to care**, with two exceptions:

* **Tracing a fault to a chip.**  Shared bank IDs `$F4-$F7` are on **U28**, and `$F8-$FB` on **U27** (not the other way
  round).  IDs whose bits 2 and 3 are equal (`$F0-$F3`, `$FC-$FF`) aren't affected, and a memory card answers the
  module number it's jumpered as (bits 4 to 7 are straight).
* **Building a ROM image.**  Paged ROM bank `b` sits at bank `swap(b)` in the chips: within a chip, banks 4-7 and 8-11
  change places.  The build (`base/tools/romimg.js`) writes the chips' view, and the emulator (`base/sim/lib/machine.js`)
  reads it the same way, so a `promN.bin` is burned as it is, and chip k (U31, U32, U34 ... in the architecture's
  table) holds banks 32k to 32k + 31.

The schematic shows the board as built; don't "fix" it, or every ROM image would have to change.

---

## The paged ROM's swapped halves

The paged ROM's window, `$A000-$DFFF`, is 16K: CPU A13 picks the half.  But in the window, `$A000-$BFFF` has A13 = 1 and
`$C000-$DFFF` has A13 = 0, and A13 goes to the chips unchanged.  So **CPU `$A000` reads the chip's offset `$2000` in the
bank, and CPU `$C000` its offset `$0000`**: each bank's two halves are swapped in the chip.

The build writes each bank's halves swapped, so the CPU sees them in order.  Anything else that writes a paged ROM image
must do the same.

---

## Registers that power up random

`T`, `U`, `V` and `W` are 74F573 latches, and the bank registers and the vector RAM are 74LS219 RAMs: none has a reset.
They power up random, and **pressing reset doesn't change them**.  So:

* The CPU may start with any BIOS page in place.  That's why **every BIOS page starts with the reset code**, and every
  page's RESET vector points to it: it sets `W` to 0 first.
* The boot code sets `T`, `U` and `V`, every task's bank registers, and all 16 vectors, before it turns interrupts on.
* After a reset, RAM keeps what it had (it isn't cleared), which the hardware test and the debugger can use.

---

## Write-only bank registers

The bank registers `$00` and `$01` can't be read: a read of `$0000` or `$0001` comes from task RAM.  But a write goes to
both, so task RAM holds the same value, and a program reads its own bank selections back from there.  Only a DMA card
writing `$0000` or `$0001` could make the two differ.

---

## Changing W changes the code

`W` chooses the BIOS ROM's page at `$E000-$FFFF`, where the code that changes `W` is running.  After `STA W_REGISTER`
the next instruction comes from the new page, at the same address.  So code that changes `W` must be at the same
address, with the same bytes, on every page: the kernel's COMMON block at `$FD00` does all its page changes ([the
kernel](../03-base/kernel.md)).

---

## Interrupts: XOR 7, and no masks

* **The vector RAM's index is the line number XOR 7**: line 0's handler is in entry 7, line 7's in entry 0, line 8's
  in entry 15.  It's how the 74LS148s encode, and `hw.inc`'s `IRQ_INDEX(line)` does the arithmetic.
* **There's no mask register.**  Each line is wired straight to the encoders, so a device that holds its line low keeps
  interrupting until its handler quiets it.  A driver's handler must always clear its device's interrupt, and a device
  that can't be quieted quickly (an I2C device) must be polled instead of given a line.
* **Line 15 has no hardware**, so its entry is free for `BRK`: the kernel sets `V` to it.

---

## No wait states

`RDY` has only a pull-up: nothing on the board stretches a bus cycle.  Every device must answer within the CPU's cycle,
and at 7.16 MHz neither the YM2151 nor the RAM path (the bank registers, the decoders, the RAMs) can: **with the stated
parts, 3.58 MHz is the fastest valid speed** (at J8's 7.16 MHz the power-on self test fails nearly every RAM test:
measured on 1.8D, 2026-10-13).  A card in a slot can pull
RDY low to add wait states of its own, but RDY must never be driven high (the W65C02S pulls it low during `WAI`): use an
open-collector output.

---

## The serial port's clock

The ACIA runs from 1.790 MHz, not the 1.8432 MHz its baud rates are made for, so every rate is **2.9% slow**: 9600 is
about 9,320, 115200 about 111,860.  Terminals accept it.  But at 115200, characters sent back to back have too little
margin, so the serial driver paces its sending by VIA timer 2, with idle bits between characters ([the
devices](devices.md#the-acia-65c51-and-the-serial-port)).

**The WDC W65C51N** has a hardware fault of its own: its "transmitter empty" status bit is always set and it never
interrupts for sending.  The BIOS ROM built for it (`bios_wdc.bin`) times each character instead, and tells the
paged ROM's serial drivers (`BIOS_ACIA`), which do the same.

---

## The YM2151's timer B

On the board, the YM2151's timer B didn't keep its period when it was re-armed every tick (its flag reset through
register `$14`, which seems to restart it): songs timed by it ran up to twice as fast, by a different amount each run.
So the sound driver times songs by the system's tick (VIA timer 1).  The hardware test's timer checks (one period from a
clean start) pass.

---

## The clock chip in task F's RAM

A DS1747 in U7's socket puts its clock's registers at the chip's top 8 bytes, which are task F's `$7FF8-$7FFF`.  So task
F's program must keep out of its own last 8 bytes, and only the kernel's `RTC` call should touch them.  The base and
HydraOS run their serial driver in task F, and keep its RAM below `$7F00`.

---

## Only slot 0's audio is mixed

Every slot has an audio pair (`SND_CL`, `SND_CR`, pins 26 and 24), but on 1.8D only **slot 0's** reaches the mixer, with
the YM2151's and the line input (J29).  1.8C mixed all six slots' audio, each through a 10K resistor; 1.8D keeps only slot
0's, R7 and R13.  A sound card goes in slot 0, or brings its audio out on a connector of its own.

The mixer's inputs are DC-coupled: a card whose outputs sit at a DC level (a DAC biased at half its supply) should
couple them through a capacitor.

---

## The power rails

* **-5 V** reaches the slots (pin 9) only from an old ATX 1.x supply's pin 20; newer supplies leave that pin
  unconnected.  Don't count on it.
* **+3.3 V** comes in on the ATX connector but only reaches a bulk capacitor (C55): it isn't used on the board or
  carried to the slots.
* **+5 V standby** (`+5VA`) likewise only reaches C51.

---

## The schematic's own quirks

Things in the KiCad files that don't match what's on the board, or look wrong but aren't:

* **J28 is a chip**, the SP720 ESD array, drawn with a DIP-16 socket's symbol.
* **The bus breakout card's labels** disagree with the main board's slot on two pins: J11 pin 3, "+5VA", is **-5 V**,
  and J7 pin 5, "!RWB", is **`DMAB`** ([the cards](cards.md#the-bus-breakout-card)).
* **J1 pin 4**, on the memory card connector, isn't connected.
* **The pseudo-registers' unused bits** (`T` and `U` 4-7, `W` 6-7, `V` 4-7) are latched and read back, but drive
  nothing: the software uses `V`'s top bits to remember a software interrupt's number.

---

## What changed from 1.8C to 1.8D

**Reliability changes only.**  The address map, the decoding, every register and every timing a program can see are
1.8C's, so the same ROMs and software run on both boards.  (`board/CHANGES-1.8D.md` has the designer's notes.)

**Parts swapped** (the same footprints and pinouts):

| Part | 1.8C | 1.8D | Why |
| :--- | :--- | :--- | :-- |
| U11 | 74F08 | 74ACT08 | It drives the CPU's clock input; the W65C02S wants a CMOS high (0.7 VDD), and an F part guarantees only about 2.5 V |
| U22 | 74F240 | 74ACT240 | It drives IRQB, PHI1, DMA and !RWB |
| U59 | 74F245 | 74ACT245 | It drives the CPU's data bus on every read |
| IC9 | LF353 | NJM4556A | The mixer drives the headphone jack: the LF353 current-limits into 16-32 ohm headphones and distorts; the 4556A drives about 70 mA |

**Parts added:**

| Part | Value | What |
| :--- | :---- | :--- |
| R22 | 330 ohms | Limits the crystal's drive (between U39's output, the crystal and C48) |
| R23-R27 | 22 ohms | Series terminations at the source of HS_CLK, SND_CLK, SER_CLK, PHI2 and PHI1 |
| R28 | 10K | A fixed pull-up on RDY, in place of 1.8C's jumper J4 |
| C86-C89 | 47 µF | Bulk capacitance on +5 V, spread over the board |
| C80-C85 | 100 nF | Decoupling at the YM2151, the YM3012, and the op-amps' supplies |
| R29 | 0 ohms | Joins the audio ground to the logic ground once, beside the YM3012 |
| C90, C91 | 10 µF | Keep the YM3012's 2.5 V out of the mixer |
| R30, R31 | 33 ohms | In series with each output: the op-amp stable into a cable, safe from a shorted plug |
| C92, C93 | 220 µF non-polar | AC-couple each output to the jack |
| R32, R33 | 10K | Hold the jack's side of C92/C93 at 0 V, so a plug going in doesn't click |
| RN4 | 10K x 8 | Pull-ups on D0-D7: a read nothing answers gives `$FF` |
| J30 | 1 x 3 header | The ACIA's CTS: 1-2 from the RS-232 line (as 1.8C), 2-3 always clear |

**Parts removed:** J4 (the RDY jumper: R28 instead), and the mixer inputs for slots 1-5 ([above](#only-slot-0s-audio-is-mixed)).

**Wiring:**

* The headphone jack (J26) has left on its tip and right on its ring, the usual way.  1.8C had them swapped.
* The power LED (J10 pin 12) is lit from +5 V through R2, not from the ATX supply's PWR_OK (a logic signal, not meant to
  drive an LED).
* ATX pin 20 feeds the slots' -5 V pin.
* U26's unused second half has its inputs tied, and U22's spare inputs are grounded.
* The unused pins are marked no-connect.

**What software sees:** only that a read of an empty slot or an unused I/O port gives `$FF` every time, instead of
whatever the bus last held.  Nothing relied on that.

**At 7.16 MHz**, ACT parts are a few ns slower than F parts; the faster clock fails on 1.8D with the stated parts
(POST's RAM tests): 3.58 MHz is the fastest valid speed.
