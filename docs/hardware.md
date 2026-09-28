## **Hydra-16 Hardware Reference**

This is the Hydra-16 main board (V1) as its schematic describes it (`board/hydra-16.kicad_sch` and its sheets), with the two companion cards in `board/`.  Reference designators (U25, J18, ...) are the schematic's.  For how the software uses the hardware, see the [Programmer's Guide](programming/README.md).

### **Contents**
1. [Overview](#overview)
2. [The CPU view: memory map](#the-cpu-view-memory-map)
3. [The pseudo-registers T, U, V, W](#the-pseudo-registers-t-u-v-w)
4. [Task RAM and the bank registers ($00, $01)](#task-ram-and-the-bank-registers)
5. [The paged RAM window ($8000-$9FFF)](#the-paged-ram-window)
6. [The paged ROM ($A000-$DFFF)](#the-paged-rom)
7. [The BIOS ROM ($E000-$FFFF)](#the-bios-rom)
8. [I/O space ($FF00-$FFFF)](#io-space)
9. [Interrupts](#interrupts)
10. [Clocks](#clocks)
11. [Reset, power and bus control](#reset-power-and-bus-control)
12. [On-board devices](#on-board-devices): VIA, ACIA (serial), YM2151 (sound), SPI
13. [Expansion slots](#expansion-slots)
14. [Companion cards](#companion-cards): the memory daughter card, the bus breakout card
15. [Connectors and jumpers](#connectors-and-jumpers)
16. [V1 errata](#v1-errata)
17. [Parts by function](#parts-by-function)

---

### **Overview**

The Hydra-16 is a W65C02S computer built for multitasking.  Its address decoding gives each of **16 tasks** its own 32K of RAM, including zero page and the stack, and its own RAM and ROM bank selections.  So switching tasks is a single write to one register, `T`.

| | |
| :-- | :-- |
| **CPU** | WDC W65C02S (U1), 3.58 MHz as built (jumper: 0.89, 1.79, 3.58 or 7.16 MHz) |
| **Task RAM** | 512K (U7, HM628512): 32K at `$0000-$7FFF` for each of 16 tasks |
| **Paged RAM** | Seen through an 8K window at `$8000-$9FFF`.  **Task banks:** up to 15 modules of 2 MB on memory daughter cards; each task sees 16 banks of 8K per module.  **Shared banks:** 2 MB on the board (U25, U27-U29), the same for all tasks: 256 banks of 8K, 16 at a time |
| **Paged ROM** | 4 MB (U30-U37, 8 x SST39SF040), seen in 16K banks at `$A000-$DFFF` |
| **BIOS ROM** | U6, SST39SF010/020/040 (128-512K): 8K pages at `$E000-$FFFF`, selected by `W` |
| **I/O** | 15 ports of 16 bytes at `$FF00-$FFEF`, plus the system port at `$FFF0-$FFFF` |
| **Interrupts** | 16 prioritised IRQ lines, each with its own vector (a 16-entry vector RAM at `$FFFE`) |
| **Devices** | 65C22 VIA (timers, SPI, I2C, GPIO), 65C51 ACIA (RS-232 on a DE-9), YM2151 + YM3012 (stereo FM sound) |
| **Expansion** | 6 slots (62-pin edge connectors, 8-bit bus), 8 SPI device headers, a GPIO/I2C header, memory card connectors |
| **Power** | ATX-24 supply (+5 V, +3.3 V, +12 V, -12 V) |

---

### **The CPU view: memory map**

What the CPU sees at each address, and what selects it:

| Addresses | What | Selected by |
| :-------- | :--- | :---------- |
| `$0000` | RAM bank register (writes); task RAM (reads) | per task (see below) |
| `$0001` | ROM bank register (writes); task RAM (reads) | per task |
| `$0002-$7FFF` | Task RAM (zero page, stack, program RAM) | `T` |
| `$8000-$9FFF` | Paged RAM: a task bank (`$00-$EF`) or a shared bank (`$F0-$FF`) | `$00`; `T` for task banks, `U` for shared banks |
| `$A000-$DFFF` | Paged ROM, 16K banks | `$01` |
| `$E000-$FEFF` | BIOS ROM | `W` |
| `$FF00-$FFEF` | I/O ports 0-14, 16 bytes each | |
| `$FFF0-$FFF3` | Pseudo-registers `T`, `U`, `V`, `W` | |
| `$FFF4-$FFF9` | Not decoded (reads float) | |
| `$FFFA-$FFFD` | BIOS ROM: the NMI and RESET vectors | `W` |
| `$FFFE-$FFFF` | IRQ/BRK vector RAM (16 entries) | the highest-priority active IRQ, or `V` |

Every task has its own `$0000-$7FFF` and its own `$00`/`$01` values.  All tasks share the I/O space, the shared RAM banks, the ROMs, and the pseudo-registers themselves.

---

### **The pseudo-registers T, U, V, W**
*(Sheet `FFF_Registers`)*

Four 8-bit registers in the system port.  Each is a 74F573 latch, written with the CPU's write strobe, and read back through a 74F541 buffer.  So a read gives the whole byte last written, all 8 bits.

| Address | Register | Latch / read-back | Bits used by the hardware |
| :------ | :------- | :---------------- | :------------------------ |
| `$FFF0` | `T`, task | U48 / U49 | T0-T3: which task's RAM (task RAM A15-A18; the modules' task lines) |
| `$FFF1` | `U`, shared macro-page | U50 / U51 | U0-U3: shared RAM A13-A16 |
| `$FFF2` | `V`, vector select | U52 / U53 | V0-V3: the vector RAM index when no IRQ is active.  V4-V7 aren't wired: the software keeps a software interrupt number there |
| `$FFF3` | `W`, BIOS ROM page | U54 / U55 | W0-W5: BIOS ROM A13-A18 (64 pages with an SST39SF040; 16 with the '010) |

The latches have no reset, so they power up random; the reset code sets them.  Nothing on the board resets them when the reset button is pressed.  Bits 4-7 of `T` and `U`, and 6-7 of `W`, are stored but go nowhere.

---

### **Task RAM and the bank registers**

**Task RAM** is one HM628512 (U7, 512K) on the main board.  CPU A0-A14 go straight to it, CPU A15 is its chip enable (low for `$0000-$7FFF`), and `T0-T3` drive its A15-A18.  So each value of `T` selects a different 32K, and a task switch is a single write to `$FFF0`.  Zero page and the stack page are part of it, so every task has its own zero page and stack.

**The bank registers `$00` and `$01`** *(sheet `ZPMirrorRAM`)* are four 74LS219 (16 x 4-bit RAM) chips, addressed by `T0-T3`:

| Register | Chips | Drives |
| :------- | :---- | :----- |
| `$00`: RAM bank | IC1 (bits 0-3), IC2 (bits 4-7) | `RAMB0-7`: which bank is at `$8000-$9FFF` |
| `$01`: ROM bank | IC3 (bits 0-3), IC4 (bits 4-7) | `ROMB0-7`: which paged ROM bank is at `$A000-$DFFF` |

* A write to `$0000` or `$0001` stores the byte in the current task's entry.  The address decoding (U8-U10 on sheet `AddressDecode`) spots a write to `$000x`, and U16 picks the register with A0.
* The write also goes to task RAM as usual, and a **read** of `$00`/`$01` comes from task RAM: the registers themselves are write-only, and task RAM mirrors them.
* The outputs are always enabled, so the bank lines follow `T` at once.  A task switch selects the new task's RAM bank and ROM bank without further writes.
* On V1, bits 2 and 3 of both registers are crossed on their way to the bank lines (see [V1 errata](#v1-errata)).

---

### **The paged RAM window**
*(`$8000-$9FFF`; sheets `ZPMirrorRAM`, `SharedMem`, `AddressDecode`)*

`$8000-$9FFF` shows one 8K bank.  The **bank ID** is the current task's `$00` value:

| Bank ID | What |
| :------ | :--- |
| `$m0-$mF` (m = `$0-$E`) | Task RAM module m: 16 banks for **each task** (the module's chips also get `T0-T3`) |
| `$F0-$FF` | Shared RAM: 16 banks, the same for every task, from shared macro-page `U` |

* **Module select.** The bank ID's upper nibble (`RAMB4-7`) goes to a 74LS154 (U23), qualified by the window's select (`nBRA_S`, `$8000-$9FFF` in PHI2).  Its outputs `nBRC0-nBRC14` go to the memory card connector (J1), one per module; `nBRC15` selects the shared RAM on the board.
* **Memory card connector.** The lower nibble and `T` are buffered to it by U21 (74F541), as `RAMB_M0-3` and `T_M0-3`, along with the read/write strobes (`RWB_M`, `!RWB_M`).  J2 carries D0-D7, A0-A12 and +5 V.
* **Task RAM modules** are [memory daughter cards](#memory-daughter-card): 4 x HM628512 per card, set to a module number with a jumper.  In a module:
  * `T_M0-3` go to the chips' A13-A16, so each task has its own 16 banks.
  * `RAMB_M0-1` go to A17-A18.
  * `RAMB_M2-3` pick the chip.
  * A module is 2 MB: 16 tasks x 16 banks x 8K.
* **Shared RAM** is four HM628512 on the main board: U25, U27, U28 and U29.
  * `RAMB_M2-3` pick the chip, through a 74F139 (U26) enabled by `nBRC15`.
  * `RAMB_M0-1` go to the chips' A17-A18, and `U0-U3` to A13-A16.
  * So the 16 bank IDs `$F0-$FF` are seen in 16 macro-pages (`U` = 0-F): 256 shared banks of 8K, 2 MB.
  * The software numbers them as **shared bank ID** = `U << 4 | (bank & $0F)`.

| Bank IDs (`$00`) | Shared RAM chip (V1) | Shared bank IDs |
| :--------------- | :------------------- | :-------------- |
| `$F0-$F3` | U25 | `$x0-$x3` for each `U` = x |
| `$F4-$F7` | U28 (V1: bits 2/3 crossed) | `$x4-$x7` |
| `$F8-$FB` | U27 (V1) | `$x8-$xB` |
| `$FC-$FF` | U29 | `$xC-$xF` |

A bank with no chip behind it (a missing module) reads whatever floats on the bus; the ROM's POST and module probe detect this.

HM628512 pins, for tracing a bad line (the POST prints bad lines by number):

| Signal | Pin | Signal | Pin |
| :----- | :-- | :----- | :-- |
| A0-A7 | 12, 11, 10, 9, 8, 7, 6, 5 | D0-D2 | 13, 14, 15 |
| A8, A9, A10, A11, A12 | 27, 26, 23, 25, 4 | D3-D7 | 17, 18, 19, 20, 21 |
| A13-A16 (`U0-U3` on shared RAM) | 28, 3, 31, 2 | ~CE, ~OE, ~WE | 22, 24, 29 |
| A17, A18 (`RAMB_M0`, `RAMB_M1`) | 30, 1 | VCC, GND | 32, 16 |

---

### **The paged ROM**
*(`$A000-$DFFF`; sheets `BankedROM`, `ZPMirrorRAM`)*

Eight SST39SF040 (512K each, U30-U37) give 4 MB, seen as 256 banks of 16K.  The bank is the current task's `$01` value:

* `ROMB0-4` go to the chips' A14-A18 (32 banks per chip).
* `ROMB5-7` pick the chip through a 74F138 (U24), qualified by the window's select (`nBRO_S`: `$A000-$DFFF` in PHI2):

| `$01` bits 5-7 | Chip | Banks |
| :------------- | :--- | :---- |
| 0 | U31 | `$00-$1F` |
| 1 | U32 | `$20-$3F` |
| 2 | U34 | `$40-$5F` |
| 3 | U36 | `$60-$7F` |
| 4 | U30 | `$80-$9F` |
| 5 | U33 | `$A0-$BF` |
| 6 | U35 | `$C0-$DF` |
| 7 | U37 | `$E0-$FF` |

* **The halves are swapped.** CPU A13 goes to the chips' A13 unchanged, but in the window `$A000-$BFFF` has A13 = 1 and `$C000-$DFFF` has A13 = 0.  So CPU `$A000` reads chip offset `$2000` of the bank, and `$C000` reads offset `$0000`.  The build writes `paged_rom_C02.bin` in chip order for this, so burn it at offset 0 of U31.
* **`nBROMD`** (a slot pin, pulled up) disables the whole paged ROM when a card pulls it low, so the card can answer `$A000-$DFFF` itself.
* The chips' ~OE is the inverted R/W; there's no write path in circuit (program the chips in a programmer).

---

### **The BIOS ROM**
*(U6, root sheet)*

An SST39SF0x0 in a 32-pin socket: the '010 (128K, 16 pages), '020 (256K, 32 pages) or '040 (512K, 64 pages).

* CPU A0-A12 go to the chip's A0-A12, and `W0-W5` to its A13-A18: each `W` value selects an 8K page.
* It's selected for `$E000-$FEFF` and for `$FFFA-$FFFD` (the NMI and RESET vectors).  I/O space and the vector RAM take the rest of `$FF00-$FFFF`.
* The build produces `os_rom_C02.bin`, 128K, for the '010.

**Changing `W` changes the code being run.**  The next instruction is fetched from the new page, at the same address.  The software handles this by keeping identical code at the same address on every page: the COMMON block at `$FD00`, and the reset entry at `$E000` (see [ROM layout](programming/rom-layout.md)).  Because `W` isn't reset, **every page must start with the reset code**: the RESET vector on every page points to `$E000`, which sets `W` to 0.

---

### **I/O space**
*(Sheet `AddressDecode`)*

`$FF00-$FFFF` is decoded when A8-A15 are all 1 (U14, a 74F30), qualified by PHI2 (`nIO_S`).  A 74LS154 (U19) splits it by A4-A7 into 16 ports of 16 bytes, `nIOP0_S-nIOP15_S`.  Ports 0-14 are for devices; port 15 is the system port.

| Port | Addresses | Use |
| :--- | :-------- | :-- |
| 0 | `$FF00-$FF0F` | VIA (65C22, U2) |
| 1 | `$FF10-$FF1F` | ACIA (65C51, U3): 4 registers, repeated every 4 bytes |
| 2 | `$FF20-$FF2F` | Slot 0, select A (intended for video) |
| 3 | `$FF30-$FF3F` | Slot 0, select B (intended for video) |
| 4 | `$FF40-$FF4F` | YM2151 (U38): `$FF40` address/status, `$FF41` data (repeated) |
| 5-9 | `$FF50-$FF9F` | Slots 1-5, select A |
| 10-14 | `$FFA0-$FFEF` | Slots 1-5, select B |
| 15 | `$FFF0-$FFFF` | System port (below) |

**The system port** is decoded by a 74F138 (U20) on A0-A2, enabled by `nIOP15_S` and by A3 differing from A2 (U12):

| Address | Select | Use |
| :------ | :----- | :-- |
| `$FFF0-$FFF3` | `nIOB0-3` | `T`, `U`, `V`, `W` |
| `$FFF4-$FFF9` | none | Not decoded (reads float; reserved) |
| `$FFFA-$FFFD` | (BIOS ROM) | NMI and RESET vectors, from the current BIOS ROM page |
| `$FFFE`, `$FFFF` | `nIOB14`, `nIOB15` | The vector RAM, low and high bytes |

Slot port selects (`nIOA_S`, `nIOB_S` on each slot) and each slot's two IRQ lines follow the same numbering: see [Expansion slots](#expansion-slots).  A card can also decode its own addresses from the whole I/O space with `nIO_S` and A0-A7.

---

### **Interrupts**
*(Sheet `IRQ_Priorty_Encoder`)*

Sixteen active-low IRQ lines, `nIRQ0-nIRQ15`, each pulled up (RN2, RN3).  Line 0 has the highest priority.

| Line | Source | Line | Source |
| ---: | :----- | ---: | :----- |
| 0 | VIA | 8 | Slot 4, A |
| 1 | ACIA | 9 | Slot 5, A |
| 2 | Slot 0, A | 10 | Slot 1, B |
| 3 | Slot 0, B | 11 | Slot 2, B |
| 4 | YM2151 | 12 | Slot 3, B |
| 5 | Slot 1, A | 13 | Slot 4, B |
| 6 | Slot 2, A | 14 | Slot 5, B |
| 7 | Slot 3, A | 15 | Not connected: used for software interrupts |

**How it works:**
* Two cascaded 74LS148 priority encoders (U17 for lines 0-7, U18 for 8-15) find the highest-priority active line.
* Any active line asserts the CPU's ~IRQ (U44, then U22).  The IRQ input is level-triggered: a device holds its line until the handler clears it.
* A 74F157 (U45) forms a 4-bit index `q0-q3`:
  * while any IRQ line is active, the index is the encoder's output;
  * otherwise it's `V0-V3`.
* The index addresses the **vector RAM**, four 74LS219 (IC5-IC8), a 16-entry table of 16-bit vectors.  The CPU reads its IRQ/BRK vector from `$FFFE/$FFFF`, so it gets the entry for the active line, and each line has its own handler.

**The index is the line number XOR 7.**  The '148s encode active-low inputs with the highest-priority input as 7, so line n gives index `n ^ 7`: line 0 is entry 7, line 7 entry 0, line 8 entry 15, line 15 entry 8.  The ROM's `IRQ_NUMBER(n)` macro is `n ^ 7` for this reason.

**Writing vectors.**  A write to `$FFFE` / `$FFFF` stores the low / high byte of entry `q`.  That's `V0-V3` when no IRQ line is active.  So set `V` to the entry wanted, then write the vector, with no IRQ pending.  The ROM does this once at boot with interrupts off.

**BRK and software interrupts.**  `BRK` also reads `$FFFE/$FFFF`.  With no IRQ line active, that's entry `V0-V3`.  So a software interrupt is: set `V` to `IRQ_NUMBER(15)` (line 15 has no hardware), then `brk`.  The ROM keeps the software interrupt number in `V4-V7`.

**NMI** (`NMIB`, pulled up, on every slot) uses the ROM's `$FFFA` vector.  Nothing on the board drives it.

---

### **Clocks**
*(Sheet `Clocks`)*

A 14.31818 MHz crystal (Y1) with a 74ACT14 (U39) oscillator, divided by a 74F191 counter (U40):

| Clock | Frequency | Goes to |
| :---- | :-------- | :------ |
| `HS_CLK` | 14.318 MHz | Slots (e.g. for video) |
| Q0 | 7.159 MHz | CPU clock jumper J8 |
| `SND_CLK` (Q1) | 3.580 MHz | YM2151 master clock; slots; CPU clock jumper J7 |
| `SER_CLK` (Q2) | 1.790 MHz | ACIA clock (XTAL1); slots; CPU clock jumper J6 |
| Q3 | 0.895 MHz | CPU clock jumper J5 |

**The CPU clock** (`PHI2`) is whichever of J5-J8 is fitted: fit exactly one.  As built, it's **J7, 3.58 MHz**.  The CPU's PHI0 input is `PHI2` gated by `DMAB` (U11), so a card asserting `DMAB` stops the CPU clock (the W65C02S is fully static).  `PHI1` (inverted `PHI2`) qualifies the address decoding.

The ROM's timing (the scheduler's tick, sound note lengths, serial timeouts) is built for one clock: `CPU_CLOCK_MULT` in `os_rom/include/hw.inc` (1 = 3.58 MHz, 2 = 7.16 MHz).  At 7.16 MHz the YM2151 is too slow for the bus: there are no wait states on V1, so don't use the sound chip at that speed.  The 0.89 and 1.79 MHz settings aren't supported by the ROM's timing.

---

### **Reset, power and bus control**

**Power** comes from an ATX-24 connector (J11):
* +5 V for the logic, +3.3 V, and ±12 V for the audio op-amps and the slots.
* +5 V standby is brought out as `+5VA`.
* The slots also have a -5 V pin, which the board doesn't supply: ATX no longer has -5 V.

**Front panel** (J10, 2x6):

| Pin | Signal |
| :-- | :----- |
| 7 | `RESB`: a push button from here to ground (pins 5, 6, 9 or 10) resets the machine |
| 8 | +5 V |
| 11 | ATX `PS_ON#`: connect to ground (a latching switch) to turn the supply on |
| 12 | Power LED: from the supply's `PWR_OK`, through R2 (470 Ω) |
| 5, 6, 9, 10 | Ground |
| 1-4 | Not connected |

**Reset** (`RESB`):
* A DM1813 supervisor (U43) holds reset low at power-up and during brown-outs.
* The front panel button and the slots (open-collector) can pull it low too.
* It resets the CPU, VIA, ACIA, YM2151 and YM3012.  It doesn't reset the pseudo-registers, the bank registers or RAM.

**Bus control:**
* **`RDY`:** pulled up through jumper **J4**, which must be fitted.  The W65C02S drives RDY low itself during `WAI`, so RDY must never be driven high directly.
* **`BE`** (bus enable): pulled up, and on the slots.
* **DMA:** a card pulls **`DMAB`** low to take the bus.
  * It stops the CPU clock, and turns off the address buffers (U58, U60, 74F541), the data transceiver (U59, 74F245) and the R/W buffer (U61, 74F125).
  * The card then drives the address and data buses itself.
* **`SYNC`** from the CPU is on the slots, for single-stepping or debugging hardware.
* `MLB`, `VPB` are unused.  `SOB` is tied high.

---

### **On-board devices**

#### **VIA (65C22, U2): port 0, IRQ line 0**
*(Registers at `$FF00-$FF0F`.)*

**Port A** is general purpose I/O, on header **J27** (2x6), with ESD protection (J28, SP720):

| J27 pin | Signal | J27 pin | Signal |
| :------ | :----- | :------ | :----- |
| 1 | GND | 2 | +5 V |
| 3 | PA0: `I2C_SCL` | 4 | PA1: `I2C_SDA` |
| 5 | PA2 | 6 | PA3 |
| 7 | PA4 | 8 | PA5 |
| 9 | PA6 | 10 | PA7 |
| 11 | CA1 | 12 | CA2 |

* PA0/PA1 are the I2C bus (bit-banged; the SDA and SCL pull-ups are in RN1), which also goes to every slot.  No I2C driver exists yet.
* **Port B is the SPI bus** (below).
* **Timer 1** is the scheduler's tick: free-running, 200 interrupts a second.
* **Timer 2** paces serial output when the ROM is built for a WDC ACIA.
* CB1/CB2 aren't brought out.

#### **ACIA (65C51, U3): port 1, IRQ line 1**
*(Registers at `$FF10-$FF13`.)*

**Chip and clock:**
* The board has a Rockwell R65C51 socket.  A WDC W65C51N works if the ROM is built for it: its transmitter status and interrupt don't work, so the ROM paces sending with VIA timer 2 (`SER_ACIA` in `hw.inc`).
* **The ACIA's clock is `SER_CLK`, 1.790 MHz**, not the 1.8432 MHz its baud rate table is made for.  So every rate is 2.9% slow (9600 gives about 9,320 baud), which terminals and USB serial adapters accept.
* DCD and DSR are tied active.

**Serial settings:** 9600 baud, 8 data bits, no parity, 1 stop bit at boot, with RTS/CTS.  The ROM can change the rate (300 to 19200, and 115200: the ACIA clock / 16), the data bits (5-8), the parity and the stop bits afterwards (`/dev/ser/ctl`, [io.md](programming/io.md#the-serial-port-settings)).

**The DE-9 connector** (J3, male) is driven by a MAX232 (U5), and wired like a modem (DCE):

| DE-9 pin | Signal | Direction |
| :------- | :----- | :-------- |
| 2 | Hydra transmits (ACIA TxD) | out |
| 3 | Hydra receives (ACIA RxD) | in |
| 5 | Ground | |
| 7 | ACIA ~CTS (from the terminal's RTS) | in |
| 8 | ACIA ~RTS (to the terminal's CTS) | out |

**Cable:** a PC or USB serial adapter connects with a **straight-through** cable with two female ends, not a null-modem cable.  The ACIA only transmits while its ~CTS is asserted, so the cable must carry pin 7.  Terminal programs assert RTS by default.

#### **YM2151 sound (U38): port 4, IRQ line 4**
*(Sheet `Sound`.)*

**The chip:**
* A Yamaha YM2151 (OPM, 8 FM channels), clocked by `SND_CLK` (3.58 MHz).
* `$FF40` selects a register (and reads the status: bit 7 = busy), and `$FF41` writes it.
* After a data write the chip is busy for 64 of its clocks (about 18 µs); writes while it's busy are lost.
* **CT1/CT2**, its two general-purpose outputs, are on header J9.

**The audio path:**
* A YM3012 DAC (U41) converts its serial output, buffered and filtered by a TL074 (U42).
* A mixer (IC9, LF353, ±12 V) adds, for each channel:
  * the YM3012's output;
  * each slot's audio lines (`SND_CL0-5`, `SND_CR0-5`), so a sound card in a slot is mixed in;
  * a line input on header J29.
* **Output:** stereo, on the 3.5 mm jack J26.  The schematic has the tip on the right channel and the ring on the left, the reverse of the usual convention (see [V1 errata](#v1-errata)).

#### **SPI bus (VIA port B)**
SPI is bit-banged on the VIA's port B; a 74F138 (U4) decodes the device select:

| Port B bit | Signal |
| :--------- | :----- |
| PB0 | `SPI_CLK` (SCLK) |
| PB1 | `nSPI_CS`: low selects the device given by PB3-PB6 |
| PB2 | `SPI_MOSI` |
| PB3-PB5 | `SPI_A0-A2`: device 0-7 |
| PB6 | `SPI_A3`: 0 = the board's devices 0-7 (U4); 1 = devices 8-15, for cards to decode (the slots carry `nSPI_CS` and `SPI_A0-A3`) |
| PB7 | `SPI_MISO` |

**The eight SPI device headers** are J18-J25, for devices 0-7.  Each is a 1x6 header:

| Pin | Signal |
| :-- | :----- |
| 1 | ~CS |
| 2 | SCLK |
| 3 | MOSI |
| 4 | MISO |
| 5 | +5 V |
| 6 | GND |

The ROM runs SPI in mode 0, and its storage server serves an SD card on any device (`/dev/sd/N`); device 0 (J18) is the usual place for an SD card adapter.  The headers supply +5 V, so the adapter must regulate and level-shift to 3.3 V for the card (common SD card modules do).

---

### **Expansion slots**
*(Sheet `Connectors`: J12-J17, "Hydra bus 8-bit", 62-pin card edge.)*

Six slots, all carrying the same bus except for each slot's two port selects, two IRQ lines and its audio pair:

| Slot | Connector | `nIOA_S` (pin 51) | `nIOB_S` (pin 49) | `IRQA` (pin 55) | `IRQB` (pin 53) | Audio (`SND_CL/CR`) |
| :--- | :-------- | :---------------- | :---------------- | :-------------- | :-------------- | :------------------ |
| 0 | J12 | port 2 `$FF20` | port 3 `$FF30` | line 2 | line 3 | pair 0 |
| 1 | J13 | port 5 `$FF50` | port 10 `$FFA0` | line 5 | line 10 | pair 1 |
| 2 | J14 | port 6 `$FF60` | port 11 `$FFB0` | line 6 | line 11 | pair 2 |
| 3 | J15 | port 7 `$FF70` | port 12 `$FFC0` | line 7 | line 12 | pair 3 |
| 4 | J16 | port 8 `$FF80` | port 13 `$FFD0` | line 8 | line 13 | pair 4 |
| 5 | J17 | port 9 `$FF90` | port 14 `$FFE0` | line 9 | line 14 | pair 5 |

Slot 0's A lines (port 2, IRQ 2) have the highest priority of the slots; the B lines of slots 1-5 the lowest.

**Pinout** (the same on every slot; `~` = active low):

| Pin | Signal | Pin | Signal |
| :-- | :----- | :-- | :----- |
| 1 | GND | 2 | ~`nBROMD` (disable the paged ROM) |
| 3 | ~`RESB` | 4 | D7 |
| 5 | +5 V | 6 | D6 |
| 7 | ~`NMIB` | 8 | D5 |
| 9 | -5 V (not supplied) | 10 | D4 |
| 11 | `BE` | 12 | D3 |
| 13 | -12 V | 14 | D2 |
| 15 | `SER_CLK` (1.79 MHz) | 16 | D1 |
| 17 | +12 V | 18 | D0 |
| 19 | GND | 20 | `RDY` |
| 21 | R/~W | 22 | `SND_CLK` (3.58 MHz) |
| 23 | ~`DMAB` | 24 | `SND_CR` (audio in, right) |
| 25 | ~`nIO_S` (any I/O port) | 26 | `SND_CL` (audio in, left) |
| 27 | `SYNC` | 28 | `I2C_SDA` |
| 29 | `SPI_SCLK` | 30 | `I2C_SCL` |
| 31 | `SPI_MOSI` | 32 | A15 |
| 33 | `SPI_MISO` | 34 | A14 |
| 35 | ~`nSPI_CS` | 36 | A13 |
| 37 | `PHI1` | 38 | A12 |
| 39 | `PHI2` | 40 | A11 |
| 41 | `SPI_A3` | 42 | A10 |
| 43 | `SPI_A2` | 44 | A9 |
| 45 | `SPI_A1` | 46 | A8 |
| 47 | `SPI_A0` | 48 | A7 |
| 49 | ~`nIOB_S` | 50 | A6 |
| 51 | ~`nIOA_S` | 52 | A5 |
| 53 | ~`IRQB` | 54 | A4 |
| 55 | ~`IRQA` | 56 | A3 |
| 57 | +5 V | 58 | A2 |
| 59 | `HS_CLK` (14.318 MHz) | 60 | A1 |
| 61 | GND | 62 | A0 |

A card's IRQ outputs should be open-collector: the board pulls each line up.  The address and data buses are buffered on the board; a DMA card drives them after asserting ~`DMAB`.

---

### **Companion cards**

#### **Memory daughter card**
*(`board/MemoryDaughterCard`: through-hole and surface-mount versions.)*

One task RAM module: 4 x HM628512, 2 MB.

* **Connectors:** it plugs into the main board's memory connectors J1 (1x32) and J2 (1x24).
* **Module number:** a 2x15 jumper block (J3) connects the card's select to one of `nBRC0-nBRC14`.  That's its module number m, so it answers bank IDs `$m0-$mF`.
* **Addressing:**
  * a 74LS139 (U1) picks one of the four chips with `RAMB_M2-3`;
  * `RAMB_M0-1` go to the chips' A17-A18, and `T_M0-3` to A13-A16;
  * A0-A12 come from J2.

Main board J1 (memory card connector):

| Pins | Signals |
| :--- | :------ |
| 1-3 | A13, A14, A15 (the card doesn't use them) |
| 5-8 | `T_M0-3` |
| 9-12 | `RAMB_M0-3` |
| 13, 14 | `RWB_M`, `!RWB_M` |
| 15-17 | GND |
| 18-32 | `nBRC0_S-nBRC14_S` (module selects) |

J2: D0-D7 (pins 1-8), +5 V (9-11), A0-A12 (12-24).

The ROM finds the installed modules at boot (`MMU_PROBE_MODULES`), tests each one's first bank in POST, and only allocates banks on modules that are present.

#### **Bus breakout card**
*(`board/HydraBusBreakoutCard`.)*  A slot card that brings every bus signal out to headers, for a logic analyzer, a scope or prototyping:

| Header | Signals |
| :----- | :------ |
| J2 | Data bus |
| J3 | Address bus |
| J4 | SPI (A0-A3, ~CS, MISO, MOSI, SCLK) |
| J5 | NMI, `nIO_S`, the slot's port selects and IRQs |
| J6 | Clocks |
| J7 | Bus control (`nBROMD`, BE, RDY, R/W, SYNC) |
| J8 | Reset |
| J9 | Audio |
| J10 | I2C |
| J11 | Power |

---

### **Connectors and jumpers**

| Ref | What | Notes |
| :-- | :--- | :---- |
| J1, J2 | Memory daughter card | Task RAM modules |
| J3 | DE-9 male: serial console | DCE wiring; straight-through F-F cable |
| J4 | RDY pull-up jumper | **Fit** |
| J5-J8 | CPU clock jumpers: 0.89 / 1.79 / 3.58 / 7.16 MHz | Fit **one**: J7 (3.58 MHz) for the standard ROM |
| J9 | YM2151 CT1/CT2 outputs | |
| J10 | Front panel | Reset, power switch, power LED |
| J11 | ATX-24 power | |
| J12-J17 | Expansion slots 0-5 | |
| J18-J25 | SPI devices 0-7 | SD card adapter on J18 (device 0) |
| J26 | Audio out, 3.5 mm stereo | |
| J27 | VIA port A: GPIO, I2C, CA1/CA2 | |
| J28 | SP720 ESD protection for J27 and the SPI lines | (a part, not a connector) |
| J29 | Line input to the audio mixer | |

---

### **V1 errata**

* **Bank register bits 2 and 3 are crossed.**
  * In the bank registers (sheet `ZPMirrorRAM`), data bit 2 drives `RAMB3`, and bit 3 drives `RAMB2`: IC1 for the RAM bank, IC3 for the ROM bank.
  * So a bank ID's bits 2 and 3 trade places before they reach the hardware.  For example, shared bank IDs `$F4-$F7` are on U28 and `$F8-$FB` on U27, module 4 and module 8 trade places, and so do paged ROM banks `$04-$07` and `$08-$0B`.
  * IDs whose bits 2 and 3 are equal aren't affected, and the software never needs to care: an ID always reaches the same memory.  It matters when you map a bank ID to a chip, for example to act on a POST report.
  * **Bits 6 and 7** have also been reported crossed on the V1 board.  The schematic's IC2 and IC4 show them straight through, so confirm on the board before relying on either.
* **No wait states.**  RDY only has a pull-up, so slow devices can't stretch a bus cycle.  This is why the YM2151 can't be used above 3.58 MHz.  Board V2 is planned to have programmable RDY wait states (see [plans/IDEAS.md](plans/IDEAS.md)).
* **Audio jack channels.**  J26 has the right channel on the tip and the left on the ring, per the schematic; the usual convention is the reverse, so left and right may come out swapped.
* **ACIA clock.**  The ACIA runs from 1.790 MHz instead of 1.8432 MHz, so its baud rates are 2.9% slow (see [ACIA](#acia-65c51-u3-port-1-irq-line-1)).

---

### **Parts by function**

| Function | Parts |
| :------- | :---- |
| CPU | U1 W65C02S |
| Bus buffers | U58, U60 74F541 (address); U59 74F245 (data); U61 74F125 (R/W) |
| Address decoding | U8, U9 74LS260; U10 74F20; U11 74F08; U12 74F86; U13, U16 74F32; U14 74F30; U15 74F00; U19 74LS154 (I/O ports); U20 74F138 (system port); U21 74F541 (memory card buffer); U22 74F240 |
| Pseudo-registers | U48, U50, U52, U54 74F573; U49, U51, U53, U55 74F541; U56 74F32; U57 74F02 |
| Bank registers | IC1-IC4 74LS219; U23 74LS154 (module select); U24 74F138 (paged ROM chip select) |
| Interrupts | U17, U18 74LS148; U44 74F00; U45 74F157; IC5-IC8 74LS219 (vector RAM); RN2, RN3 pull-ups |
| Task RAM | U7 HM628512 |
| Shared RAM | U25, U27, U28, U29 HM628512; U26 74F139 |
| Paged ROM | U30-U37 SST39SF040 |
| BIOS ROM | U6 SST39SF010/020/040 |
| Clocks | Y1 14.31818 MHz; U39 74ACT14; U40 74F191 |
| Reset | U43 DM1813 |
| VIA, SPI | U2 R65C22; U4 74F138 (SPI device select) |
| Serial | U3 R65C51; U5 MAX232 |
| Sound | U38 YM2151; U41 YM3012; U42 TL074; IC9 LF353 |
