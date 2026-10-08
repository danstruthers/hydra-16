; ****************************************************************************
; math - the math library (docs/design/plans/NUMBERS.md, step 4): sqrt, exp, log, sin, cos, tan, atan, pi and real
; powers on numbers in the stored format, exact when the answer is, else fixed decimals of the precision's significant
; digits (DIGITS: 12 at the start), correctly rounded (danlang's NumMath.cs's).  A library module (HT_LIBRARY) in its
; own bank, called as numbers is (XCALL, its entries spec/numbers.def's math: obj/gen/math_jt.inc), in the same RAM
; bank (r13): the numbers library's registers, state and arena (numbers/nmbank.inc), its integers and its tower
; assembled here too (numbers/nmreg.inc, nmval.inc ...), so that math works them in its own bank.  Its own work uses
; the digits' pages as registers R24-R31 as well (the text's, not math's).

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "numbers.inc"
.include "numbers/nmbank.inc"

            HYX2_LIBRARY "math", "HEADER", "ROM2", 2
.include "math_jt.inc"

.code
.include "numbers/nmcall.inc"
.include "numbers/nmreg.inc"
.include "numbers/nmval.inc"
.include "numbers/nmpow.inc"
.include "mtmac.inc"
.include "mtcore.inc"
.include "mtargs.inc"
.include "mtfun.inc"

; ---- TRIG and ATAN, in the second bank (docs/design/plans/NUMSPEED.md, step 4: the first's room), with their own
; copies of what they use (their variables the first's: NM_COPY)
.segment "CODE2"
.scope b2
NM_COPY         = 1
.include "numbers/nmcall.inc"
.include "numbers/nmreg.inc"
.include "numbers/nmval.inc"
.include "mtcore.inc"
.include "mtargs.inc"
.include "mttrig.inc"
.endscope

; Their entries, from the first bank: NMFAR2 into the second
.code
mt_trig:
            NMFAR2      b2::mt_trig
            rts
mt_atan:
            NMFAR2      b2::mt_atan
            rts

; ---- The arena: the rest of the bank (each call's own: math's variables aren't numbers')
.segment "NUMBANK"
arena:
.assert     arena + 2 * NUM_MAX <= DIGS, lderror, "The arena's too small for an operand and a number's digits"
