; ****************************************************************************
; vt - the console driver's second bank (its first: cons.s): each window's screen, kept as cells in the driver's
; RAM banks, and a VT100 that writes them (docs/plans/WINDOWS.md).

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "cons.inc"

.segment "CODE2"
vt_none:
            rts
