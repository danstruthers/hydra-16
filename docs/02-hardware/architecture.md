# The architecture

This page is how the board works: what the processor sees at each address, and which chips make it so.  The
[overview](../01-overview.md#the-ideas-behind-it) gave the ideas; here they're taken down to the gates.  The sheet named
under each heading is the schematic sheet to look at (`board/NAME.kicad_sch`).

## Contents

1. [The board at a glance](#the-board-at-a-glance)
2. [The memory map](#the-memory-map)
3. [The pseudo-registers T, U, V, W](#the-pseudo-registers-t-u-v-w)
4. [Task RAM and the bank registers](#task-ram-and-the-bank-registers)
5. [The RAM window](#the-ram-window)
6. [The paged ROM](#the-paged-rom)
7. [The BIOS ROM](#the-bios-rom)
8. [I/O space](#io-space)
9. [Interrupts](#interrupts)
10. [Clocks](#clocks)
11. [Reset, power and the bus](#reset-power-and-the-bus)
12. [The chips by function](#the-chips-by-function)

---

## The board at a glance

| | |
| :-- | :-- |
| **CPU** | WDC W65C02S (U1), 3.58 MHz as built (a jumper chooses 0.89, 1.79, 3.58 or 7.16 MHz) |
| **Task RAM** | 512K (U7, HM628512, or a DS1747 for a clock that keeps time): 32K at `$0000-$7FFF` for each of 16 tasks |
| **The RAM window** | 8K at `$8000-$9FFF`.  **Task banks:** up to 15 memory cards of 2 MB, each giving every task 16 banks of its own.  **Shared banks:** 2 MB on the board (U25, U27-U29), the same for every task: 256 banks, 16 at a time |
| **Paged ROM** | 4 MB (U30-U37, eight SST39SF040), seen in 16K banks at `$A000-$DFFF` |
| **BIOS ROM** | U6, an SST39SF010A (128K; a '020A or '040 fits too), seen in 8K pages at `$E000-$FFFF` |
| **I/O** | 15 ports of 16 bytes at `$FF00-$FFEF`, and the system port at `$FFF0-$FFFF` |
| **Interrupts** | 16 IRQ lines in priority order, each with its own vector (a 16-entry vector RAM read at `$FFFE`) |
| **Devices** | A 65C22 VIA (timers, GPIO, I2C, SPI), a 65C51 ACIA (RS-232 on a DE-9), a YM2151 and YM3012 (stereo FM sound) with a mixer |
| **Expansion** | Six slots (62-pin card edge, 8-bit bus), eight SPI device headers, a GPIO and I2C header, the memory card connectors |
| **Video** | The Vera X in slot 0 ([the cards](cards.md#the-vera-x)) |
| **Power** | An ATX PC supply (+5 V, +3.3 V, +12 V, -12 V) |

There are no programmable logic chips on the board: everything is 74-series logic, so every signal can be followed
on the schematic and probed on the board.

---

## The memory map

What the CPU sees at each address, and what chooses it:

| Addresses | What | Chosen by |
| :-------- | :--- | :-------- |
| `$0000` | RAM bank register (writes); task RAM (reads) | per task |
| `$0001` | ROM bank register (writes); task RAM (reads) | per task |
| `$0002-$7FFF` | Task RAM: page zero, the stack, the program's RAM | `T` |
| `$8000-$9FFF` | The RAM window: a task bank (`$00-$EF`) or a shared bank (`$F0-$FF`) | `$00`; and `T` for task banks, `U` for shared banks |
| `$A000-$DFFF` | Paged ROM, a 16K bank | `$01` |
| `$E000-$FEFF` | BIOS ROM, an 8K page | `W` |
| `$FF00-$FFEF` | I/O ports 0-14, 16 bytes each | |
| `$FFF0-$FFF3` | The pseudo-registers `T`, `U`, `V`, `W` | |
| `$FFF4-$FFF9` | Not decoded: reads give `$FF` (1.8D) | |
| `$FFFA-$FFFD` | BIOS ROM: the NMI and RESET vectors | `W` |
| `$FFFE-$FFFF` | The IRQ/BRK vector, from the vector RAM (16 entries) | the highest-priority active IRQ line, or `V` |

Every task has its own `$0000-$7FFF` and its own `$00` and `$01`.  All tasks share the I/O space, the shared RAM
banks, both ROMs, and the pseudo-registers themselves.

---

## The pseudo-registers T, U, V, W
*(Sheet `FFF_Registers`)*

Four 8-bit registers in the system port.  Each is a 74F573 latch, written by the CPU's write strobe, and read back
through a 74F541 buffer, so a read gives back all 8 bits last written.

| Address | Register | Latch / read-back | The bits the hardware uses |
| :------ | :------- | :---------------- | :------------------------- |
| `$FFF0` | `T`, the task | U48 / U49 | T0-T3: which task's RAM (U7's A15-A18; the memory cards' task lines) |
| `$FFF1` | `U`, the shared macro-page | U50 / U51 | U0-U3: the shared RAM's A13-A16 |
| `$FFF2` | `V`, the vector select | U52 / U53 | V0-V3: the vector RAM's entry when no IRQ line is active.  V4-V7 go nowhere: the kernel keeps a software interrupt's number there |
| `$FFF3` | `W`, the BIOS ROM page | U54 / U55 | W0-W5: the BIOS ROM's A13-A18 (16 pages on the '010A, 64 on a '040) |

* **They power up random.**  The latches have no reset: the boot code sets them all.  Pressing the reset button
  doesn't change them either.
* **The unused bits are kept.**  Bits 4-7 of `T` and `U`, and 6-7 of `W`, are stored and read back, but drive nothing.
* **`T` is the one that matters most.**  Writing it switches every per-task thing at once: the task RAM, the bank
  registers' outputs (so the RAM window's bank and the ROM bank), and the memory cards' task lines.

---

## Task RAM and the bank registers
*(Sheets `ZPMirrorRAM`, `AddressDecode`)*

**Task RAM** is one 512K static RAM (U7, an HM628512 or a DS1747) on the main board:

* CPU A0-A14 go straight to it; CPU A15 is its chip enable, low for `$0000-$7FFF`.
* `T0-T3` drive its A15-A18, so each value of `T` selects a different 32K.
* Page zero and the stack page are part of it, so every task has its own zero page and stack, and a task switch is a
  single write to `$FFF0`.

**The DS1747 option.**  U7's socket also takes a Dallas DS1747 (the 5 V part, not the 3.3 V DS1747W): a 512K
battery-backed RAM with a real-time clock, pin compatible, in a 600-mil module.  Its clock's registers are the chip's
top 8 bytes, which are **task F's `$7FF8-$7FFF`**.  The kernel looks for the clock at boot and sets the time from it;
the rest of the chip is task RAM as before, kept while the power's off.  [The devices](devices.md#the-clock-chip) say
more.

**The bank registers `$00` and `$01`** are four 74LS219 chips, small 16 x 4-bit RAMs, addressed by `T0-T3`: so each
task has its own pair of registers.

| Register | Chips | Drives |
| :------- | :---- | :----- |
| `$00`: the RAM bank | IC1 (bits 0-3), IC2 (bits 4-7) | `RAMB0-7`: which bank is at `$8000-$9FFF` |
| `$01`: the ROM bank | IC3 (bits 0-3), IC4 (bits 4-7) | `ROMB0-7`: which paged ROM bank is at `$A000-$DFFF` |

* A write to `$0000` or `$0001` stores the byte in the current task's entry.  The decoding (U8-U10) spots a write to
  `$000x`, and U16 picks the register by A0.
* The same write also goes to task RAM as usual.  The registers can't be read; a **read** of `$00` or `$01` comes from
  task RAM, which holds the same value.  So a program can read its own bank selections back.
* The chips' outputs are always on, so the bank lines follow `T` at once: a task switch brings the new task's RAM bank
  and ROM bank in with no further writes.
* On this board, two pairs of bank bits are crossed on their way to the memory.  The software never notices, but it
  matters when you trace a fault to a chip: see [the quirks](quirks.md#the-crossed-bank-bits).

---

## The RAM window
*(`$8000-$9FFF`; sheets `ZPMirrorRAM`, `SharedMem`, `AddressDecode`)*

`$8000-$9FFF` shows one 8K bank.  The **bank ID** is the current task's `$00`:

| Bank ID | What |
| :------ | :--- |
| `$m0-$mF` (m = `$0-$E`) | Memory card m: 16 banks **for each task** (the card's chips also get `T0-T3`) |
| `$F0-$FF` | Shared RAM: 16 banks, the same for every task, from shared macro-page `U` |

How it's decoded:

* **The module select.**  The bank ID's upper nibble (`RAMB4-7`) goes to a 74LS154 (U23), enabled by the window's
  select (`nBRA_S`: `$8000-$9FFF` while PHI2 is high).  Its outputs `nBRC0-nBRC14` go to the memory card connector
  (J1), one for each module; `nBRC15` selects the shared RAM on the board.
* **The memory card connector.**  The lower nibble and `T` are buffered to it by U21 (74F541), as `RAMB_M0-3` and
  `T_M0-3`, with the read and write strobes (`RWB_M`, `!RWB_M`).  J2 carries D0-D7, A0-A12 and +5 V.
* **A memory card** ([the cards](cards.md#the-memory-daughter-card)) is four HM628512, 2 MB, set to its module number
  by a jumper.  `T_M0-3` go to its chips' A13-A16, so each task has its own 16 banks; `RAMB_M0-1` go to A17-A18; and
  `RAMB_M2-3` pick the chip.  16 tasks x 16 banks x 8K is the card's 2 MB.
* **Shared RAM** is four HM628512 on the main board, U25, U27, U28 and U29 (surface mount, in SOP-32).  `RAMB_M2-3`
  pick the chip, through a 74F139 (U26) enabled by `nBRC15`.  `RAMB_M0-1` go to the chips' A17-A18, and `U0-U3` to
  A13-A16, so the 16 bank IDs `$F0-$FF` are seen in 16 macro-pages (`U` = 0-F): 256 shared banks, 2 MB.  The software
  numbers them **shared bank IDs**, `U << 4 | (bank & $0F)`.

| Bank IDs (`$00`) | Shared RAM chip | Shared bank IDs |
| :--------------- | :-------------- | :-------------- |
| `$F0-$F3` | U25 | `$x0-$x3` for each `U` = x |
| `$F4-$F7` | U28 (the crossed bits 2 and 3) | `$x4-$x7` |
| `$F8-$FB` | U27 (the crossed bits) | `$x8-$xB` |
| `$FC-$FF` | U29 | `$xC-$xF` |

A bank with no chip behind it, a missing memory card, reads `$FF` on 1.8D (the data bus's pull-ups, RN4).  POST
finds the cards that are there (`RAM modules: 03` at boot), and the kernel hands out banks only on those.

The HM628512's pins, for tracing a bad line (POST and the hardware test report bad lines by number):

| Signal | Pin | Signal | Pin |
| :----- | :-- | :----- | :-- |
| A0-A7 | 12, 11, 10, 9, 8, 7, 6, 5 | D0-D2 | 13, 14, 15 |
| A8, A9, A10, A11, A12 | 27, 26, 23, 25, 4 | D3-D7 | 17, 18, 19, 20, 21 |
| A13-A16 (`U0-U3` on shared RAM; `T_M0-3` on a card) | 28, 3, 31, 2 | ~CE, ~OE, ~WE | 22, 24, 29 |
| A17, A18 (`RAMB_M0`, `RAMB_M1`) | 30, 1 | VCC, GND | 32, 16 |

(The SOP-32 package has the same pin numbers as the DIP.)

---

## The paged ROM
*(`$A000-$DFFF`; sheets `BankedROM`, `ZPMirrorRAM`)*

Eight SST39SF040 flash chips (512K each, U30-U37) make 4 MB, seen as 256 banks of 16K.  The bank is the current
task's `$01`:

* `ROMB0-4` go to the chips' A14-A18: 32 banks in each chip.
* `ROMB5-7` pick the chip through a 74F138 (U24), enabled by the window's select (`nBRO_S`: `$A000-$DFFF` while PHI2
  is high).

| Chip select (the 74F138's output) | Chip | Its image (HydraOS's build) |
| :-------------------------------- | :--- | :-------------------------- |
| 0 | U31 | `bin/prom0.bin` |
| 1 | U32 | `bin/prom1.bin` |
| 2 | U34 | `bin/prom2.bin` |
| 3 | U36 | `bin/prom3.bin` |
| 4 | U30 | |
| 5 | U33 | |
| 6 | U35 | |
| 7 | U37 | |

* **The images are the chips' view, not the CPU's.**  Two things on the board shuffle the banks: each bank's two 8K
  halves are swapped (CPU `$A000` reads the chip's offset `$2000` in the bank, and `$C000` its offset `$0000`), and the
  bank number's bits 2 and 3, and 6 and 7, are crossed.  The build allows for both (`base/tools/romimg.js`), so you burn
  each `promN.bin` whole, at offset 0, in the socket the table gives, and never think about it.  [The
  quirks](quirks.md#the-paged-roms-swapped-halves) explain.
* **Only the sockets that are used need chips.**  HydraOS fills four (U31, U32, U34, U36); the base alone fills one
  (U31).  The others can stay empty.
* **A card can take the window over.**  `nBROMD`, a slot pin pulled up on the board, disables the whole paged ROM when
  a card pulls it low, so the card can answer `$A000-$DFFF` itself.
* **The ROMs can't be written in circuit.**  The chips' ~OE is the inverted R/W, and their ~WE is tied high: program
  them in a programmer ([building it](building.md#programming-the-roms)).

---

## The BIOS ROM
*(U6, the root sheet)*

An SST39SF010A in a PLCC-32 socket: 128K, 16 pages of 8K.  (The '020A, 32 pages, and the '040, 64 pages, fit the same
socket.)

* CPU A0-A12 go to the chip's A0-A12, and `W0-W5` to its A13-A18: each value of `W` selects an 8K page.
* It's selected for `$E000-$FEFF` and for `$FFFA-$FFFD`, the NMI and RESET vectors.  The I/O space and the vector RAM
  take the rest of `$FF00-$FFFF`.
* The build's `bin/bios.bin` is 128K, the whole '010A.

**Changing `W` changes the code being run.**  The next instruction is fetched from the new page, at the same
address.  So the kernel keeps identical code at the same address on every page: the COMMON block at `$FD00` (the IRQ
entry and exit, and the far call between pages) and the reset stub at `$E000`.  And since `W` isn't reset, **every
page starts with the reset code**: the RESET vector on every page is `$E000`, which sets `W` to 0.  [The
kernel](../03-base/kernel.md) explains how the pages are used.

---

## I/O space
*(Sheet `AddressDecode`)*

`$FF00-$FFFF` is decoded when A8-A15 are all 1 (U14, a 74F30), while PHI2 is high (`nIO_S`).  A 74LS154 (U19) splits
it by A4-A7 into 16 ports of 16 bytes, `nIOP0_S-nIOP15_S`.  Ports 0-14 are for devices; port 15 is the system port.

| Port | Addresses | Use |
| :--- | :-------- | :-- |
| 0 | `$FF00-$FF0F` | VIA (65C22, U2) |
| 1 | `$FF10-$FF1F` | ACIA (65C51, U3): 4 registers, repeated every 4 bytes |
| 2 | `$FF20-$FF2F` | Slot 0, select A (the Vera X: its registers 0-15) |
| 3 | `$FF30-$FF3F` | Slot 0, select B (the Vera X: its registers 16-31) |
| 4 | `$FF40-$FF4F` | YM2151 (U38): `$FF40` address and status, `$FF41` data (repeated) |
| 5-9 | `$FF50-$FF9F` | Slots 1-5, select A |
| 10-14 | `$FFA0-$FFEF` | Slots 1-5, select B |
| 15 | `$FFF0-$FFFF` | The system port (below) |

**The system port** is decoded by a 74F138 (U20) on A0-A2, enabled by `nIOP15_S` and by A3 differing from A2 (U12):

| Address | Select | Use |
| :------ | :----- | :-- |
| `$FFF0-$FFF3` | `nIOB0-3` | `T`, `U`, `V`, `W` |
| `$FFF4-$FFF9` | none | Not decoded (reserved) |
| `$FFFA-$FFFD` | (the BIOS ROM) | The NMI and RESET vectors, from the current BIOS page |
| `$FFFE`, `$FFFF` | `nIOB14`, `nIOB15` | The vector RAM, low and high bytes |

Each slot's port selects (`nIOA_S`, `nIOB_S`) and its two IRQ lines are listed with [the slots](devices.md#the-expansion-slots).
A card can also decode addresses of its own from the whole I/O page, with `nIO_S` and A0-A7.

---

## Interrupts
*(Sheet `IRQ_P_E`, the IRQ priority encoder)*

Sixteen active-low IRQ lines, `nIRQ0-nIRQ15`, each pulled up (RN2, RN3).  Line 0 has the highest priority.

| Line | Source | Line | Source |
| ---: | :----- | ---: | :----- |
| 0 | VIA | 8 | Slot 4, A |
| 1 | ACIA | 9 | Slot 5, A |
| 2 | Slot 0, A (the Vera X) | 10 | Slot 1, B |
| 3 | Slot 0, B | 11 | Slot 2, B |
| 4 | YM2151 | 12 | Slot 3, B |
| 5 | Slot 1, A | 13 | Slot 4, B |
| 6 | Slot 2, A | 14 | Slot 5, B |
| 7 | Slot 3, A | 15 | Not connected: the software interrupt |

**How it works:**

* Two cascaded 74LS148 priority encoders (U17 for lines 0-7, U18 for 8-15) find the highest-priority active line.
* Any active line asserts the CPU's ~IRQ (through U44, then U22).  The IRQ input is level-triggered: a device holds
  its line low until its handler clears the cause.
* A 74F157 (U45) forms a 4-bit index, `q0-q3`: the encoders' output while any IRQ line is active, `V0-V3` otherwise.
* The index addresses the **vector RAM**, four 74LS219 (IC5-IC8): a 16-entry table of 16-bit vectors.  The CPU reads
  its IRQ/BRK vector from `$FFFE/$FFFF`, so it gets the entry for the active line: each line has its own handler.

```
 nIRQ0-7 --> U17 '148 --+
                        +--> index q0-q3 --> U45 '157 --> IC5-IC8 vector RAM --> D0-D7 at $FFFE/$FFFF
 nIRQ8-15 -> U18 '148 --+                     ^
                                    V0-V3 ----+ (when no line is active)
```

**The index is the line number XOR 7.**  The '148s encode active-low inputs with the highest-priority input as 7, so
line n gives index `n ^ 7`: line 0 is entry 7, line 7 entry 0, line 8 entry 15, line 15 entry 8.  `hw.inc`'s
`IRQ_INDEX(line)` is `line ^ 7` for this reason.

**Writing a vector.**  A write to `$FFFE` or `$FFFF` stores the low or high byte of entry `q`, which is `V0-V3` when
no IRQ line is active.  So: interrupts off, set `V` to the entry, write the two bytes.  The kernel's boot does this
once (`base/kernel/reset.s`): every line's entry is the kernel's one IRQ path, which then calls the line's driver.

**BRK.**  `BRK` also reads `$FFFE/$FFFF`, and with no IRQ line active that's entry `V0-V3`.  The kernel leaves `V` at
line 15's entry (line 15 has no hardware), so a `BRK` comes to the kernel by that vector: a program's stray `BRK`
is a note to it, and the debugger's breakpoints are `BRK`s.

**The lines can't be masked.**  There's no mask register: a line held low keeps interrupting until the device lets it
go.  So a driver must quiet its device in its handler, and a device that can only be quieted slowly (an I2C device,
say) mustn't be given a line.

**NMI** (`NMIB`, pulled up, on every slot) uses the BIOS ROM's `$FFFA` vector.  Nothing on the board drives it.

---

## Clocks
*(Sheet `Clocks`)*

A 14.31818 MHz crystal (Y1) with a 74ACT14 (U39) oscillator, divided by a 74F191 counter (U40):

| Clock | Frequency | Goes to |
| :---- | :-------- | :------ |
| `HS_CLK` | 14.318 MHz | The slots |
| Q0 | 7.159 MHz | CPU clock jumper J8 |
| `SND_CLK` (Q1) | 3.580 MHz | The YM2151's clock; the slots; CPU clock jumper J7 |
| `SER_CLK` (Q2) | 1.790 MHz | The ACIA's clock (XTAL1); the slots; CPU clock jumper J6 |
| Q3 | 0.895 MHz | CPU clock jumper J5 |

* **The CPU clock** (`PHI2`) is whichever of J5-J8 is fitted: fit exactly one.  The software is built for **J7,
  3.58 MHz**, unless you build it for 7.16 MHz (`node build.js --clock 2`, with J8).  0.89 and 1.79 MHz work
  electrically, but the software's timing (the tick, the serial port) isn't built for them.
* **At 7.16 MHz the YM2151 is too slow for the bus**, and the board has no wait states: don't use the sound chip at
  that speed.
* **DMA stops the clock.**  The CPU's PHI0 input is `PHI2` gated by `DMAB` (U11), so a card pulling `DMAB` low
  stops the CPU (the W65C02S is fully static).
* **`PHI1`**, the inverse of `PHI2` (U22), qualifies the address decoding: every select is only active while PHI2 is
  high.
* **1.8D's terminations.**  Each clock leaves its driver through a 22 ohm resistor (R23-R27): a clock's driver is the
  net `X_SRC`, and the board and the slots see `X`.  R22 (330 ohms) limits the crystal's drive.

---

## Reset, power and the bus

**Power** comes from an ATX-24 connector (J11):

* +5 V for the logic, +3.3 V, and +12 V and -12 V for the audio op-amps and the slots.
* +5 V standby is brought out as `+5VA`.
* The slots have a -5 V pin, fed from ATX pin 20 (1.8D): older ATX 1.x supplies have -5 V there; newer ones leave it
  unconnected.  Don't count on it.

**The front panel** (J10, 2 x 6):

| Pin | Signal |
| :-- | :----- |
| 7 | `RESB`: a push button from here to ground resets the machine |
| 8 | +5 V |
| 11 | ATX `PS_ON#`: a switch from here to ground turns the supply on |
| 12 | The power LED's anode, through R2 (470 ohms) from +5 V (1.8D); its cathode to ground |
| 5, 6, 9, 10 | Ground |
| 1-4 | Not connected |

**Reset** (`RESB`):

* A DM1813 supervisor (U43) holds reset low at power-up and during brown-outs.
* The front panel's button and the slots (open-collector) can pull it low too.
* It resets the CPU, the VIA, the ACIA, the YM2151 and the YM3012.  It **doesn't** reset the pseudo-registers, the
  bank registers or any RAM: the boot code sets those.

**The bus:**

* **`RDY`** is pulled up by R28 (10K).  The W65C02S drives RDY low itself while it waits in `WAI`, so RDY must never be
  driven high directly.  (1.8C had a jumper, J4, for the pull-up; 1.8D has the resistor instead.)
* **`BE`**, the bus enable, is pulled up, and on the slots.
* **DMA:** a card pulls **`DMAB`** low to take the bus.  That stops the CPU's clock and turns off the address buffers
  (U58, U60, 74F541), the data transceiver (U59, 74ACT245) and the R/W buffer (U61, 74F125); the card then drives the
  address and data buses itself.  Nothing uses DMA yet.
* **The data bus's pull-ups** (RN4, 10K, 1.8D) make a read that nothing answers, an empty slot or an unused port, give
  `$FF`.
* **`SYNC`**, the CPU's opcode-fetch signal, is on the slots, for single-stepping hardware or a logic analyser.
* `MLB` and `VPB` aren't used.  `SOB` is tied high.

---

## The chips by function

| Function | Parts |
| :------- | :---- |
| CPU | U1 W65C02S |
| Bus buffers | U58, U60 74F541 (address); U59 74ACT245 (data); U61 74F125 (R/W) |
| Address decoding | U8, U9 74LS260; U10 74F20; U11 74ACT08; U12 74F86; U13, U16 74F32; U14 74F30; U15 74F00; U19 74LS154 (the I/O ports); U20 74F138 (the system port); U21 74F541 (the memory card buffer); U22 74ACT240 |
| Pseudo-registers | U48, U50, U52, U54 74F573; U49, U51, U53, U55 74F541; U56 74F32; U57 74F02 |
| Bank registers | IC1-IC4 74LS219; U23 74LS154 (the module select); U24 74F138 (the paged ROM's chip select) |
| Interrupts | U17, U18 74LS148; U44 74F00; U45 74F157; IC5-IC8 74LS219 (the vector RAM); RN2, RN3 pull-ups |
| Task RAM | U7 HM628512 (or DS1747) |
| Shared RAM | U25, U27, U28, U29 HM628512 (SOP-32); U26 74F139 |
| Paged ROM | U30-U37 SST39SF040 |
| BIOS ROM | U6 SST39SF010A (PLCC-32) |
| Clocks | Y1 14.31818 MHz; U39 74ACT14; U40 74F191 |
| Reset | U43 DM1813 |
| VIA, SPI | U2 R65C22; U4 74F138 (the SPI device select) |
| Serial | U3 R65C51; U5 MAX232 |
| Sound | U38 YM2151; U41 YM3012; U42 TL074; IC9 NJM4556A (the mixer) |

[The parts](parts.md) has every part, with its value and package.
