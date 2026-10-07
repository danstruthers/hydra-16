# Wiring the Vera X to the Hydra-16 through a bus breakout card

How to connect the VERA X 6.1 (Joe Burks's, the 2x13 header) to the Hydra-16 with a **HydraBusBreakoutCard** in
**slot 0**, two glue chips on a small protoboard, and wires; and, for a keyboard and mouse, the X16's input
controller beside them ([below](#the-keyboard-and-mouse-the-x16s-smc)).  Until the carrier card (the plan's
option A: [design/plans/VIDEO.md](design/plans/VIDEO.md); the card in [hardware.md](hardware.md#the-vera-x-slot-0))
exists, this is how the driver (`vid`) meets the real chip.

Everything here comes from the board's own schematics (`board/`, read only): netlists exported with KiCad 9's
`kicad-cli` from `board/HydraBusBreakoutCard/HydraBusBreakoutCard.kicad_sch` and `board/hydra-16.kicad_sch`, the
VERA's gateware (`c:\source\vera-module`, `fpga/source/top.v`), the OtterX's VERA connector and bus decoder (which
hosts the same card), and the W65C02S data sheet.

## What you need

* The VERA X 6.1, a VGA monitor, and its cable.
* A HydraBusBreakoutCard.
* A small protoboard with a **2x13 female header** (0.1") for the VERA X, and:
  * **U1: 74AHCT138** (a 3-to-8 decoder): the VERA's chip select.
  * **U2: 74ACT00** (four 2-input NANDs): its read and write strobes.
  * (Optional) **U3: 74AHCT245** (an 8-bit bus transceiver): full 5 V data levels (below).
  * A 0.1 µF capacitor at each chip, and 10 µF across the VERA's power pins.
* Dupont or wire-wrap wire, kept short (15 cm or less for the bus).

## The plan

* **Slot 0** (the main board's J12).  Its two I/O port selects are ports 2 and 3, `$FF20-$FF3F`, the VERA's 32
  registers; its IRQ A is IRQ line 2, which `vid` owns; its audio pair goes into the mixer's left and right.  In
  any other slot the address, the interrupt line and the mixer channel would all be different.
* **No buffers on the bus side.**  The breakout card is only headers: each slot pin to a header pin.  The slot's data
  lines are on the system data bus itself.
* **Glue logic.**  The VERA wants `CS#`, `RD#` and `WR#` (active low).  The bus has `R/W` (`RWB`) and `PHI2`, and an
  I/O page select; the strobes are made from them as the X16 and the OtterX make them.

## First: two labels on the breakout card are wrong

The breakout card's connector symbol and the main board's slot symbol disagree on two pins.  The main board drives
what its own schematic says:

| Breakout header | Its label | What slot 0 really carries | So |
| :--- | :--- | :--- | :--- |
| **J11 pin 3** | `+5VA` | **-5 V** (slot pin 9) | **Never use it as a 5 V supply** |
| **J7 pin 5** | `!RWB` | **/DMAB** (slot pin 23) | It's not an inverted R/W: don't use it for the strobes |

The other differences between the two symbols are only their point of view (an output of the board is an input of
the card).

## The breakout card's headers, as slot 0 drives them

Only the pins this wiring uses:

| Header | Pin | Signal | | Header | Pin | Signal |
| :--- | :--- | :--- | --- | :--- | :--- | :--- |
| J2 (Data Bus) | 1 | D7 | | J3 (Address Bus) | 9 | A7 |
| | 2 | D6 | | | 10 | A6 |
| | 3 | D5 | | | 11 | A5 |
| | 4 | D4 | | | 12 | A4 |
| | 5 | D3 | | | 13 | A3 |
| | 6 | D2 | | | 14 | A2 |
| | 7 | D1 | | | 15 | A1 |
| | 8 | D0 | | | 16 | A0 |
| J5 (Interrupts & I/O) | 2 | `nIO_S`: the `$FFxx` page, low only while PHI2 is high | | J6 (Clocks) | 4 | PHI2 |
| | 3 | `nIOB_S`: port 3, `$FF30-$FF3F` | | J7 (Bus Control) | 4 | RWB (high: a read) |
| | 4 | `nIOA_S`: port 2, `$FF20-$FF2F` | | J8 (Reset) | 1 | RESB (low: reset) |
| | 6 | IRQA: IRQ line 2 (3.3K pull-up on the board) | | J9 (Audio) | 2 | SND_CR: mixer right |
| J11 (Power) | 4 | +5 V | | | 3 | SND_CL: mixer left |
| | 5 | GND | | | | |

(J3 pin 1 is A15, down to A0 at pin 16.  `nIO_S` is the main board's `$FFxx` decode ORed with PHI1, so it's low
only in the second half of a cycle, while PHI2 is high; the board's 74LS154, U19, splits it into 16-byte ports by
A4-A7.)

## The glue logic

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
| 16 | VCC | +5 V, 0.1 µF to GND |

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
| 12, 13 | 4A, 4B | GND (an unused CMOS input never floats) |
| 14 | VCC | +5 V, 0.1 µF to GND |

Use the ACT/AHCT families as listed: their inputs take the TTL-level signals of the board's LS and F parts, and
they're fast, which the write strobe needs (Timing, below).

## The VERA X's 2x13 header, pin by pin

Pin 1 is SCL; odd pins on one row, even on the other (the OtterX's VERA connector, VIDEO.md's table).

| VERA pin | Signal | Connect to |
| :--- | :--- | :--- |
| 1 | SCL | unconnected (the VERA doesn't need it) |
| 2 | SDA | unconnected |
| 3 | +5 V | J11 pin 4 (+5 V) |
| 4 | GND | J11 pin 5 (GND) |
| 5 | D7 | J2 pin 1 |
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
| 16 | `IRQ#` | J5 pin 6 (IRQA; or through an open-collector buffer, below) |
| 17 | A4 | J3 pin 12 |
| 18 | `RD#` | U2 pin 3 |
| 19 | A2 | J3 pin 14 |
| 20 | A3 | J3 pin 13 |
| 21 | A0 | J3 pin 16 |
| 22 | A1 | J3 pin 15 |
| 23 | GND | GND |
| 24 | GND | GND |
| 25 | Audio left | J9 pin 3 (SND_CL) |
| 26 | Audio right | J9 pin 2 (SND_CR) |

So the VERA's registers 0-31 are `$FF20-$FF3F`, `VERA_BASE` in `include/hw.inc`; the reset makes its FPGA configure
itself again (`vid` looks for the card for 0.3 s after one).

## Optional: 5 V data levels (U3, 74AHCT245)

The VERA X takes 5 V inputs (the X16's VERA module does it with an SN74CBTD3861 bus switch) but drives 3.3 V highs.
The W65C02S's data inputs are specified high at 0.7 VDD, **3.5 V**, so a 3.3 V high is 0.2 V short of the data
sheet.  The X16 and the OtterX run this way, as a CMOS input's real threshold is near 2.5 V, so try without it
first; if the card is found unreliably or reads come back wrong, put a 74AHCT245 between the VERA's data pins and
the bus (its TTL inputs take the 3.3 V, its outputs give 5 V):

| U3 pin | Function | Connect to |
| :--- | :--- | :--- |
| 1 | DIR | J7 pin 4 (RWB: high, A to B, the VERA to the bus) |
| 2-9 | A1-A8 | VERA pins 12, 11, 10, 9, 8, 7, 6, 5 (D0-D7) |
| 10 | GND | GND |
| 11-18 | B8-B1 | J2 pins 1-8 (D7 at pin 11, down to D0 at pin 18) |
| 19 | /OE | U1 pin 14 (`CS#`) |
| 20 | VCC | +5 V, 0.1 µF to GND |

With it, the VERA's data pins go to U3's A side only, not to J2.

## Power, grounds, wires

* **Power**: J11 pin 4 (+5 V) to VERA pin 3 and the chips; J11 pin 5 (GND) to VERA pins 4, 23 and 24 (all three)
  and the chips.  Use a heavier wire for the ground than for the signals: the whole data bus returns through it.
  10 µF and 0.1 µF across the VERA's pins 3 and 4, at the header.
* **Never J11 pin 3** (-5 V, whatever the card says).
* Keep the bus wires short and together; run a ground wire next to PHI2.  Plug and unplug with the power off.
* The VERA X's SD-card header and its I2C pins stay unconnected; its VGA output goes to the monitor.

## Audio

The VERA's line outputs go into the mixer's slot 0 inputs: 10K resistors into an LF353 on ±12 V, wired as an
inverting summer (its inputs at a virtual ground), beside the YM2151.  A ground-centred line output connects
straight in.  Before wiring them, measure pins 25 and 26 against GND with the card running: if they sit at a DC
level (a DAC biased at half its supply) rather than about 0 V, put a 10 µF capacitor in series with each (its +
toward the VERA if the level is positive), or the mixer will amplify the offset too.

The mixer is a unity-gain summer (10K in, 10K feedback, for each of the slots' pairs and the YM2151's), so the VERA
comes out as loud as its line output is, which may be louder than the YM2151.  If so, a divider in each line evens
them (10K in series and 4.7K to GND at the mixer's side: about a third), as the carrier card's plan has it.

## The interrupt

Slot 0's IRQ A is its alone, with a 3.3K pull-up on the board into the 74LS148, so the VERA's `IRQ#` can drive it
directly: low for an interrupt, high (3.3 V) or let go otherwise.  The carrier card's plan puts an open-collector
buffer in the way, so the card only ever pulls the line low; on the bench a 74LS07 gate (VERA pin 16 to its input,
its output to J5 pin 6) does the same, if the line misbehaves.

## The keyboard and mouse: the X16's SMC

The input controller is the X16's own (VIDEO.md's step 6): its SMC, an **ATtiny861** with the X16 community's
firmware, `x16-smc` (github.com/X16Community/x16-smc), unchanged.  A PS/2 keyboard and a PS/2 mouse plug into it,
and it talks to the Hydra over the I2C bus at address `$42`, which every slot carries and the breakout card brings
to J10.  The `input` program reads it ([programming/video.md](programming/video.md#the-keyboard-and-the-mouse)).

* **What you need**: an ATtiny861 (the 20-pin DIP) programmed with an `x16-smc` release (its HEX file and fuses, as
  its README gives them; a TL866-class programmer does it), two 6-pin mini-DIN sockets (PS/2), a push button, a 10K
  resistor and a 0.1 µF capacitor.
* **Its pins** are the firmware's (`smc_pins.h`, the default build, not its `COMMUNITYX16_PINS` one; the firmware
  numbers its pins 0-7 for PA0-PA7 and 8-15 for PB0-PB7), on the chip's DIP pins:

| ATtiny861 pin | Port | The firmware's name | To |
| :--- | :--- | :--- | :--- |
| 1 | PB0 | `I2C_SDA_PIN` | J10 pin 1 (SDA) |
| 3 | PB2 | `I2C_SCL_PIN` | J10 pin 2 (SCL) |
| 18 | PA2 | `PS2_KBD_CLK` | The keyboard's socket, pin 5 (clock) |
| 4 | PB3 | `PS2_KBD_DAT` | The keyboard's socket, pin 1 (data) |
| 9 | PB6 | `PS2_MSE_CLK` | The mouse's socket, pin 5 |
| 8 | PB5 | `PS2_MSE_DAT` | The mouse's socket, pin 1 |
| 14 | PA4 | `POWER_BUTTON_PIN` | A push button to GND |
| 17 | PA3 | `PWR_OK` | +5 V through the 10K (the X16's power supply, "good") |
| 5, 15 | VCC, AVCC | | +5 V (J11 pin 4), the 0.1 µF to GND at the chip |
| 6, 16 | GND, AGND | | GND (J11 pin 5) |
| 10 | PB7 | (its RESET) | Nothing (the programmer's) |
| 20, 19, 2, 13, 12, 7, 11 | PA0, PA1, PB1, PA5, PA6, PB4, PA7 | `RESB_PIN`, `NMIB_PIN`, `IRQB_PIN`, `PWR_ON`, `ACT_LED`, the reset and NMI buttons | Nothing: they're the X16's |

The sockets' pin 3 is GND and pin 4 +5 V; pins 2 and 6 aren't used.

* **The I2C bus** has its pull-ups on the main board (RN1), so the SMC needs none.  Nothing else on the board
  answers at `$42`.  `IRQB_PIN` stays unconnected: the Hydra polls the SMC, as the X16 does, since the board's IRQ
  lines can't be masked (VIDEO.md says why).
* **Press the button once after switching on.**  The firmware is a power supply's controller first: its power-on,
  which the power button starts, is what starts the keyboard and mouse (their lines' pull-ups on, each reset), and it
  checks `PWR_OK`, held high here.  Till then it answers on I2C but has no keys.  (A build of `x16-smc` that powered
  on as it starts would do away with the button.)
* **Its version** is the first thing `input` reads: `ls /dev/i2c` lists `42` once the SMC's on the bus, and `ps`
  shows `input` running (with no SMC it ends at once).

## Timing (why the strobes are made so)

* **The VERA latches a write's data when its write strobe ends**: `bus_write = !CS# && !WR#`, data captured on its
  falling edge (`top.v`).  The 65C02S holds write data only **10 ns** after PHI2 falls (tDHW).  So `WR#` must end the
  write, promptly: it's PHI2 through one 74ACT00 gate, rising 3-9 ns after PHI2 falls.  (`CS#` comes through the
  board's decode and ends later: as the write's end it would miss the data.)
* **Reads**: the 65C02S needs data 10 ns before PHI2 falls (tDSR).  `CS#` falls some 15-20 ns after PHI2 rises
  (`nIO_S` from the board's 74F32, then U1), and the VERA's data follows within tens of ns.  PHI2 is high about
  140 ns at 3.58 MHz and 70 ns at 7.16 MHz: room at both.
* **No glitches**: `nIO_S` is low only while PHI2 is high, when the address is steady, so `CS#` can't pulse while it
  changes.
* **The slot's own port selects instead** (`CS#` = J5 pin 4 AND J5 pin 3, one gate of a 74AHCT08, in place of U1)
  also work, at 3.58 MHz: they come through the board's 74LS154, 25-35 ns more, which leaves too little of a 7.16
  MHz cycle.  U1 is as many chips and works at both speeds.

## Bringing it up

1. **The glue board alone** (no VERA on it), the breakout card in slot 0, the Hydra on at 3.58 MHz: check +5 V and
   GND at the VERA socket's pins 3 and 4.  With a logic probe or a scope, watch `CS#` (U1 pin 14), `RD#` (U2 pin 3)
   and `WR#` (U2 pin 8) while something touches `$FF20-$FF3F`: in BASIC, `10 POKE 65313,85: X=PEEK(65313): GOTO
   10`, `RUN` (Ctrl-C stops it).  `CS#` should pulse on every access, `WR#` on the POKE, `RD#` on the PEEK, each
   only while PHI2 is high.
2. **The VERA X**, power off, then on: the console comes up on the monitor (`vid` found the card; `cons` shows the
   windows on both the screen and the serial terminal).  `cat /dev/vid/ctl` starts `vera 47.0.2` (the gateware's
   version).
3. **Try it**: `echo border 4 >/dev/vid/ctl` (the border's colour), `cat /lib/font/cp437 >/dev/vid/font` (the PC's
   characters), `echo note 8 69 >/dev/sndctl` (an A from the PSG, through the mixer; `echo off 8 >/dev/sndctl`), `play`
   a WAV file (the PCM).
4. **No `/dev/vid`** (no card found): check +5 V, the data lines' order (VERA pin 5 is D7), A0-A4, `CS#`, `RD#` and
   `WR#` (each active low), and `RES#` (it must be high once the reset is over).  Then U3, for the 3.3 V highs.
5. **The keyboard and mouse** (the SMC): `ls /dev/i2c` lists `42`; press its button; then keys typed on the PS/2
   keyboard reach the shell, its Caps Lock lights, and `cat /dev/vid/mouse` prints a line each time the mouse moves,
   the pointer following it on the screen.
6. **Then 7.16 MHz**, once everything works at 3.58 (the board's faster clock, and a build with `--clock 2`).

[programming/video.md](programming/video.md) is the programmer's side: the driver's files, claims, the PSG and the
PCM, the keyboard and the mouse.
