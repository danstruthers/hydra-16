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

            HYX2_LIBRARY "numbers", "HEADER", "ROM"
.include "numbers_jt.inc"

; ---- The entries not written yet: NE_TODO
nm_parse        = nm_todo
nm_display      = nm_todo
nm_format       = nm_todo
nm_add          = nm_todo
nm_sub          = nm_todo
nm_mul          = nm_todo
nm_div          = nm_todo
nm_neg          = nm_todo
nm_abs          = nm_todo
nm_cmp          = nm_todo
nm_kind         = nm_todo
nm_idiv         = nm_todo
nm_gcd          = nm_todo
nm_pow          = nm_todo
nm_truncate     = nm_todo
nm_floor        = nm_todo
nm_round        = nm_todo
nm_to_fixed     = nm_todo
nm_to_rational  = nm_todo
nm_numerator    = nm_todo
nm_denominator  = nm_todo
nm_complex      = nm_todo
nm_part         = nm_todo
nm_from_int     = nm_todo
nm_to_int       = nm_todo
nm_bits         = nm_todo
nm_random       = nm_todo
nm_fib          = nm_todo

.code
nm_todo:
            jsr         nm_begin
            lda         #NE_TODO
            jmp         nm_fail

.include "nmcall.inc"
.include "nmstate.inc"
.include "nmbytes.inc"
