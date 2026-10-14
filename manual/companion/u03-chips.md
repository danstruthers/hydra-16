# Unit 3. Power, ROMs and Chips

> **THIS STAGE:** Manual: sections 4.3 to 4.6, and 4.8. The board gets power for the first time, *before* any chip
> is in it; then the ROM chips are programmed, and every chip goes into its socket.

## The power test

⚠ No chips in the sockets yet. If something is wrong with the power, this is when you want to find out.

**Before the power supply is connected** (section 4.3):

| ☐ | Check | What I measured |
|---|---|---|
| ☐ | Resistance from pin 8 to pin 21 of U1's socket: it should be *more* than about 100 ohms | |

✎ If the reading was close to zero, what did you find?

<div class="write l2"></div>

**With the power supply and a switch connected** (section 4.8), and switched on:

| ☐ | Red probe on | Should read | What I measured |
|---|---|---|---|
| ☐ | U1 socket, pin 8 | +4.75 to +5.25 V | |
| ☐ | U42 socket, pin 4 | about +12 V | |
| ☐ | U42 socket, pin 11 | about −12 V | |

| ☐ | And |
|---|---|
| ☐ | Nothing got hot, and nothing smells of burning. |
| ☐ | The power light lights (if it's wired). |
| ☐ | The power is switched off again, **and unplugged**, before I go on. |

## The ROM chips

Skip this if your ROM chips came programmed and labelled (section 4.4).

⚠ First look at the serial chip, U3. The kernel ROM, U6, has a file for each kind: `bios_rockwell.bin` for a
Rockwell R65C51, `bios_wdc.bin` for a WDC W65C51N (section 4.4). The paged ROM's files are the same for both.

| ☐ | Check |
|---|---|
| ☐ | U3 is a ________________, so U6's file is ________________. |

| Programmed | Verified | Labelled | File | Chip type | Goes in |
|---|---|---|---|---|---|
| ☐ | ☐ | ☐ | `bios_rockwell.bin` or `bios_wdc.bin` | SST39SF010A (PLCC) | U6 |
| ☐ | ☐ | ☐ | `prom0.bin` | SST39SF040 | U31 |
| ☐ | ☐ | ☐ | `prom1.bin` | SST39SF040 | U32 |
| ☐ | ☐ | ☐ | `prom2.bin` | SST39SF040 | U34 |

✎ Which programmer did you use, on what date were the ROM images built, and from which version of HydraOS? (You'll
want to know this when you program them again.)

<div class="write l2"></div>

## The chips

⚠ Wrist strap on. Power unplugged. Read the number on each chip, and the number printed on the board beside its
socket, before the chip goes in: many of them look alike (section 4.5). The notch on the chip matches the notch on
the socket.

<!-- generated: 4.5/1 -->

| Fitted | Checked | Chip | Places | How many |
|---|---|---|---|---|
| ☐ | ☐ | W65C02S (processor) | U1 | 1 |
| ☐ | ☐ | R65C22 or W65C22N (VIA) | U2 | 1 |
| ☐ | ☐ | R65C51 or W65C51N (ACIA, serial) | U3 | 1 |
| ☐ | ☐ | MAX232 (serial levels) | U5 | 1 |
| ☐ | ☐ | SST39SF010A, programmed with U3's kernel ROM file | U6 | 1 |
| ☐ | ☐ | HM628512 (task RAM), or a DS1747 for a real-time clock | U7 | 1 |
| ☐ | ☐ | SST39SF040, programmed with `prom0.bin`, `prom1.bin`, `prom2.bin` | U31, U32, U34 | 3 |
| ☐ | ☐ | YM2151 (sound) | U38 | 1 |
| ☐ | ☐ | YM3012 (sound converter) | U41 | 1 |
| ☐ | ☐ | TL074 (op-amps) | U42 | 1 |
| ☐ | ☐ | NJM4556A (headphone amplifier) | IC9 | 1 |
| ☐ | ☐ | SP720 (protection for the I/O header) | J28 | 1 |
| ☐ | ☐ | 74F138 | U4, U20, U24 | 3 |
| ☐ | ☐ | 74LS260 | U8, U9 | 2 |
| ☐ | ☐ | 74F20 | U10 | 1 |
| ☐ | ☐ | 74ACT08 | U11 | 1 |
| ☐ | ☐ | 74F86 | U12 | 1 |
| ☐ | ☐ | 74F32 | U13, U16, U56 | 3 |
| ☐ | ☐ | 74F30 | U14 | 1 |
| ☐ | ☐ | 74F00 | U15, U44 | 2 |
| ☐ | ☐ | 74LS148 | U17, U18 | 2 |
| ☐ | ☐ | 74LS154 | U19, U23 | 2 |
| ☐ | ☐ | 74F541 | U21, U49, U51, U53, U55, U58, U60 | 7 |
| ☐ | ☐ | 74ACT240 | U22 | 1 |
| ☐ | ☐ | 74F139 | U26 | 1 |
| ☐ | ☐ | 74ACT14 | U39 | 1 |
| ☐ | ☐ | 74F191 | U40 | 1 |
| ☐ | ☐ | 74F157 | U45 | 1 |
| ☐ | ☐ | 74F573 | U48, U50, U52, U54 | 4 |
| ☐ | ☐ | 74F02 | U57 | 1 |
| ☐ | ☐ | 74ACT245 | U59 | 1 |
| ☐ | ☐ | 74F125 | U61 | 1 |
| ☐ | ☐ | 74LS219 | IC1 to IC8 | 8 |

<!-- end generated: 4.5/1 -->

| ☐ | Afterwards |
|---|---|
| ☐ | I looked along every chip from the side: no leg is bent under, or outside its socket. |
| ☐ | The five empty sockets are U30, U33, U35, U36 and U37, and nothing else is empty. |

✎ Does your Hydra have a DS1747 clock chip in U7, or an HM628512? And an R65C51 in U3, or a W65C51N?

<div class="write l2"></div>

## The jumpers

| ☐ | Jumper | Where it goes |
|---|---|---|
| ☐ | The clock | One jumper only, on **J7** (3.58 MHz) |
| ☐ | J30 | Pins 1 and 2 (or 2 and 3, for a three-wire serial cable) |
| ☐ | J9 and J29 | **No** jumpers |

## The power, the switch, the reset button and the light

| ☐ | Connection | To |
|---|---|---|
| ☐ | The ATX supply's 24-pin plug | J11 |
| ☐ | A power switch that stays on | J10, pins 11 and 9 |
| ☐ | A reset push button | J10, pins 7 and 5 |
| ☐ | A power light: the LED's longer leg to pin 12, its shorter leg to pin 10 | J10, pins 12 and 10 |

✎ Draw J10 as you've wired it, with pin 1 marked.

<div class="space s2"></div>
