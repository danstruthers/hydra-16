# Hydra-16 V1.8D: what changed from 1.8C

Reliability changes only. The address map, the decoding, every register and every timing a program can see are
1.8C's, so the same ROMs and software run on both boards.

## Parts swapped (same footprints, same pinouts)

| Part | 1.8C | 1.8D | Why |
| :--- | :--- | :--- | :--- |
| U11 | 74F08 | 74ACT08 | It drives the CPU's clock input (P_PHI0). The W65C02S wants a CMOS high (0.7 VDD); a 74F output is guaranteed only about 2.5 V. |
| U22 | 74F240 | 74ACT240 | It drives IRQB, and PHI1, DMA and !RWB. |
| U59 | 74F245 | 74ACT245 | It drives the CPU's data bus on every read. |
| IC9 | LF353 | NJM4556A | The mixer's op-amp drives the headphone jack. The LF353 wants 2 kOhm or more and current-limits into 16-32 ohm headphones, which distorted. The 4556A (same dual DIP-8 pinout) drives about 70 mA. |

ACT parts are a few ns slower than F parts and have faster edges: check the timing at 7.16 MHz.

## Parts added

| Part | Value | Sheet | What |
| :--- | :--- | :--- | :--- |
| R22 | 330 | Clocks | Between U39A's output and the crystal and C48: limits the crystal's drive. |
| R23-R26 | 22 | Clocks | Series terminations at the source of HS_CLK, SND_CLK, SER_CLK and PHI2. Each driver's net is now X_SRC. |
| R27 | 22 | AddressDecode | The same for PHI1 (U22's output, PHI1_SRC). |
| R28 | 10K | Clocks | A fixed pull-up on RDY, whether J4 is fitted or not. |
| C86-C89 | 47 uF | Clocks | Bulk capacitance on +5 V. Place them apart: by the slots, the ROM bank and the decode logic. |
| C80-C85 | 100 nF | Sound | Decoupling. C80 at the YM2151, C81 at the YM3012, C82/C83 at U42's +12 V/-12 V, C84/C85 at IC9's. |
| R29 | 0 (or a ferrite bead) | Sound | Joins GNDA to GND once, beside the YM3012. In 1.8C they met only inside the power supply. |
| C90, C91 | 10 uF | Sound | In series with R5 and R4, + toward U42D/U42C. The YM3012's outputs sit at its COM (about 2.5 V); without these, that DC went through the summer to the jack. |
| R30, R31 | 33 | Sound | In series with each output: keep IC9 stable into a cable, and safe from a shorted plug. The op-amp's side of each is AUDIO_L_AMP / AUDIO_R_AMP. |
| C92, C93 | 220 uF non-polar | Sound | AC-couple each output to the jack. |
| R32, R33 | 10K | Sound | Hold the jack's side of C92/C93 at 0 V, so a plug going in doesn't click. |
| RN4 | 10K x 8 | Buffers | Pull-ups on D0-D7, so a read that nothing answers (an empty slot) gives $FF. |
| J30 | 1x3 header | root | The ACIA's CTS: 1-2 from the RS-232 line (as 1.8C), 2-3 always clear (a 3-wire cable). Fit a jumper on 1-2. |
| U26B | (U26's second half) | SharedMemory | It was unplaced, so its inputs floated. Now /2E is tied high and A/B are tied low. |

## Wiring

* U22's spare inputs (pins 11, 15) are tied to GND, and its spare outputs are marked unconnected.
* ATX pin 20 (-5 V on ATX 1.x supplies; not connected on newer ones) feeds the slots' -5V, which nothing fed before.
* The power LED (front panel J10 pin 12, through R2's 470 ohms) is lit from +5 V, not from the ATX supply's
  PWR_OK. PWR_OK is a logic signal, specified to source little current, so an LED loaded it. The main +5 V rail is
  up only while the supply is on, so the LED still means "on". PWR_OK is now unconnected.
* Four wires that went nowhere are removed: two drawn over buses on SharedMemory, and one each on BankedROM and
  ZPMirrorRAM.
* No-connect marks are added on the unused pins: U40's RC and TC, U18's EO, J28 pin 10, and front-panel J10's
  pins 1-4.
* The root sheet's sheet boxes are on the 1.27 mm grid, all the same size.
* The headphone jack (J26) has left on its tip and right on its ring, the usual way. They were swapped.
* The slots' audio inputs are still DC-coupled into the mixer: a card with DC on its outputs should AC-couple
  them itself, or add a capacitor in series with its resistor (R6-R19).

## Not changed

* **J1 pin 4** (the banked RAM header) is still unconnected.
* **The PCB** (`hydra-16.kicad_pcb`) isn't updated. In KiCad, *Tools > Update PCB from Schematic* brings the new
  parts in, to be placed and routed.

## Software

Nothing changes. Reads of an empty slot or an unused I/O port now return $FF every time, instead of whatever the
bus held. The software never relied on that value (vid finds the Vera X by its version register).
