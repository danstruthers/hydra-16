# The devices

The chips on the board that do I/O, and the connectors they reach.  Each section gives the chip's port and IRQ line,
how it's wired, and which driver serves it: [chapter 4](../04-drivers/README.md) is the software side.

## Contents

1. [The VIA (65C22): timers, GPIO, I2C, SPI](#the-via-65c22)
2. [The SPI bus and its device headers](#the-spi-bus)
3. [The I2C bus](#the-i2c-bus)
4. [The ACIA (65C51) and the serial port](#the-acia-65c51-and-the-serial-port)
5. [The YM2151 and the audio path](#the-ym2151-and-the-audio-path)
6. [The clock chip](#the-clock-chip)
7. [The expansion slots](#the-expansion-slots)

---

## The VIA (65C22)
*U2, a Rockwell R65C22 (or WDC W65C22N).  Port 0, `$FF00-$FF0F`; IRQ line 0, the highest priority.*

The VIA is the board's general-purpose I/O chip: two 8-bit ports, two timers, and two handshake lines on each port.
On the Hydra:

| Part of the VIA | Used for |
| :-------------- | :------- |
| **Port A**, PA0-PA1 | The I2C bus: PA0 is SCL, PA1 is SDA (bit-banged) |
| **Port A**, PA2-PA7, CA1, CA2 | General-purpose I/O, on header J27 |
| **Port B** | The SPI bus (bit-banged): its clock, data, select and device number |
| **Timer 1** | The kernel's tick: free-running, 200 interrupts a second, which drives the scheduler and the clock |
| **Timer 2** | The serial driver's send pacing ([below](#the-acia-65c51-and-the-serial-port)) |
| **CB1, CB2**, the shift register | Not brought out: the shift register can't drive the SPI lines |

**J27, the VIA port A header** (2 x 6, 2.54 mm):

| Pin | Signal | Pin | Signal |
| :-- | :----- | :-- | :----- |
| 1 | GND | 2 | +5 V |
| 3 | PA0: `I2C_SCL` | 4 | PA1: `I2C_SDA` |
| 5 | PA2 | 6 | PA3 |
| 7 | PA4 | 8 | PA5 |
| 9 | PA6 | 10 | PA7 |
| 11 | CA1 | 12 | CA2 |

Every pin of J27, and the SPI bus's clock and data lines, pass through **J28**, an SP720 ESD protection array (a
chip in a DIP-16 socket, though the schematic calls it a connector).  It clamps static from fingers and cables to the
supply rails.  The board works without it, but fit it if anything's ever plugged into J27.

HydraOS serves the pins as files (in `kdev`): `/dev/gpio/0` to `/dev/gpio/7`, the whole port as `port`, `ctl` to
set each pin's direction, and `ca1`, which can interrupt ([GPIO, I2C, SPI and the clock](../04-drivers/gpio.md)).
PA0 and PA1 belong to the I2C bus, so don't use them as GPIO.

---

## The SPI bus
*VIA port B, bit-banged; U4 (74F138) decodes the device select.*

| Port B bit | Signal |
| :--------- | :----- |
| PB0 | `SPI_CLK`: the clock (SCLK) |
| PB1 | `nSPI_CS`: low selects the device given by PB3-PB6 |
| PB2 | `SPI_MOSI`: data out |
| PB3-PB5 | `SPI_A0-A2`: the device, 0-7 |
| PB6 | `SPI_A3`: 0 for the board's devices 0-7 (U4); 1 for devices 8-15, which a card in a slot decodes itself |
| PB7 | `SPI_MISO`: data in |

So a transfer is: set PB3-PB6 to the device, take PB1 low, clock bits out on PB2 and in from PB7 by toggling PB0,
then take PB1 high.  U4 turns `nSPI_CS` and `SPI_A0-A3` into one select line for each of the eight headers.  Every
slot carries the whole bus, `nSPI_CS` and `SPI_A0-A3` included, so a card can be devices 8-15.

**The eight SPI device headers**, J18-J25 for devices 0-7, are each 1 x 6, 2.54 mm:

| Pin | Signal |
| :-- | :----- |
| 1 | ~CS (the device's select) |
| 2 | SCLK |
| 3 | MOSI |
| 4 | MISO |
| 5 | +5 V |
| 6 | GND |

**SD cards.**  An SD card goes on a header through a common SD card module, one with its own 3.3 V regulator and
level shifter: the headers give +5 V and 5 V logic, which would destroy a bare card.  Device 0 (J18) is the usual
place.  The storage driver serves a card on any device as `/dev/sd/N` (N the device, in hex) and its file system at
`/sd/N` ([storage](../04-drivers/storage.md)).  Any SPI device can also be used raw, as `/dev/spi/N`.

---

## The I2C bus
*VIA PA0 (SCL) and PA1 (SDA), bit-banged; pull-ups in RN1.*

The I2C bus goes to J27 (pins 3 and 4) and to every slot (pins 28 and 30), with its pull-ups on the board, so a device
on a card needs none of its own.  HydraOS serves it as `/dev/i2c`: its `ctl`, and a file for each device that
answers.  The Vera X's input controller (the keyboard and mouse) is on it at `$42`.

Being bit-banged, it's slow (some tens of kHz), and an I2C device can't be given an interrupt line: the board's lines
can't be masked, and a device can only be quieted by an I2C transaction, too slow for an interrupt handler.  So I2C
devices are polled.

---

## The ACIA (65C51) and the serial port
*U3, a Rockwell R65C51 (or WDC W65C51N).  Port 1, `$FF10-$FF13` (repeated through `$FF1F`); IRQ line 1.*

The serial port is the Hydra's console: everything in this book is typed at a terminal on it.

**The chip and its clock:**

* The ACIA's clock is **`SER_CLK`, 1.790 MHz**, not the 1.8432 MHz its baud rate table assumes.  So every rate is 2.9%
  slow: 9600 baud is really about 9,320.  Terminals and USB serial adapters accept that.
* The rates the board has are the ACIA's table's: 300, 600, 1200, 2400, 4800, 9600, 19200, and 115200 (the ACIA's
  clock divided by 16).  Both the base and HydraOS start at **9600 baud, 8 data bits, no parity, 1 stop bit**, with
  RTS/CTS.  HydraOS can change the rate (`echo b115200 >/dev/serctl`).
* DCD and DSR are tied active (to ground).
* A **WDC W65C51N** works too, with the BIOS ROM for it (`bios_wdc.bin`): its transmitter's status bit and interrupt
  don't work, so the drivers time each byte instead.

**Sending is paced.**  At 115200 the 2.9% error and the MAX232's slow edges leave the receiver little margin, so long
output sent back to back loses characters.  So the serial driver sends each byte from VIA timer 2, not from the
ACIA's "transmitter empty" interrupt: a character's time and two idle bits at 115200, one at the other rates.  Every
build does this, on either chip.

**The DE-9 connector** (J3, a female socket) is driven by a MAX232 (U5), and wired like a modem (DCE):

| DE-9 pin | Signal | Direction |
| :------- | :----- | :-------- |
| 2 | The Hydra transmits (ACIA TxD) | out |
| 3 | The Hydra receives (ACIA RxD) | in |
| 5 | Ground | |
| 7 | ACIA ~CTS, from the terminal's RTS (through J30) | in |
| 8 | ACIA ~RTS, to the terminal's CTS | out |
| 1, 4, 6, 9 | Not connected | |

**The cable.** A USB serial adapter's male plug goes straight into J3. An extension between them is a
**straight-through** male-to-female cable (pin 2 to pin 2, 3 to 3, and so on), not a null-modem cable.

**J30, the CTS jumper** (1.8D).  The ACIA only transmits while its ~CTS is low:

* **1-2** (fit this): ~CTS comes from the line's pin 7, so the terminal's RTS can stop the Hydra.  The cable must
  carry pin 7, and the terminal must assert RTS (they do by default).
* **2-3**: ~CTS is held low: the Hydra always sends.  For a three-wire cable (pins 2, 3 and 5 only).
* With no jumper, ~CTS floats and the Hydra may send nothing at all.

---

## The YM2151 and the audio path
*U38, a Yamaha YM2151 (OPM).  Port 4: `$FF40` address and status, `$FF41` data (repeated through `$FF4F`); IRQ line
4.  Sheets `Sound` and `Mixer`.*

**The chip.**  The YM2151 is an 8-channel, 4-operator FM synthesizer, the chip in many 1980s arcade machines and the
Commander X16.  It's clocked by `SND_CLK`, 3.58 MHz, its design frequency.

* Writing `$FF40` selects a register; writing `$FF41` writes it.  Reading `$FF40` gives the status: bit 7 is busy,
  bits 0 and 1 are timer A's and timer B's flags.
* After a data write the chip is busy for 64 of its clocks, about 18 µs, and a write while it's busy is lost: so the
  driver checks bit 7 before each write.
* Its two timers can interrupt on line 4.  The sound driver doesn't use them for timing songs (the system's tick is
  steadier: [the quirks](quirks.md#the-ym2151s-timer-b)).
* **CT1 and CT2**, its two general-purpose outputs, are on header J9.

**The audio path:**

```
 YM2151 U38 --serial--> YM3012 U41 (DAC) --> TL074 U42 (buffers, filters) --> C90/C91 --+
                                                                                        |
 slot 0's SND_CL0 / SND_CR0 (the Vera X) ---------------------------------------------- +--> mixer IC9 --> R30/R31 --> C92/C93 --> J26
                                                                                        |   (NJM4556A,
 line input J29 ------------------------------------------------------------------------+    ±12 V)
```

* The **YM3012** (U41) turns the YM2151's serial output into two analog channels, centred on its COM pin (about
  2.5 V).  A **TL074** (U42) buffers and filters them.  In 1.8D, C90 and C91 (10 µF) keep that 2.5 V out of the mixer.
* The **mixer** (IC9, an NJM4556A on ±12 V) is two inverting summers, one for each channel, each at unity gain (10K in,
  10K feedback).  Each adds three inputs: the YM3012's channel, **slot 0's** audio (`SND_CL0`, `SND_CR0`: the Vera X's
  PSG and PCM), and the line input on **J29** (pin 1 left, pin 2 right).  On 1.8D the other slots' audio pins aren't
  connected; 1.8C mixed all six slots ([the quirks](quirks.md#only-slot-0s-audio-is-mixed)).
* **The output** (1.8D): through R30/R31 (33 ohms, which keep the op-amp stable into a cable and safe from a shorted
  plug), and C92/C93 (220 µF, non-polar), to the 3.5 mm stereo jack **J26**: left on the tip, right on the ring.  R32
  and R33 hold the jack's side at 0 V so a plug going in doesn't click.  The NJM4556A drives headphones (16-32 ohms)
  directly.
* **The grounds.**  The audio's ground, GNDA, meets the logic's ground once, at R29 (0 ohms) beside the YM3012, and at
  the ATX connector.

The sound driver serves the YM2151 and the Vera X's PSG as one synthesizer, `/dev/snd` ([sound](../04-drivers/sound.md)).

---

## The clock chip

The board has no clock chip of its own: the time comes from a **Dallas DS1747** in the task RAM's socket, U7, in place
of the HM628512.  The DS1747 is a 512K static RAM with a battery and a real-time clock inside, in the same pinout; its
clock's eight registers are its top 8 bytes.  Since `T0-T3` drive U7's A15-A18, those are task F's `$7FF8-$7FFF`.

* Task F's program (the serial driver, `ser` or `cons`) keeps its RAM below `$7FF8`, so nothing else writes there.
* The kernel's `RTC` call reads and sets the chip's registers; `kdev` sets the system's clock from the chip as it
  starts, and `/dev/time` (`date`) sets both the system's clock and the chip's.
* The rest of the chip is task RAM as before, kept while the power's off.
* A new DS1747 comes with its battery disconnected and its oscillator stopped: setting the time once starts it.
* Without a DS1747 the clock starts at 2000-01-01 each boot, until something sets it.

Use the 5 V DS1747, not the 3.3 V DS1747W.

---

## The expansion slots
*J12-J17, slots 0-5: 62-pin card edge connectors, the "Hydra bus 8-bit".  Sheet `Connectors`.*

All six slots carry the same bus, except for each slot's two port selects and its two IRQ lines (and its audio pair,
which on 1.8D only slot 0's reaches the mixer):

| Slot | Connector | `nIOA_S` (pin 51) | `nIOB_S` (pin 49) | `IRQA` (pin 55) | `IRQB` (pin 53) |
| :--- | :-------- | :---------------- | :---------------- | :-------------- | :-------------- |
| 0 | J12 | port 2, `$FF20` | port 3, `$FF30` | line 2 | line 3 |
| 1 | J13 | port 5, `$FF50` | port 10, `$FFA0` | line 5 | line 10 |
| 2 | J14 | port 6, `$FF60` | port 11, `$FFB0` | line 6 | line 11 |
| 3 | J15 | port 7, `$FF70` | port 12, `$FFC0` | line 7 | line 12 |
| 4 | J16 | port 8, `$FF80` | port 13, `$FFD0` | line 8 | line 13 |
| 5 | J17 | port 9, `$FF90` | port 14, `$FFE0` | line 9 | line 14 |

Slot 0's lines have the highest priority of the slots, which is why the video card goes there; the B lines of slots
1-5 have the lowest.

**The pinout**, the same on every slot (`~` active low):

| Pin | Signal | Pin | Signal |
| :-- | :----- | :-- | :----- |
| 1 | GND | 2 | ~`nBROMD`: disable the paged ROM |
| 3 | ~`RESB` | 4 | D7 |
| 5 | +5 V | 6 | D6 |
| 7 | ~`NMIB` | 8 | D5 |
| 9 | -5 V (ATX pin 20, if the supply has it) | 10 | D4 |
| 11 | `BE` | 12 | D3 |
| 13 | -12 V | 14 | D2 |
| 15 | `SER_CLK` (1.79 MHz) | 16 | D1 |
| 17 | +12 V | 18 | D0 |
| 19 | GND | 20 | `RDY` |
| 21 | R/~W | 22 | `SND_CLK` (3.58 MHz) |
| 23 | ~`DMAB` | 24 | `SND_CR`: audio in, right (slot 0 only) |
| 25 | ~`nIO_S`: any I/O port | 26 | `SND_CL`: audio in, left (slot 0 only) |
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
| 49 | ~`nIOB_S`: the slot's port B | 50 | A6 |
| 51 | ~`nIOA_S`: the slot's port A | 52 | A5 |
| 53 | ~`IRQB` | 54 | A4 |
| 55 | ~`IRQA` | 56 | A3 |
| 57 | +5 V | 58 | A2 |
| 59 | `HS_CLK` (14.318 MHz) | 60 | A1 |
| 61 | GND | 62 | A0 |

**Designing a card:**

* **Select your registers** with `nIOA_S` or `nIOB_S`: each is low for the slot's 16 bytes, only while PHI2 is high,
  with the address already steady.  Decode A0-A3 for the register.  For more than 32 bytes, decode `nIO_S` and A0-A7
  yourself, at addresses no other card uses.
* **Interrupts:** drive `IRQA`/`IRQB` open-collector (pull low, or let go): the board pulls each line up.  Hold the
  line until the handler clears the cause, and give the card a way to be quieted from a register.
* **Timing:** the bus has no wait states.  A read's data must be on the bus 10 ns before PHI2 falls (about 140 ns of
  PHI2 high at 3.58 MHz, 70 ns at 7.16), and a write's data is held only 10 ns after PHI2 falls, so latch it with a
  strobe that ends with PHI2.
* **An empty slot reads `$FF`** (1.8D's pull-ups), so software can look for a card by reading its registers.
* **Power:** +5 V, +12 V and -12 V are always there; -5 V only with an old ATX supply.  +3.3 V isn't on the slots.

[The cards](cards.md) describes the cards made so far.
