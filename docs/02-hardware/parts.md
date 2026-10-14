# The parts

Every part on every board, with what to buy.  The tables between the markers are made from the schematics by
`node docs/build/bom.js` (from `board/`): don't edit them by hand, edit the schematic and run it again.  The prose
around them is written by hand.

## Contents

1. [Reading the tables](#reading-the-tables)
2. [Buying the parts](#buying-the-parts): the chips that need care, substitutes, sockets
3. [The main board](#the-main-board)
4. [The memory daughter card](#the-memory-daughter-card) (through-hole and surface mount)
5. [The bus breakout card](#the-bus-breakout-card)
6. [The Vera X glue board](#the-vera-x-glue-board)
7. [What each chip does](#what-each-chip-does)

---

## Reading the tables

* **Qty** is how many of that part, value and package the board takes.
* **Part** is the schematic's value.  Capacitors and resistors are marked the way their bodies are, with the three-digit
  EIA code, and the plain value beside it: **104** is 10 x 10^4 pF, 100 nF; **103** is 10 x 10^3 ohms, 10K; **220** is
  22 ohms (22 x 10^0).  **NP** is a non-polar (bipolar) electrolytic.
* **Package** is the footprint's: DIP-n is a through-hole chip with n pins, 0.3" wide unless it says 0.6"; SOP-32 a
  surface-mount chip; "ceramic, through-hole" a small disc or MLCC capacitor at 2.54 or 5.08 mm pitch; "radial" an
  electrolytic, its diameter given.
* **Socket** is the socket the footprint is drawn for.  Every DIP chip on the main board is socketed, which makes a
  fault easy to fix and a chip easy to try in another place.  **Sockets to buy** totals them.
* **References** are the schematic's, in ranges: C1-C4 is C1, C2, C3 and C4.  [The board layout](layout.md) shows where
  each is.

---

## Buying the parts

Most of the board is ordinary 74-series logic, resistors and capacitors from any distributor.  A few parts need care.

**The CPU, W65C02S** (U1).  Buy the current WDC part, W65C02S6TPG-14 (DIP-40), from a distributor: it's still made.  An
old NMOS 6502 won't do: the software uses the 65C02's instructions throughout, the kernel's idle loop uses the
W65C02S's `WAI`, and DMA relies on its static core (it stops the CPU's clock).

**The VIA and the ACIA** (U2, U3).  The schematic has Rockwell's R65C22 and R65C51, which are long out of production.
WDC's W65C22N and W65C51N are current:

* **The W65C22N** is a drop-in replacement (the N has an open-drain IRQ output, as the R65C22 has).
* **The W65C51N** has a known fault: its transmitter's status bit is stuck "empty" and it gives no transmitter
  interrupt.  The software allows for it: program U6 with `bios_wdc.bin` ([programming the ROMs](building.md#programming-the-roms)).

Both WDC chips have run on a 1.8D (2026-10-13: a W65C22 in U2 with no change, a W65C51N in U3 with the WDC
build).  U2's IRQ pin is the priority encoder's own input (`nIRQ0`), not a shared line, so the push-pull W65C22S works
there as well as the open-drain N.

**The sound chips** (U38 YM2151, U41 YM3012).  These are only available used, pulled from old equipment, and
fakes are common: buy from a seller with a good record, and test them (the hardware test checks the YM2151's busy
flag and timers).  The board works without them; leave U38 and U41 out and the system just has no FM sound.
A modern alternative is the **OPM2151** (an FPGA re-implementation of the YM2151, pin-compatible and cycle-accurate,
sold on Tindie by Joe's Retro Sto'), with an optional I2S DAC board that fits U41's socket in place of a YM3012:
close the OPM2151's solder jumper JP2 ("I2S Mode") for that.  Untested on the Hydra at the time of writing.

**The 74LS219s** (IC1-IC8): 16 x 4-bit RAMs, the bank registers and the vector RAM.  They're out of production, but
easy to find as surplus.  The **74LS189** is the same chip with inverted outputs, and **won't** do.

**The RAMs**, HM628512 (512K x 8, 55 or 70 ns).  Hitachi's are surplus now; Alliance's **AS6C4008** (55 ns) is current and pin
compatible: the DIP-32 for U7 and the memory cards, the SOP-32 for the shared RAM (check that its body fits the
footprint, 11.3 mm wide).  U7 can be a **DS1747** instead, for
a clock that keeps time ([the devices](devices.md#the-clock-chip)).

**The flash ROMs.**  Microchip's SST39SF040 (512K, DIP-32, 70 ns) for the paged ROM, and the SST39SF010A (128K) in
PLCC-32 for the BIOS ROM.  Both are current.  Buy only as many SST39SF040 as the system needs: HydraOS uses three (U31,
U32, U34), the base alone one (U31).

**The reset supervisor** (U43, DM1813 in TO-92): the DS1813-10 (Dallas, now Analog Devices).

**The mixer's op-amp** (IC9, NJM4556A): a dual op-amp that drives headphones.  An LF353 fits the socket but distorts
into headphones (it's what 1.8C had).

**The ESD array** (J28, SP720): Littelfuse's SP720APP, in a DIP-16 socket.  Optional, but fit it if J27 is used.

**The crystal** (Y1, 14.31818 MHz, HC-18 or HC-49).  The NTSC colour-burst frequency times four, so it's cheap and
common.

**Families.**  The schematic's families matter in a few places:

* **74F and 74ACT** where it says so: the decoding must be fast, and the 1.8D board uses ACT where a part drives a
  CMOS input (the CPU's clock, IRQ and data inputs).  An F part in an ACT place works on most boards, but out of
  specification.
* **74LS** parts can be LS or ALS; **74F** parts mustn't be LS (they're too slow at 7.16 MHz).
* Don't substitute HC or HCT parts: they're slower than F and ACT.

**Jumper shunts** (2.54 mm): one for the CPU clock (J7), one for the CTS jumper (J30), and one for each memory card.

**Power.**  Any ATX power supply with a 24-pin connector: the board needs far less than any of them gives.

---

## The main board

`board/hydra-16.kicad_sch`, revision 1.8D.  The memory cards, the Vera X and its glue are separate (below).

<!-- parts:main:begin -->

**ICs**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | 74ACT08 | DIP-14 | DIP-14 (0.3") | U11 |
| 1 | 74ACT14 | DIP-14 | DIP-14 (0.3") | U39 |
| 1 | 74ACT240 | DIP-20 | DIP-20 (0.3") | U22 |
| 1 | 74ACT245 | DIP-20 | DIP-20 (0.3") | U59 |
| 2 | 74F00 | DIP-14 | DIP-14 (0.3") | U15, U44 |
| 1 | 74F02 | DIP-14 | DIP-14 (0.3") | U57 |
| 1 | 74F125 | DIP-14 | DIP-14 (0.3") | U61 |
| 3 | 74F138 | DIP-16 | DIP-16 (0.3") | U4, U20, U24 |
| 1 | 74F139 | DIP-16 | DIP-16 (0.3") | U26 |
| 1 | 74F157 | DIP-16 | DIP-16 (0.3") | U45 |
| 1 | 74F191 | DIP-16 | DIP-16 (0.3") | U40 |
| 1 | 74F20 | DIP-14 | DIP-14 (0.3") | U10 |
| 1 | 74F30 | DIP-14 | DIP-14 (0.3") | U14 |
| 3 | 74F32 | DIP-14 | DIP-14 (0.3") | U13, U16, U56 |
| 7 | 74F541 | DIP-20 | DIP-20 (0.3") | U21, U49, U51, U53, U55, U58, U60 |
| 4 | 74F573 | DIP-20 | DIP-20 (0.3") | U48, U50, U52, U54 |
| 1 | 74F86 | DIP-14 | DIP-14 (0.3") | U12 |
| 2 | 74LS148 | DIP-16 | DIP-16 (0.3") | U17, U18 |
| 2 | 74LS154 | DIP-24 | DIP-24 (0.6") | U19, U23 |
| 8 | 74LS219 | DIP-16 | DIP-16 (0.3") | IC1-IC8 |
| 2 | 74LS260 | DIP-14 | DIP-14 (0.3") | U8, U9 |
| 1 | DM1813-10 | TO-92 Inline Wide |  | U43 |
| 1 | HM628512LP-5 | DIP-32 | DIP-32 (0.6") | U7 |
| 4 | HM628512LP-5 | SOP-32 |  | U25, U27-U29 |
| 1 | MAX232 | DIP-16 | DIP-16 (0.3") | U5 |
| 1 | NJM4556AD | DIP-8 | DIP-8 (0.3") | IC9 |
| 1 | R65C22-P2 | DIP-40 | DIP-40 (0.6") | U2 |
| 1 | R65C51-P2 | DIP-28 | DIP-28 (0.6") | U3 |
| 8 | SST39SF040 | DIP-32 | DIP-32 (0.6") | U30-U37 |
| 1 | SST39SF0x0A | PLCC-32 | PLCC-32 | U6 |
| 1 | TL074 | DIP-14 | DIP-14 (0.3") | U42 |
| 1 | WDC65C02S | DIP-40 | DIP-40 (0.6") | U1 |
| 1 | YM2151 | DIP-24 | DIP-24 (0.6") | U38 |
| 1 | YM3012 | DIP-16 | DIP-16 (0.3") | U41 |

**Crystals and oscillators**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | 14.31818MHz | crystal HC18 |  | Y1 |

**Capacitors**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 68 | 104 (100 nF) | ceramic, through-hole |  | C1-C3, C5, C6, C12-C46, C57-C64, C66-C85 |
| 5 | 105 (1 µF) | ceramic, through-hole |  | C4, C7-C10 |
| 2 | 106 (10 µF) | radial 5.0 mm |  | C90, C91 |
| 5 | 107 (100 µF) | radial 6.3 mm |  | C51, C53-C56 |
| 2 | 227 NP (220 µF) | radial 8.0 mm |  | C92, C93 |
| 2 | 330 (33 pF) | ceramic, through-hole |  | C47, C48 |
| 2 | 332 (3.3 nF) | ceramic, through-hole |  | C50, C52 |
| 1 | 475 (4.7 µF) | ceramic, through-hole |  | C49 |
| 4 | 476 (47 µF) | radial 6.3 mm |  | C86-C89 |

**Resistors and networks**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | 0 (a link) | axial resistor |  | R29 |
| 1 | 101 (100 Ω) | axial resistor |  | R3 |
| 11 | 103 (10K) | axial resistor |  | R4-R7, R13, R19-R21, R28, R32, R33 |
| 1 | 103 (10K) | SIP-9 network |  | RN4 |
| 1 | 105 (1M) | axial resistor |  | R1 |
| 5 | 220 (22 Ω) | axial resistor |  | R23-R27 |
| 2 | 330 (33 Ω) | axial resistor |  | R30, R31 |
| 1 | 331 (330 Ω) | axial resistor |  | R22 |
| 3 | 332 (3.3K) | SIP-9 network |  | RN1-RN3 |
| 1 | 471 (470 Ω) | axial resistor |  | R2 |

**Connectors, headers and jumpers**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | 0.89489MHz | header 2x01, 2.54 mm |  | J5 |
| 1 | 1.78977MHz | header 2x01, 2.54 mm |  | J6 |
| 1 | 3.57955MHz | header 2x01, 2.54 mm |  | J7 |
| 1 | 7.15909MHz | header 2x01, 2.54 mm |  | J8 |
| 1 | ATX-24 | Molex Mini-Fit Jr 5566-24A2 2x12 P4.20mm Vertical |  | J11 |
| 1 | Audio In | header 2x01, 2.54 mm |  | J29 |
| 1 | Audio Jack | Jack 3.5mm CUI SJ1-3525N Horizontal |  | J26 |
| 1 | CTS | header 1x03, 2.54 mm |  | J30 |
| 1 | Conn_01x24 | socket 1x24, 2.54 mm |  | J2 |
| 1 | Conn_01x32 | socket 1x32, 2.54 mm |  | J1 |
| 1 | DSUB_9 | L77SDE09SA4CH3R |  | J3 |
| 1 | Front Panel | header 2x06, 2.54 mm |  | J10 |
| 6 | SLOT 0-5 | TE 7-5530843-0 |  | J12-J17 |
| 1 | SND CT1/2 | header 2x01, 2.54 mm |  | J9 |
| 1 | SP720APP | DIP-16 | DIP-16 (0.3") | J28 |
| 8 | SPI Device 0-7 | socket 1x06, 2.54 mm |  | J18-J25 |
| 1 | VIA Port A | header 2x06, 2.54 mm |  | J27 |

**Sockets to buy**

| Qty | Socket |
| --: | :----- |
| 15 | DIP-14 (0.3") |
| 19 | DIP-16 (0.3") |
| 13 | DIP-20 (0.3") |
| 3 | DIP-24 (0.6") |
| 1 | DIP-28 (0.6") |
| 9 | DIP-32 (0.6") |
| 2 | DIP-40 (0.6") |
| 1 | DIP-8 (0.3") |
| 1 | PLCC-32 |

216 parts on the main board, from `board/hydra-16.kicad_sch`.

<!-- parts:main:end -->

---

## The memory daughter card

A task RAM module is 2 MB, which gives each task 16 more banks; the main board takes up to 15 modules.  There are two
versions of the card:

### Through-hole

`board/MemoryDaughterCard/MemDaughterCard.kicad_sch`: four DIP-32 RAMs, one module, 2 MB.

<!-- parts:memory:begin -->

**ICs**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | 74LS139 | DIP-16 |  | U1 |
| 4 | HM628512LP-7 | DIP-32 | DIP-32 (0.6") | IC1-IC4 |

**Capacitors**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 5 | .1uF | ceramic, through-hole |  | C1-C5 |

**Connectors, headers and jumpers**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | Conn_01x24 | header 1x24, 2.54 mm |  | J2 |
| 1 | Conn_01x32 | header 1x32, 2.54 mm |  | J1 |
| 1 | Conn_02x15_Odd_Even | header 2x15, 2.54 mm |  | J3 |

**Sockets to buy**

| Qty | Socket |
| --: | :----- |
| 4 | DIP-32 (0.6") |

13 parts on the memory daughter card (through-hole), from `board/MemoryDaughterCard/MemDaughterCard.kicad_sch`.

<!-- parts:memory:end -->

### Surface mount

`board/MemoryDaughterCard/MemDaughterCard_SurfaceMount.kicad_sch`: eight SOP-32 RAMs, which make it **two** modules,
4 MB.  J3 chooses the first module's number (IC1-IC4) and J4 the second's (IC5-IC8): [the cards](cards.md#the-memory-daughter-card).

<!-- parts:memory-smd:begin -->

**ICs**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | 74LS139 | DIP-16 |  | U1 |
| 8 | HM628512LFP-7 | SOIC127P1414X300-32N |  | IC1-IC8 |

**Capacitors**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 5 | .1uF | ceramic, through-hole |  | C1-C5 |

**Connectors, headers and jumpers**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | Conn_01x15_Pin | header 1x15, 2.54 mm |  | J4 |
| 1 | Conn_01x24 | header 1x24, 2.54 mm |  | J2 |
| 1 | Conn_01x32 | header 1x32, 2.54 mm |  | J1 |
| 1 | Conn_02x15_Odd_Even | header 2x15, 2.54 mm |  | J3 |

18 parts on the memory daughter card (surface mount), from `board/MemoryDaughterCard/MemDaughterCard_SurfaceMount.kicad_sch`.

<!-- parts:memory-smd:end -->

---

## The bus breakout card

`board/HydraBusBreakoutCard/HydraBusBreakoutCard.kicad_sch`: a slot card with every bus signal on headers, for a logic
analyser or for wiring a prototype.  It has no chips.

<!-- parts:breakout:begin -->

**Connectors, headers and jumpers**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | Address Bus | header 1x16, 2.54 mm |  | J3 |
| 1 | Audio | header 1x03, 2.54 mm |  | J9 |
| 1 | Bus Control | header 1x06, 2.54 mm |  | J7 |
| 1 | Bus_Hydra_8bit | BUS Hydra 8Bit |  | J1 |
| 1 | Clocks | header 1x05, 2.54 mm |  | J6 |
| 1 | Data Bus | header 1x08, 2.54 mm |  | J2 |
| 1 | I2C | header 1x02, 2.54 mm |  | J10 |
| 1 | Interrupts & I/O | header 1x06, 2.54 mm |  | J5 |
| 1 | Power | header 1x05, 2.54 mm |  | J11 |
| 1 | Reset | header 1x01, 2.54 mm |  | J8 |
| 1 | SPI | header 1x08, 2.54 mm |  | J4 |

11 parts on the bus breakout card, from `board/HydraBusBreakoutCard/HydraBusBreakoutCard.kicad_sch`.

<!-- parts:breakout:end -->

---

## The Vera X glue board

`board/vera-x/vera-x.kicad_sch`: the protoboard that connects the Vera X to a breakout card in slot 0, with the
keyboard and mouse controller ([the cards](cards.md#the-vera-x)).  The Vera X card itself is bought (Joe Burks's VERA X
6.1), and isn't listed.

<!-- parts:vera-x:begin -->

**ICs**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | 74ACT00 | DIP-14 |  | U2 |
| 1 | 74AHCT138 | DIP-16 |  | U1 |
| 1 | ATtiny861 (x16-smc) | DIP-20 |  | U4 |

**Capacitors**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 5 | 100nF | ceramic, through-hole |  | C2-C6 |
| 1 | 10uF | radial 5.0 mm |  | C1 |

**Resistors and networks**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | 10K | axial resistor |  | R1 |

**Connectors, headers and jumpers**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | Breakout J10: I2C | header 1x02, 2.54 mm |  | J10 |
| 1 | Breakout J11: power (pin 3 is -5 V!) | header 1x05, 2.54 mm |  | J11 |
| 1 | Breakout J2: data bus | header 1x08, 2.54 mm |  | J2 |
| 1 | Breakout J3: address bus (A15 at 1) | header 1x16, 2.54 mm |  | J3 |
| 1 | Breakout J5: interrupts & I/O | header 1x06, 2.54 mm |  | J5 |
| 1 | Breakout J6: clocks | header 1x05, 2.54 mm |  | J6 |
| 1 | Breakout J7: bus control (pin 5 is /DMAB) | header 1x06, 2.54 mm |  | J7 |
| 1 | Breakout J8: reset | header 1x01, 2.54 mm |  | J8 |
| 1 | Breakout J9: audio | header 1x03, 2.54 mm |  | J9 |
| 1 | Data: 8 shunts (without U3) | header 2x08, 2.54 mm |  | JP1 |
| 1 | PS/2 keyboard |  |  | J12 |
| 1 | PS/2 mouse |  |  | J13 |
| 1 | VERA X 6.1 (2x13 socket) | socket 2x13, 2.54 mm |  | J1 |

**Switches**

| Qty | Part | Package | Socket | References |
| --: | :--- | :------ | :----- | :--------- |
| 1 | Power (press once at power-up) | SW PUSH 6mm |  | SW1 |

24 parts on the Vera X glue board, from `board/vera-x/vera-x.kicad_sch`.

<!-- parts:vera-x:end -->

---

## What each chip does

The main board's chips, by what they're for.  [The architecture](architecture.md) explains each part of the board.

| Chip | Part | What it does |
| :--- | :--- | :----------- |
| U1 | W65C02S | The CPU |
| U2 | 65C22 | The VIA: the tick, GPIO, I2C, SPI |
| U3 | 65C51 | The ACIA: the serial port |
| U4 | 74F138 | The SPI device select (headers J18-J25) |
| U5 | MAX232 | The serial port's RS-232 levels |
| U6 | SST39SF010A | The BIOS ROM: the kernel |
| U7 | HM628512 or DS1747 | Task RAM, 32K for each task |
| U8, U9 | 74LS260 | Address decoding: `$000x` (the bank registers' writes) |
| U10 | 74F20 | Address decoding |
| U11 | 74ACT08 | Address decoding; the CPU's clock, gated by DMA |
| U12 | 74F86 | The system port's decoding |
| U13, U16 | 74F32 | Address decoding: the windows' selects, qualified by PHI1 |
| U14 | 74F30 | The I/O page, `$FFxx` |
| U15 | 74F00 | Address decoding |
| U17, U18 | 74LS148 | The interrupt priority encoders |
| U19 | 74LS154 | The 16 I/O ports |
| U20 | 74F138 | The system port: `T`, `U`, `V`, `W` and the vector RAM |
| U21 | 74F541 | The memory card connector's buffer |
| U22 | 74ACT240 | Drives IRQB, PHI1, DMA and !RWB |
| U23 | 74LS154 | The RAM window's module select |
| U24 | 74F138 | The paged ROM's chip select |
| U25, U27-U29 | HM628512 (SOP-32) | Shared RAM, 2 MB |
| U26 | 74F139 | The shared RAM's chip select |
| U30-U37 | SST39SF040 | The paged ROM, 4 MB |
| U38 | YM2151 | FM sound |
| U39 | 74ACT14 | The crystal oscillator, and the clocks' drivers |
| U40 | 74F191 | The clock divider |
| U41 | YM3012 | The YM2151's DAC |
| U42 | TL074 | The DAC's buffers and filters |
| U43 | DM1813 | The reset supervisor |
| U44 | 74F00 | The IRQ to the CPU |
| U45 | 74F157 | The vector RAM's index: the active line, or `V` |
| U48, U50, U52, U54 | 74F573 | The latches of `T`, `U`, `V`, `W` |
| U49, U51, U53, U55 | 74F541 | Their read-back buffers |
| U56 | 74F32 | The latches' strobes |
| U57 | 74F02 | The latches' strobes |
| U58, U60 | 74F541 | The address bus buffers |
| U59 | 74ACT245 | The data bus transceiver |
| U61 | 74F125 | The R/W buffers |
| IC1-IC4 | 74LS219 | The bank registers, `$00` and `$01` |
| IC5-IC8 | 74LS219 | The vector RAM |
| IC9 | NJM4556A | The audio mixer and headphone driver |
| J28 | SP720 | ESD protection for J27 and the SPI lines |
