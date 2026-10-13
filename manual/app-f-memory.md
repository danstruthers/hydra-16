# Appendix F. The Memory and I/O Map

This appendix collects the Hydra's addresses in one place: what the 65C02 sees, what's behind its windows, and the
registers of each chip. Chapter 7 explains how it all fits together.

## F.1 The 64K the 65C02 sees

| Addresses | Size | What's there | Chosen by |
|---|---|---|---|
| $0000-$7FFF | 32K | The running task's own RAM (U7) | T ($FFF0) |
| $8000-$9FFF | 8K | A RAM bank: the task's own, or a shared one | The RAM bank register ($00), and U |
| $A000-$DFFF | 16K | A paged ROM bank | The ROM bank register ($01) |
| $E000-$FEFF | 8K (less 256 bytes) | A kernel ROM page (U6) | W ($FFF3) |
| $FF00-$FFEF | 240 | The I/O ports: 15 ports of 16 bytes (F.5) | (fixed) |
| $FFF0-$FFF3 | 4 | The registers T, U, V and W | (fixed) |
| $FFFA-$FFFD | 4 | The NMI and reset vectors, from the kernel ROM page | W |
| $FFFE-$FFFF | 2 | The interrupt vector: the vector RAM (F.6) | The interrupt line, or V |

## F.2 A task's 32K

| Addresses | What's there |
|---|---|
| $00 | The RAM bank register (written: chooses the bank at $8000; read: the bank chosen) |
| $01 | The ROM bank register (the same, for $A000) |
| $02-$21 | r0 to r15: the call registers, 2 bytes each, low byte first |
| $22-$7F | The program's zero page (94 bytes) |
| $80-$FF | HydraOS's zero page for the task |
| $0100-$01FF | The stack (a program starts with S = $FD) |
| $0200-$03FF | HydraOS's area for the task: its open files, its name, its arguments ... |
| $0400-$7FFF | The program's memory. A program from a file is loaded at $0800: its header (48 bytes), code, data, BSS, then free memory up to its break. A program in the paged ROM runs there, in place, and has its data and BSS here from $0400 |
| $7FF8-$7FFF | In task F (15) only, with a DS1747 for U7: the clock's registers |

## F.3 The RAM banks

A task's RAM bank register takes a bank number from $00 to $FF:

| Banks | What they are |
|---|---|
| $m0-$mF | The task's own 16 banks on RAM module *m* (0 to 14) of the memory cards: 128K for each task on each module |
| $F0-$FF | 16 of the shared RAM's 256 banks (2 MB), chosen by U: bank $Fn is shared bank U × 16 + *n* |

So with three RAM modules (modules 0, 1 and 2), a task has banks $00-$2F: 48 banks, 384K. Programs ask HydraOS for
banks with `BANKS_ALLOC` (and for shared ones, through **segments**, with `SEG_CREATE`), rather than taking them.

## F.4 The paged ROM

The ROM bank register chooses one of 256 banks of 16K: 4 MB in eight sockets, U30 to U37, of 512K (32 banks) each.
HydraOS's three chips (in U31, U32 and U34, section 4.4) hold its modules (the drivers, the shells, the languages,
the libraries) and then the ROM disk. A big module has banks of its own; a small one shares a bank with others, each
in its own part of it. `mods` lists the modules and their banks (Chapter 11).

## F.5 The I/O ports

| Port | Addresses | IRQ line | Chip |
|---|---|---|---|
| 0 | $FF00-$FF0F | 0 | 65C22 VIA (U2): the tick, the SPI bus, the I/O header J27 |
| 1 | $FF10-$FF1F | 1 | 65C51 ACIA (U3): the serial port |
| 2, 3 | $FF20-$FF3F | 2, 3 | Slot 0: the Vera X |
| 4 | $FF40-$FF4F | 4 | YM2151 (U38): FM sound |
| 5-9 | $FF50-$FF9F | 5-9 | Slots 1 to 5: select A |
| 10-14 | $FFA0-$FFEF | 10-14 | Slots 1 to 5: select B |

**The VIA** (65C22, port 0):

| Address | Register | On the Hydra |
|---|---|---|
| $FF00 | Port B | The SPI bus: PB0 the clock, PB1 the select enable, PB2 MOSI, PB3-PB6 the device, PB7 MISO |
| $FF01 | Port A | J27's pins: PA0 SCL and PA1 SDA (the I²C bus), PA2-PA7 general purpose |
| $FF02 | DDRB | Port B's directions (1: an output) |
| $FF03 | DDRA | Port A's directions |
| $FF04, $FF05 | T1C-L, T1C-H | Timer 1's counter: HydraOS's tick, 200 a second |
| $FF06, $FF07 | T1L-L, T1L-H | Timer 1's latch |
| $FF08, $FF09 | T2C-L, T2C-H | Timer 2 (the console's pacing) |
| $FF0A | SR | The shift register |
| $FF0B | ACR | The auxiliary control register |
| $FF0C | PCR | The peripheral control register (CA1: J27 pin 11, CA2: pin 12) |
| $FF0D | IFR | The interrupt flags |
| $FF0E | IER | The interrupt enables |
| $FF0F | Port A (no handshake) | Port A, without CA1 and CA2's handshake |

**The ACIA** (65C51, port 1):

| Address | Register |
|---|---|
| $FF10 | Data: the byte received (read), or to send (write) |
| $FF11 | Status (read); writing it resets the chip |
| $FF12 | Command |
| $FF13 | Control: the speed, and the bits of each byte |

The ACIA's clock on this board is 1.790 MHz, so every speed is 2.9% slow (section 13.6).

**The YM2151** (port 4):

| Address | Register |
|---|---|
| $FF40 | Write: chooses one of the chip's registers |
| $FF41 | Write: the chosen register; read: the status (bit 7: busy; bits 0 and 1: the timers) |

**The Vera X** (slot 0, ports 2 and 3): the VERA's 32 registers, $FF20 + 0 to $FF20 + $1F, in the same order as
on the Commander X16 (where they're at $9F20), so the X16's documentation describes them:

| Address | Register | Address | Register |
|---|---|---|---|
| $FF20-$FF22 | ADDRx_L, ADDRx_M, ADDRx_H: a VRAM address | $FF2D-$FF33 | Layer 0: CONFIG, MAPBASE, TILEBASE, scrolls |
| $FF23, $FF24 | DATA0, DATA1: VRAM at the addresses | $FF34-$FF3A | Layer 1: the same |
| $FF25 | CTRL | $FF3B | AUDIO_CTRL: recorded sound |
| $FF26, $FF27 | IEN, ISR: interrupts | $FF3C | AUDIO_RATE |
| $FF28 | IRQLINE_L, SCANLINE_L | $FF3D | AUDIO_DATA |
| $FF29-$FF2C | DC_VIDEO ... (chosen by CTRL's DCSEL) | $FF3E, $FF3F | SPI_DATA, SPI_CTRL: the SD card |

The Vera X's keyboard and mouse controller isn't on these ports: it's on the I²C bus, at address $42 (section 16.9).

**The Hydra's registers:**

| Address | Register | What it does |
|---|---|---|
| $FFF0 | T | The task: which 32K of U7 is at $0000-$7FFF, and whose bank registers $00 and $01 are |
| $FFF1 | U | Which 16 of the shared RAM's 256 banks are banks $F0-$FF |
| $FFF2 | V | The vector RAM's entry when no interrupt line is active (the `BRK`'s) |
| $FFF3 | W | The kernel ROM page at $E000-$FEFF |

They can be written and read back. Only HydraOS writes them.

## F.6 The interrupt lines

The lines go to the priority encoder (U17, U18): when several are active, the lowest number wins. The vector RAM
(IC5 to IC8) holds a handler's address for each line, which the 65C02 reads at $FFFE.

| Line | Its interrupt | Line | Its interrupt |
|---|---|---|---|
| 0 | The VIA: its timers (the tick), CA1 | 8 | Slot 4, select A |
| 1 | The ACIA: the serial port | 9 | Slot 5, select A |
| 2 | Slot 0: the Vera X's VERA | 10 | Slot 1, select B |
| 3 | Slot 0's second line | 11 | Slot 2, select B |
| 4 | The YM2151's timers | 12 | Slot 3, select B |
| 5 | Slot 1, select A | 13 | Slot 4, select B |
| 6 | Slot 2, select A | 14 | Slot 5, select B |
| 7 | Slot 3, select A | 15 | (None: the `BRK`'s entry) |

## F.7 HydraOS's addresses

The kernel's ROM, U6, is 16 pages of 8K, and W chooses which is at $E000. Every page has the same few things in the
same places, so the kernel can switch pages and carry on:

| Address | What's there |
|---|---|
| $E000 | The reset stub, on every page: it switches to page 0 (W isn't cleared by a reset) |
| $F800-$FA18 | On page 0: the system calls' jump table (Appendix K), 3 bytes each, a `jmp` to the call's code |
| $FD00-$FDFE | The COMMON block, the same on every page: the interrupts' entry and exit, and the kernel's calls between pages |
| $FDFF | The page's number (the power-on test reads it to check the W lines) |
| $FFFA-$FFFD | The NMI and reset vectors, on every page |

Page 0 has the kernel's core, page 1 its tasks and memory, page 2 its files, page 3 its namespaces, and page 4 the
power-on test. In the paged ROM, a module's (or library's) header is its first 48 bytes: at $A000 in its first bank,
or, for a small module that shares a bank, at $A000 plus 256 times its **page** (`mods` shows bank 4, page $33, as
`4+33`: its header is at $D300).
