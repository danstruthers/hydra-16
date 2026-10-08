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

            HYX2_LIBRARY "math", "HEADER", "ROM"
.include "math_jt.inc"

.code
.include "numbers/nmcall.inc"
.include "numbers/nmreg.inc"
.include "numbers/nmval.inc"
.include "numbers/nmpow.inc"
.include "mtcore.inc"
.include "mtfun.inc"

; ---- The arena: the rest of the bank (each call's own: math's variables aren't numbers')
.segment "NUMBANK"
arena:
.assert     arena + 2 * NUM_MAX <= DIGS, lderror, "The arena's too small for an operand and a number's digits"
