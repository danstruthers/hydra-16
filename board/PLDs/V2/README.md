# Hydra-16 V2 programmable logic

Eight GALs hold the glue logic of V2 (rev 2.0D): seven replaced 29 TTL/CMOS packages in rev 2.0C, and GAL C (rev 2.0D)
replaced the 74F191 clock divider with a bus clock generator that can stretch PHI2.
Each `.PLD` is the WinCUPL source, `.jed` the fuse map to program, `.doc` the CUPL listing (expanded equations and fuse plot).

| Ref | File   | Device          | Sheet         | Function |
|-----|--------|-----------------|---------------|----------|
| U81 | GALA   | ATF22V10C-10PU  | AddressDecode | Main address decode from A15..A4: I/O page, VIA, ACIA, $FFFx page, banked RAM/ROM windows, OS ROM chip enable ($E000-$FFFF), slot I/O stretch region ($FF20-$FFEF), page-zero detect |
| U82 | GALB1  | ATF22V10C-10PU  | AddressDecode | $FFFx page: OS ROM output enable (reads of $E000-$FEFF and $FFFA-$FFFD), vector RAM select ($FFFE/F, writes only during PHI2), $FFF0-3 register read selects and write latch strobes |
| U83 | GALB2  | ATF22V10C-10PU  | AddressDecode | Memory write strobe (RWB_M), ZP mirror RAM control, Z register read ($FFF4), stretch register write ($FFF5), DMA request sync (DMAB_S, clocked by PHI1) |
| U88 | GALC   | ATF22V10C-10PU  | Clocks        | Bus clock generator from the 14.318 MHz master: PHI2/PHI1 (7.16 MHz, or 3.58 MHz with J36 2-3), clock stretching for slow regions, SND_CLK 3.58 MHz |
| U84 | GALD   | ATF22V10C-10PU  | WaitStates    | Clock-stretch register $FFF5; tells GAL C how many extra PHI2-high periods the current access needs (N1:N0) |
| U85 | GALE   | ATF22V10C-10PU  | IRQ           | 15-line IRQ priority encoder, level latch (holds while VPB is low), IRQB |
| U86 | GALF   | ATF22V10C-10PU  | IRQ           | Vector RAM address mux: IRQ level while an IRQ is pending, V register otherwise (software interrupts) |
| U87 | GALG   | ATF16V8C-10PU   | WaitStates    | OS ROM paging: $E000-$EFFF = any 4K page (RW6..RW0), $F000-$FFFF = fixed page at ROM offset $1000 |

## Bus timing (GAL C + GAL D)

One period is 69.8 ns (14.318 MHz). A normal cycle is PHI2 low for 1 period, then high for 1 period (7.16 MHz).

Every access to a slow region gets one extra PHI2-low period, so addresses, bank registers and chip selects settle before
PHI2 rises (and before any write strobe starts). The stretch register then adds 0-3 extra PHI2-high periods:

| Bits ($FFF5) | Region                       | Extra PHI2-high periods |
|--------------|------------------------------|-------------------------|
| 1:0          | Banked RAM $8000-$9FFF       | 3 - value               |
| 3:2          | Banked ROM $A000-$DFFF       | 3 - value               |
| 5:4          | Slot I/O $FF20-$FFEF         | 3 - value               |

The register is write-only and reset clears it, so every region starts at 3 extra periods. Main RAM, the OS ROM, the VIA/ACIA
and $FFF0-$FFFF are never stretched. A slow access therefore takes 3 to 6 periods (210-420 ns) instead of 2 (140 ns).
With J36 at 2-3 (3.58 MHz) every phase, including the stretch periods, is twice as long.

Suggested field values at 7.16 MHz (worst-case timing):

| Region     | Value | Why |
|------------|-------|-----|
| Banked RAM | 3     | on-board 45 ns SRAM has ~20 ns to spare with no extra high period; use 2 if memory cards carry 70 ns parts |
| Banked ROM | 3     | 70 ns flash has ~24 ns to spare |
| Slot I/O   | 1     | the YM2151 at $FF40 needs 2 extra periods for its ~180 ns read access; go lower (more periods) for slow cards |

Because PHI2 is stretched, the VIA timers (which count PHI2 cycles) run slower while code or data sits in a stretched region.
Use the YM2151 timers or HS_CLK on a card where wall-clock accuracy matters. Cards can still pull RDY for longer waits;
the CPU then repeats the cycle (each repeat is a new PHI2 strobe).

## Speed grade

-10 parts are specified. The $FFFx decode (U81 -> U82/U83) passes through two GALs (2 x 10 ns) and is the tightest
unstretched path; the OS ROM chip enable is single-level (U81) and its output enable is two-level, which tolerates a 70 ns
flash with about 7 ns to spare (a -55 part is specified). A -15 GAL adds 5-10 ns to these paths: check before substituting.
GAL C runs at 14.318 MHz with registered outputs only, well inside the -10 part's limits.

## Rebuilding and programming

    cupl.exe -jaxf -m1 -u C:\Wincupl\Shared\Atmel.dl GALA.PLD

Program the `.jed` files with any GAL-capable programmer (e.g. TL866II+/T48 with Xgpro or minipro), selecting the exact device from the table.
Label each chip with its reference: the ATF22V10Cs are interchangeable electrically but not in function.

## Verification

The fuse maps were checked with an independent 22V10 JEDEC evaluator. Rev 2.0C GALs were checked against a gate-level model of
the discrete logic they replaced; rev 2.0D changes were checked against behavioural models of the new equations.

| GAL | Rev  | Vectors | Coverage |
|-----|------|---------|----------|
| A   | 2.0D | 4,096   | exhaustive over A15..A4 |
| B1  | 2.0D | 1,024   | exhaustive over all inputs |
| B2  | 2.0C | 902     | address classes and control inputs, plus DMAB_S clocking |
| C   | 2.0D | 60,000  | edge-by-edge against a clock-generator model with random inputs, both speeds; PHI2 waveforms checked per region and N |
| D   | 2.0D | 40,000  | random register writes, resets and region selects |
| E   | 2.0C | 32,864  | exhaustive over the 15 IRQ lines, plus latch/hold cycles |
| F   | 2.0C | 16,384  | exhaustive |

GAL G (a 16V8) was checked from its CUPL expanded equations.
