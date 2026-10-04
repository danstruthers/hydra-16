# Hydra-16 V2 programmable logic

Seven GALs replace the discrete glue logic of V2 rev 2.0B/2.0C (29 TTL/CMOS packages + their bypass caps).
Each `.PLD` is the WinCUPL source, `.jed` the fuse map to program, `.doc` the CUPL listing (expanded equations and fuse plot).

| Ref | File   | Device          | Sheet         | Function |
|-----|--------|-----------------|---------------|----------|
| U81 | GALA   | ATF22V10C-10PU  | AddressDecode | Main address decode from A15..A4: I/O page, VIA, ACIA, $FFFx page, banked RAM/ROM windows, OS ROM, I/O wait region, page-zero detect |
| U82 | GALB1  | ATF22V10C-10PU  | AddressDecode | $FFFx page: OS ROM select (vectors excluded), vector RAM select ($FFFE/F), $FFF0-3 register read selects and write latch strobes |
| U83 | GALB2  | ATF22V10C-10PU  | AddressDecode | Write qualification (nWR_Q), ZP mirror RAM control, Z register read ($FFF4), wait-state register write ($FFF5), DMA request sync (DMAB_S, clocked by PHI1) |
| U84 | GALD   | ATF22V10C-10PU  | WaitStates    | Wait-state register $FFF5 and counter; pulls CPU_RDY low while stalling |
| U85 | GALE   | ATF22V10C-10PU  | IRQ           | 15-line IRQ priority encoder, level latch (holds while VPB is low), IRQB |
| U86 | GALF   | ATF22V10C-10PU  | IRQ           | Vector RAM address mux: IRQ level during an IRQ vector read, V register otherwise |
| U87 | GALG   | ATF16V8C-10PU   | WaitStates    | OS ROM bank lines: W = RW for $E000-$EFFF, 0 for $F000-$FFFF |

## Wait-state register ($FFF5, write-only, cleared by reset)

| Bits | Region                    | Wait states   |
|------|---------------------------|---------------|
| 1:0  | Banked RAM $8000-$9FFF    | 3 - value     |
| 3:2  | Banked ROM $A000-$DFFF    | 3 - value     |
| 5:4  | I/O $FF20-$FFFF           | 3 - value     |

Reset leaves 3 waits in every region. Main RAM, the OS ROM and the VIA/ACIA never get waits.

## Speed grade

-10 parts are specified. The $FFFx decode (U81 -> U82/U83) and the wait-state region decode (U81 -> U84) pass through two GALs (2 x 10 ns).
A -15 part adds 10 ns to those paths. Check it against the CPU address-setup time before substituting.

## Rebuilding and programming

    cupl.exe -jaxf -m1 -u C:\Wincupl\Shared\Atmel.dl GALA.PLD

Program the `.jed` files with any GAL-capable programmer (e.g. TL866II+/T48 with Xgpro or minipro), selecting the exact device from the table.
Label each chip with its reference: the ATF22V10Cs are interchangeable electrically but not in function.

## Verification

The fuse maps were checked against a gate-level model of the discrete logic they replace, using an independent 22V10 JEDEC evaluator:

| GAL | Vectors | Coverage |
|-----|---------|----------|
| A   | 4,096   | exhaustive over A15..A4 |
| B1  | 1,792   | chained through GAL A: every address class x A3..A0 x RWB/PHI2/RESB |
| B2  | 902     | address classes and control inputs, plus DMAB_S clocking |
| E   | 32,864  | exhaustive over the 15 IRQ lines, plus latch/hold cycles |
| F   | 16,384  | exhaustive |
| D   | 23,412  | random bus-cycle sequences, including register writes and reset |

GAL G (a 16V8) was checked from its CUPL expanded equations.
