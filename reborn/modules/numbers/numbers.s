; ****************************************************************************
; numbers - the numbers library (docs/design/plans/NUMBERS.md, step 2): the number system every language shares,
; danlang's and hylang's (integers of any size, fixed decimals, rationals, complex numbers), on numbers in the stored
; format.  A library module (HT_LIBRARY) in its own bank of the paged ROM, run in its caller's task through XCALL:
; its entries (spec/numbers.def, its jump table made from it: obj/gen/numbers_jt.inc) read their operands from the
; caller's memory and write their results there, and work in the RAM bank the caller gives it (r13, nmbank.inc) with
; the zero page they borrow ($70-$7F).

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "numbers.inc"
.include "nmbank.inc"

            HYX2_LIBRARY "numbers", "HEADER", "ROM2", 2
.include "numbers_jt.inc"

.code
.include "nmcall.inc"
.include "nmreg.inc"
.include "nmval.inc"
.include "nmstate.inc"
.include "nmarith.inc"
.include "nmsmall.inc"
.include "nmconv.inc"
.include "nmpow.inc"
.include "nmbits.inc"
.include "nmrand.inc"
.include "nmbytes.inc"
.include "numconst.inc"

; ---- Text, in the second bank (NUMSPEED.md, step 2: the first's room for the arithmetic): its entries there, with
; their own copies of the calls', the registers' and the tower's code (their variables the first's: NM_COPY)
.segment "CODE2"
.scope b2
NM_COPY         = 1
.include "nmcall.inc"
.include "nmreg.inc"
.include "nmval.inc"
.include "nmtext.inc"
.endscope

; Its entries, from the first bank: NMFAR2 into the second
.code
nm_set_base:
            NMFAR2      b2::nm_set_base
            rts
nm_parse:
            NMFAR2      b2::nm_parse
            rts
nm_display:
            NMFAR2      b2::nm_display
            rts
nm_format:
            NMFAR2      b2::nm_format
            rts
.assert     NCONST * CONST_BYTES / 2 < 256 && NCONST * CONST_BYTES .mod 2 = 0, error, "INIT copies the constants in two halves"

; ---- The arena: the rest of the bank
.segment "NUMBANK"
arena:
.assert     arena + 2 * NUM_MAX <= DIGS, lderror, "The arena's too small for an operand and a number's digits"
