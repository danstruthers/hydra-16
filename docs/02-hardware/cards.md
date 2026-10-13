# The cards

The boards that plug into the main board: the memory daughter card (task banks), the bus breakout card (every bus
signal on a header), and the Vera X (video, a second sound chip, a keyboard and a mouse).  Their parts are in [the
parts list](parts.md), and the drawings of the first two in [the board layout](layout.md).

## Contents

1. [The memory daughter card](#the-memory-daughter-card)
2. [The bus breakout card](#the-bus-breakout-card)
3. [The Vera X](#the-vera-x)
   1. [What it is](#what-it-is), [on the Hydra](#on-the-hydra)
   2. [The glue board](#the-glue-board): [the plan](#the-plan), [the breakout card's headers](#the-breakout-cards-headers-as-slot-0-drives-them),
      [the glue logic](#the-glue-logic), [the Vera X's header](#the-vera-xs-header-pin-by-pin), [the data path](#the-data-path),
      [power](#power-grounds-wires), [audio](#audio), [the interrupt](#the-interrupt)
   3. [The keyboard and mouse](#the-keyboard-and-mouse-the-x16s-smc)
   4. [Timing](#timing), [bringing it up](#bringing-it-up), [the carrier card to come](#the-carrier-card-to-come)

---

## The memory daughter card
*`board/MemoryDaughterCard`: a through-hole version and a surface-mount one.*

A **module** of task RAM is 2 MB: four HM628512, which give each of the 16 tasks 16 banks of 8K of its own
([the architecture](architecture.md#the-ram-window)).  The main board takes up to 15 modules, numbered 0-14, as bank
IDs `$00-$EF`: module m is banks `$m0-$mF`.

**The cards:**

* **Through-hole** (`MemDaughterCard`): one module, four DIP-32 chips.
* **Surface mount** (`MemDaughterCard_SurfaceMount`): **two** modules, eight SOP-32 chips (IC1-IC4 the first, IC5-IC8
  the second).

**How a card works:**

* It plugs into the main board's memory connectors, J1 (1 x 32) and J2 (1 x 24): every module sees the same signals,
  and its number decides which bank IDs it answers.
* A 74LS139 (U1) picks one of the four chips by `RAMB_M2-3` (each half of it for one module on the surface-mount card).
* `RAMB_M0-1` go to the chips' A17-A18, `T_M0-3` to A13-A16 (so each task has its own banks), and A0-A12 and the
  data lines come from J2.

**Setting the module number.**  J3 is a 2 x 15 header.  Its odd pins (1, 3, ... 29) are the module selects
`nBRC0`-`nBRC14`; its even pins are all the card's select.  **A shunt across pins 2m+1 and 2m+2 makes the card module
m**: pins 1-2 for module 0, 3-4 for module 1, and so on.  On the surface-mount card, J4 (1 x 15) is the second module's
select, beside J3's odd row: a shunt from J4's pin m+1 to J3's pin 2m+1 makes the second module m.

* Give every module a different number.  Start at 0 and count up.
* A module answers the number it's jumpered as: [the crossed bank bits](quirks.md#the-crossed-bank-bits) are 2 and 3,
  inside a module's sixteen banks, not the module number's.  (Until 2026-10-13 this page said a module jumpered as 4
  answered as 8; it doesn't.)
* POST finds the modules that are there and tests each one: `RAM modules: 03` is three found.  A module with a bad line
  is left unused.

**The main board's J1** (the card's J1 is the same):

| Pins | Signals |
| :--- | :------ |
| 1-3 | A13, A14, A15 (not used by the card) |
| 4 | Not connected |
| 5-8 | `T_M0-3`: the task |
| 9-12 | `RAMB_M0-3`: the bank's low nibble |
| 13, 14 | `RWB_M`, `!RWB_M`: write and read strobes |
| 15-17 | GND |
| 18-32 | `nBRC0_S-nBRC14_S`: the module selects |

**J2:** D0-D7 (pins 1-8), +5 V (9-11), A0-A12 (12-24).

---

## The bus breakout card
*`board/HydraBusBreakoutCard`.*

A slot card with no chips: every bus signal brought out to a header, for a logic analyser, a scope, or wiring a
prototype (the Vera X's glue board is wired to one).

| Header | Signals |
| :----- | :------ |
| J1 | The slot's edge connector |
| J2 | The data bus: D7 at pin 1, down to D0 at pin 8 |
| J3 | The address bus: A15 at pin 1, down to A0 at pin 16 |
| J4 | SPI: A0-A3, ~CS, MISO, MOSI, SCLK |
| J5 | NMIB, `nIO_S`, the slot's port selects and IRQs |
| J6 | The clocks |
| J7 | Bus control: `nBROMD`, BE, RDY, R/W, SYNC, and `DMAB` |
| J8 | Reset |
| J9 | Audio: the slot's pair |
| J10 | I2C: SDA (pin 1), SCL (pin 2) |
| J11 | Power |

**Two of its labels are wrong.**  The card's connector symbol and the main board's slot disagree, and the slot carries
what the main board's says:

| Header | Its label | What the slot really carries | So |
| :----- | :-------- | :--------------------------- | :- |
| **J11 pin 3** | `+5VA` | **-5 V** (slot pin 9, and only with an old ATX supply) | **Never use it as a 5 V supply** |
| **J7 pin 5** | `!RWB` | **`DMAB`** (slot pin 23) | It's not an inverted R/W: pulling it low stops the CPU |

---

## The Vera X

### What it is

The Hydra's video card is Joe Burks's **VERA X 6.1**, a module carrying the Commander X16's **VERA** (Video Embedded
Retro Adapter): an iCE40UP5K FPGA with 128K of video RAM inside it, running the X16 community's gateware (version 47
or later).  It gives:

* **VGA video**, 640 x 480: two layers, each text (with a 256-character font), tiles, or a bitmap at 1, 2, 4 or 8 bits a
  pixel; 128 hardware sprites; a palette of 256 colours from 4,096; scaling, and the gateware's FX (line drawing,
  polygon filling, cache writes).
* **A 16-voice PSG** (each voice square, sawtooth, triangle or noise), and **a PCM FIFO** (8 or 16 bits, mono or
  stereo, up to 48 kHz), mixed to a stereo line output.
* **An SD card's SPI controller**, with its own card slot on the module.

The VERA's own documentation is the X16's: *The Commander X16 Programmer's Reference*, its VERA chapters.

### On the Hydra

| | |
| :-- | :-- |
| **Slot** | Slot 0, always: its port selects, interrupt and audio pair are the ones the software expects |
| **Registers** | The VERA's 32, at slot 0's ports 2 and 3, `$FF20-$FF3F`: `VERA_*` in `base/include/hw.inc` |
| **Interrupt** | Its `IRQ#` on slot 0's IRQ A, **line 2**: the highest priority after the VIA and the ACIA |
| **Audio** | Its line output into slot 0's audio pair, mixed with the YM2151's |
| **Reset** | `RESB` to its `RES#`: a reset makes the FPGA load itself again from its flash, and it doesn't answer until that's done (the driver looks for it for 0.3 s) |
| **Its SD card** | The VERA's own SPI controller, separate from the board's SPI bus: a card there is the storage driver's disk `v`, `/sd/v` |
| **Keyboard and mouse** | The X16's input controller (an ATtiny861 running `x16-smc`) on the I2C bus at `$42`, polled |

The software: `vid` finds the card as the system starts (by its version register), shows the console on the screen,
and serves the card as `/dev/vid`; the sound driver's channels 8-23 are the PSG's voices and `/dev/vid/pcm` the PCM; and
`input` reads the keyboard and mouse ([video](../04-drivers/video.md)).  The emulator has the whole card
(`node sim/run.js --vera --smc`).

**As built** (October 2026): the VERA X module alone, wired to a bus breakout card in slot 0 through the **glue board**
below.  A proper carrier card ([below](#the-carrier-card-to-come)) is still to be made.

---

### The glue board
*`board/vera-x/vera-x.kicad_sch`: a schematic for a protoboard, with no PCB.*

How to connect the VERA X 6.1 (its 2 x 13 header) to the Hydra with a **bus breakout card in slot 0**, two glue chips
on a small protoboard, and wires, with the X16's input controller beside them for a keyboard and mouse.  Its parts are
in [the parts list](parts.md#the-vera-x-glue-board).

#### The plan

* **Slot 0** (the main board's J12).  Its two port selects are ports 2 and 3, `$FF20-$FF3F`, the VERA's 32 registers;
  its IRQ A is line 2; its audio pair goes into the mixer.  In any other slot the addresses, the interrupt and the audio
  would all be wrong.
* **No buffers on the bus side.**  The breakout card is only headers: each slot pin to a header pin, on the system's
  buses themselves.
* **Glue logic.**  The VERA wants `CS#`, `RD#` and `WR#` (active low).  The bus has `R/W` (`RWB`), `PHI2` and an I/O page
  select; the strobes are made from them as the X16 makes them.

#### The breakout card's headers, as slot 0 drives them

Only the pins this wiring uses:

| Header | Pin | Signal | | Header | Pin | Signal |
| :--- | :--- | :--- | --- | :--- | :--- | :--- |
| J2 (data bus) | 1 | D7 | | J3 (address bus) | 9 | A7 |
| | 2 | D6 | | | 10 | A6 |
| | 3 | D5 | | | 11 | A5 |
| | 4 | D4 | | | 12 | A4 |
| | 5 | D3 | | | 13 | A3 |
| | 6 | D2 | | | 14 | A2 |
| | 7 | D1 | | | 15 | A1 |
| | 8 | D0 | | | 16 | A0 |
| J5 (interrupts & I/O) | 2 | `nIO_S`: the `$FFxx` page, low only while PHI2 is high | | J6 (clocks) | 4 | PHI2 |
| | 3 | `nIOB_S`: port 3, `$FF30-$FF3F` | | J7 (bus control) | 4 | RWB (high: a read) |
| | 4 | `nIOA_S`: port 2, `$FF20-$FF2F` | | J8 (reset) | 1 | RESB (low: reset) |
| | 6 | IRQA: IRQ line 2 (a 3.3K pull-up on the board) | | J9 (audio) | 2 | SND_CR: the mixer's right |
| J11 (power) | 4 | +5 V | | | 3 | SND_CL: the mixer's left |
| | 5 | GND | | J10 (I2C) | 1, 2 | SDA, SCL |

#### The glue logic

**U1, 74AHCT138: `CS#`** for `$FF20-$FF3F`.  Enabled by `nIO_S`, it decodes A5-A7: output Y1 is low for A7 A6 A5 =
0 0 1.

| U1 pin | Function | Connect to |
| :--- | :--- | :--- |
| 1 | A0 | J3 pin 11 (A5) |
| 2 | A1 | J3 pin 10 (A6) |
| 3 | A2 | J3 pin 9 (A7) |
| 4 | /E1 | J5 pin 2 (`nIO_S`) |
| 5 | /E2 | GND |
| 6 | E3 | +5 V |
| 8 | GND | GND |
| 14 | /Y1 | VERA pin 13 (`CS#`) |
| 16 | VCC | +5 V, 100 nF to GND |

Leave the other outputs (7, 9-13, 15) unconnected.

**U2, 74ACT00: `RD#` and `WR#`**.  `RD#` = NOT (PHI2 AND RWB): low while PHI2 is high in a read.  `WR#` = NOT (PHI2
AND NOT RWB): low while PHI2 is high in a write.  Gate 2 is the inverter.

| U2 pin | Function | Connect to |
| :--- | :--- | :--- |
| 1 | 1A | J6 pin 4 (PHI2) |
| 2 | 1B | J7 pin 4 (RWB) |
| 3 | 1Y | VERA pin 18 (`RD#`) |
| 4 | 2A | J7 pin 4 (RWB) |
| 5 | 2B | J7 pin 4 (RWB) |
| 6 | 2Y | U2 pin 10 (NOT RWB) |
| 7 | GND | GND |
| 8 | 3Y | VERA pin 15 (`WR#`) |
| 9 | 3A | J6 pin 4 (PHI2) |
| 10 | 3B | U2 pin 6 |
| 11 | 4Y | unconnected |
| 12, 13 | 4A, 4B | GND (an unused CMOS input must never float) |
| 14 | VCC | +5 V, 100 nF to GND |

Use the ACT and AHCT families as listed: their inputs take the TTL-level signals of the board's LS and F parts, and
they're fast, which the write strobe needs ([timing](#timing)).

#### The Vera X's header, pin by pin

Its 2 x 13 header goes in a 2 x 13 socket (J1 on the glue board).  Pin 1 is SCL; odd pins on one row, even on the other.

| VERA pin | Signal | Connect to |
| :--- | :--- | :--- |
| 1 | SCL | unconnected (the VERA doesn't need it) |
| 2 | SDA | unconnected |
| 3 | +5 V | J11 pin 4 (+5 V) |
| 4 | GND | J11 pin 5 (GND) |
| 5 | D7 | the data path (below), to J2 pin 1 |
| 6 | D6 | J2 pin 2 |
| 7 | D5 | J2 pin 3 |
| 8 | D4 | J2 pin 4 |
| 9 | D3 | J2 pin 5 |
| 10 | D2 | J2 pin 6 |
| 11 | D1 | J2 pin 7 |
| 12 | D0 | J2 pin 8 |
| 13 | `CS#` | U1 pin 14 |
| 14 | `RES#` | J8 pin 1 (RESB) |
| 15 | `WR#` | U2 pin 8 |
| 16 | `IRQ#` | J5 pin 6 (IRQA) |
| 17 | A4 | J3 pin 12 |
| 18 | `RD#` | U2 pin 3 |
| 19 | A2 | J3 pin 14 |
| 20 | A3 | J3 pin 13 |
| 21 | A0 | J3 pin 16 |
| 22 | A1 | J3 pin 15 |
| 23, 24 | GND | GND |
| 25 | Audio left | J9 pin 3 (SND_CL) |
| 26 | Audio right | J9 pin 2 (SND_CR) |

So the VERA's registers 0-31 are `$FF20-$FF3F`, `VERA_BASE` in `hw.inc`.

#### The data path

The VERA X takes 5 V inputs but drives 3.3 V highs.  The W65C02S's data inputs are specified high at 0.7 VDD, **3.5 V**,
so a 3.3 V high is 0.2 V short of the data sheet.  The X16 runs this way, since a CMOS input's real threshold is near
2.5 V, so there are two ways to wire the data lines, and the glue board has both:

* **JP1 with 8 shunts** (try this first): the VERA's data pins straight to the bus.  JP1's odd pins are the VERA's D0-D7
  (`VD0`-`VD7`), its even pins the bus's.
* **U3, a 74AHCT245**, fitted with JP1 empty: its TTL inputs take the VERA's 3.3 V, and its outputs give 5 V.  If the
  card is found unreliably or reads come back wrong, fit it.

| U3 pin | Function | Connect to |
| :--- | :--- | :--- |
| 1 | DIR | J7 pin 4 (RWB: high, A to B, the VERA onto the bus) |
| 2-9 | A1-A8 | VERA pins 12, 11, 10, 9, 8, 7, 6, 5 (D0-D7) |
| 10 | GND | GND |
| 11-18 | B8-B1 | J2 pins 1-8 (D7 at pin 11, down to D0 at pin 18) |
| 19 | /OE | U1 pin 14 (`CS#`) |
| 20 | VCC | +5 V, 100 nF to GND |

#### Power, grounds, wires

* **Power**: J11 pin 4 (+5 V) to VERA pin 3 and the chips; J11 pin 5 (GND) to VERA pins 4, 23 and 24 (all three) and
  the chips.  Use a heavier wire for the ground: the whole data bus returns through it.  10 µF (C1) and 100 nF (C2)
  across the VERA's pins 3 and 4, at the socket; 100 nF at each chip (C3-C6).
* **Never J11 pin 3** (-5 V, whatever the breakout card says).
* Keep the bus wires short (15 cm or less) and together; run a ground wire beside PHI2.  Plug and unplug with the power
  off.
* The VERA X's VGA output goes to the monitor.  Its SD card slot is its own (at 3.3 V, as the VERA X's notes say).

#### Audio

The VERA's line outputs go into the mixer's slot 0 inputs: 10K into an inverting summer on ±12 V, beside the YM2151's.
A ground-centred line output connects straight in.  Before wiring them, measure pins 25 and 26 against GND with the card
running: if they sit at a DC level rather than about 0 V, put a 10 µF capacitor in series with each (its + toward the
VERA if the level is positive), or the mixer passes the offset to the jack.

The mixer is unity gain, so the VERA comes out as loud as its line output is, which may be louder than the YM2151.  If
so, a divider in each line evens them (10K in series and 4.7K to GND at the mixer's side: about a third).

#### The interrupt

Slot 0's IRQ A is the VERA's alone, with a 3.3K pull-up on the board, so the VERA's `IRQ#` drives it directly: low for an
interrupt, high (3.3 V) otherwise.  If the line misbehaves, put a 74LS07 gate (open collector) between them: VERA pin 16
to its input, its output to J5 pin 6, so the card only ever pulls the line low.

---

### The keyboard and mouse: the X16's SMC

The input controller is the X16's own: its **SMC**, an **ATtiny861** (U4) with the X16 community's firmware, `x16-smc`
(github.com/X16Community/x16-smc), unchanged.  A PS/2 keyboard and a PS/2 mouse plug into it, and it talks to the Hydra
over the I2C bus at address `$42`, which every slot carries and the breakout card brings out on J10.

**What you need:** an ATtiny861 (DIP-20) programmed with an `x16-smc` release (its HEX file and fuses, as its README
gives them; a TL866-class programmer does it), two 6-pin mini-DIN sockets (J12, J13), a push button (SW1), a 10K
resistor (R1) and a 100 nF capacitor.

**Its pins** are the firmware's default build's (not its `COMMUNITYX16_PINS` one):

| ATtiny861 pin | Port | The firmware's name | To |
| :--- | :--- | :--- | :--- |
| 1 | PB0 | `I2C_SDA_PIN` | J10 pin 1 (SDA) |
| 3 | PB2 | `I2C_SCL_PIN` | J10 pin 2 (SCL) |
| 18 | PA2 | `PS2_KBD_CLK` | The keyboard's socket (J12), pin 5 (clock) |
| 4 | PB3 | `PS2_KBD_DAT` | The keyboard's socket, pin 1 (data) |
| 9 | PB6 | `PS2_MSE_CLK` | The mouse's socket (J13), pin 5 |
| 8 | PB5 | `PS2_MSE_DAT` | The mouse's socket, pin 1 |
| 14 | PA4 | `POWER_BUTTON_PIN` | SW1, to GND |
| 17 | PA3 | `PWR_OK` | +5 V through R1 (10K): "the power is good" |
| 5, 15 | VCC, AVCC | | +5 V, 100 nF to GND at the chip |
| 6, 16 | GND, AGND | | GND |
| 10 | PB7 | (its RESET) | Nothing (the programmer's) |
| 20, 19, 2, 13, 12, 7, 11 | PA0, PA1, PB1, PA5, PA6, PB4, PA7 | `RESB_PIN`, `NMIB_PIN`, `IRQB_PIN`, `PWR_ON`, `ACT_LED`, ... | Nothing: they're the X16's |

The PS/2 sockets' pin 3 is GND and pin 4 +5 V; pins 2 and 6 aren't used.

* **The I2C bus** has its pull-ups on the main board (RN1), so the SMC needs none.  Nothing else on the board answers at
  `$42`.  `IRQB_PIN` stays unconnected: the Hydra polls the SMC, as its interrupt couldn't be quieted from a handler
  ([the quirks](quirks.md#interrupts-xor-7-and-no-masks)).
* **Press SW1 once after switching on.**  The firmware is a power supply's controller first: its power-on, which the
  button starts, is what starts the keyboard and mouse.  Until then it answers on I2C but has no keys.
* **To check it:** `ls /dev/i2c` lists `42` once the SMC is on the bus, and `ps` shows `input` running (with no SMC it
  ends at once).

---

### Timing

* **The VERA latches a write's data as its write strobe ends**, and the 65C02S holds write data only **10 ns** after PHI2
  falls.  So `WR#` must end the write promptly: it's PHI2 through one 74ACT00 gate, rising 3-9 ns after PHI2 falls.
  (`CS#` comes through the board's decoding and ends later: as the write's end, it would miss the data.)
* **Reads**: the 65C02S needs data 10 ns before PHI2 falls.  `CS#` falls some 15-20 ns after PHI2 rises, and the VERA's
  data follows within tens of ns.  PHI2 is high about 140 ns at 3.58 MHz and 70 ns at 7.16 MHz: room at both.
* **No glitches**: `nIO_S` is low only while PHI2 is high, when the address is steady, so `CS#` can't pulse while it
  changes.
* **The slot's own port selects** (`CS#` = J5 pin 4 AND J5 pin 3, one gate of a 74AHCT08, in place of U1) also work at
  3.58 MHz, but they come through the board's 74LS154, 25-35 ns more, which leaves too little of a 7.16 MHz cycle.  U1
  works at both speeds.

### Bringing it up

1. **The glue board alone** (no VERA in its socket), the breakout card in slot 0, the Hydra on: check +5 V and GND at
   the socket's pins 3 and 4.  With a logic probe or a scope, watch `CS#` (U1 pin 14), `RD#` (U2 pin 3) and `WR#` (U2
   pin 8) while something touches `$FF20-$FF3F`: in HyBASIC, `10 POKE 65313,85: X=PEEK(65313): GOTO 10`, `RUN`
   (Ctrl-C stops it).  `CS#` should pulse on every access, `WR#` on the POKE, `RD#` on the PEEK, each only while PHI2
   is high.
2. **The VERA X**, power off, then on: the console comes up on the monitor too (`vid` found the card).  `cat
   /dev/vid/ctl` starts with `vera 47.0.2` (the gateware's version).
3. **Try it**: `echo border 4 >/dev/vid/ctl` (the border's colour), `echo note 8 69 >/dev/sndctl` (an A from the PSG,
   through the mixer; `echo off 8 >/dev/sndctl`), `play` a WAV file (the PCM).
4. **No `/dev/vid`** (no card found): check +5 V, the data lines' order (VERA pin 5 is D7), A0-A4, `CS#`, `RD#` and
   `WR#` (each active low), and `RES#` (high once the reset is over).  Then try U3 for the 3.3 V highs.
5. **The keyboard and mouse**: `ls /dev/i2c` lists `42`; press SW1; then keys typed on the PS/2 keyboard reach the shell,
   and `cat /dev/vid/mouse` prints a line each time the mouse moves, the pointer following it on the screen.
6. **Then 7.16 MHz**, once everything works at 3.58 MHz (J8, and a build with `--clock 2`).

### The carrier card to come

The plan for a proper card (`docs/design/plans/VIDEO.md`, option A) is a small slot card with a socket for the VERA X
module and the same glue: `CS#` from either port select, `RD#` and `WR#` from PHI2 and R/W, `IRQ#` through an
open-collector buffer, the audio through a divider to the mixer's level, and the SMC with its PS/2 sockets on the slot's
I2C.  Until it exists, the glue board above does the same job.
