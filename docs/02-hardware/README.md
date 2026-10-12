# 2. The hardware

This chapter is the Hydra-16's board, from the outside in.  It starts with how the board works as a whole: what the
processor sees at each address, and which chips make each part of that happen.  Then come the devices on the board, the
parts you need to build one, how to build it and bring it up, where everything is on the board, what it does that you
wouldn't expect, and the cards that plug into it.

It describes **board revision 1.8D**, the schematic in `board/` (`board/hydra-16.kicad_sch` and its sheets; KiCad 9).
The 1.8D board runs the same software as 1.8C: it changes some parts for reliability, and nothing a program can see.
[The quirks](quirks.md#what-changed-from-18c-to-18d) lists what changed.

Reference designators (U25, J18, R28) are the schematic's.  The registers' names (`T_REGISTER`, `VIA_T1CL`, ...) are
those of `base/include/hw.inc`, which every program on the Hydra includes.

## Contents

1. [The architecture](architecture.md): how the board works, part by part.
   * [The memory map](architecture.md#the-memory-map) and [the pseudo-registers `T`, `U`, `V`, `W`](architecture.md#the-pseudo-registers-t-u-v-w)
   * [Task RAM and the bank registers](architecture.md#task-ram-and-the-bank-registers), [the RAM window](architecture.md#the-ram-window),
     [the paged ROM](architecture.md#the-paged-rom), [the BIOS ROM](architecture.md#the-bios-rom)
   * [I/O space](architecture.md#io-space), [interrupts](architecture.md#interrupts), [clocks](architecture.md#clocks),
     [reset, power and the bus](architecture.md#reset-power-and-the-bus)
2. [The devices](devices.md): the VIA and its GPIO, I2C and SPI buses, the ACIA and the serial port, the YM2151 and the
   audio path, the clock chip, the expansion slots.
3. [The parts](parts.md): every part on every board, generated from the schematics, with what to buy.
4. [Building it](building.md): assembly, the first power-up, programming the ROMs, POST, the hardware test, and finding
   faults.
5. [The board layout](layout.md): a drawing of each board, where each part is, and the connectors' pins.
6. [The quirks](quirks.md): what the board does that you wouldn't expect, and what changed from 1.8C to 1.8D.
7. [The cards](cards.md): the memory daughter card, the bus breakout card, and the Vera X video card.

## The board in one picture

```
                    +-----------------------------------------------------------+
  ATX power  ----> | J11  power, reset (U43), clocks (Y1, U39, U40)            |
  front panel ---> | J10                                                       |
                    |                                                           |
  serial (DE-9) <->| J3  ACIA U3 + MAX232 U5         VIA U2  ---> J27 GPIO/I2C |
                    |                                   |                       |
                    |  CPU U1 (W65C02S)                 +--> SPI J18-J25 (SD)   |
                    |    |                                                      |
                    |  buffers U58-U61 --- the system bus ----------------------+--> slots J12-J17
                    |    |                                                      |
                    |  decode (U8-U16, U19, U20)                                |
                    |    |                                                      |
                    |  T U V W latches (U48-U55)   bank registers (IC1-IC4)     |
                    |    |                              |                       |
                    |  task RAM U7   shared RAM U25,U27-U29   memory cards J1/J2 |
                    |  BIOS ROM U6   paged ROM U30-U37                          |
                    |  IRQ encoders U17/U18 + vector RAM IC5-IC8                |
                    |  YM2151 U38 -> YM3012 U41 -> TL074 U42 -> mixer IC9 -> J26 audio out
                    +-----------------------------------------------------------+
```

[The board layout](layout.md) has the real drawing, made from the PCB.
