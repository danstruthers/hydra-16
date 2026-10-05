; ****************************************************************************
; numval.inc - hylang's number tower (phase 5), in its third bank, as danlang's Numbers.cs: integers (numreg.inc's),
; fixed decimals (PK_FIXED: its digits, an integer, and its places, a fixnum: digits / 10^places, with no 0 at its
; end but in 0), rationals (PK_RATIO: its numerator and denominator, integers in lowest terms, the denominator above
; 1) and complex numbers (PK_COMPLEX: its real and imaginary parts, real numbers, the imaginary not 0).
;   A sum, difference or product is complex if either is; else rational if either is; else fixed if either is (its
; places a sum's the more of the two's, a product's both's); else an integer.  A quotient is exact: a rational, an
; integer if it's whole.  A real number is worked as a fraction, n/d in registers (an integer's d 1, a fixed one's
; 10^places), then made the kind those rules give; a complex number by its parts, values on the root stack (n_cpx).
;   Anything made that there's no room for goes back to the entry's abort point (n_nomem).

NK_INT          = 0             ; Number kinds (n_kind's): an integer (a fixnum, a bignum) ...
NK_FIX          = 1             ;   a fixed decimal ...
NK_RAT          = 2             ;   a rational ...
NK_CPX          = 3             ;   a complex number ...
NK_NOT          = $FF           ;   or not a number
OP_ADD          = 0             ; The operations (n_op's)
OP_SUB          = 1
OP_MUL          = 2
OP_DIV          = 3

.bss
n_x:        .res        2                                   ; (An operation's numbers: values; their kinds, places)
n_y:        .res        2
n_k1:       .res        1
n_k2:       .res        1
n_p1:       .res        2
n_p2:       .res        2
n_pl:       .res        2                                   ; (A result's places)
n_opc:      .res        1                                   ; (The operation)
n_v:        .res        2                                   ; (A value loaded, its kind; a complex one's imaginary
n_kv:       .res        1                                   ;   parts)
n_xi:       .res        2
n_yi:       .res        2
n_r2:       .res        1                                   ; (n_pair's second register)
n_base:     .res        1                                   ; (n_cpx's: its parts' place on the root stack, and the
n_pc:       .res        1                                   ;   program's byte it's at)

.segment "CODE3"

; ---- Kinds

; IN: .A, .X = a value.  OUT: .A = its number kind (NK_*: NK_NOT if it isn't a number), Z = 1 if it's an integer.
; Keeps .Y
n_kind:
            lsr
            bcs         @int
            cpx         #IMM_PAGES
            bcc         @not
            lda         pk,x
            sec
            sbc         #PK_BIGNUM
            beq         @rts
            cmp         #PK_COMPLEX - PK_BIGNUM + 1
            bcc         @rts
@not:
            lda         #NK_NOT
            rts
@int:
            lda         #NK_INT
@rts:
            rts

; IN: .A, .X = a real number.  OUT: Z = 1 if it's 0 (the fixnum 0, or a fixed decimal whose digits are)
n_zero_p:
            sta         n_t
            stx         n_t + 1
            cmp         #<FIX(0)
            bne         @fixed
            cpx         #0
            beq         @rts
@fixed:
            lda         n_t
            ldx         n_t + 1
            jsr         n_kind
            cmp         #NK_FIX
            bne         @rts
            lda         n_t
            ldx         n_t + 1
            ldy         #0
            jsr         cell_get
            cmp         #<FIX(0)
            bne         @rts
            cpx         #0
@rts:
            rts

; ---- Reals loaded and made

; The real number .A, .X loaded: register .Y its numerator, .Y + 1 its denominator (positive); n_kv its kind, n_pl
; its places (a fixed decimal's; else 0)
n_load:
            sta         n_v
            stx         n_v + 1
            sty         nrc
            stz         n_pl
            stz         n_pl + 1
            jsr         n_kind
            sta         n_kv
            cmp         #NK_FIX
            beq         @fix
            cmp         #NK_RAT
            beq         @rat
            lda         n_v                                 ; (An integer: d 1)
            ldx         n_v + 1
            jsr         r_load
            inc         nrc
            lda         #1
            ldx         #0
            jmp         r_set16
@fix:
            jsr         @first                              ; (Its digits, and 10 ^ its places)
            lda         n_v
            ldx         n_v + 1
            ldy         #1
            jsr         cell_get
            jsr         unfix
            sta         n_pl
            stx         n_pl + 1
            inc         nrc
            jmp         r_pow10
@rat:
            jsr         @first
            lda         n_v
            ldx         n_v + 1
            ldy         #1
            jsr         cell_get
            inc         nrc
            jmp         r_load
@first:
            lda         n_v
            ldx         n_v + 1
            ldy         #0
            jsr         cell_get
            jmp         r_load

; hv = a cell of kind .Y of the integers in registers .A (its first word) and .X (its second)
n_pair:
            sty         n_kv
            stx         n_r2
            sta         nrc
            jsr         r_store
            lda         hv
            ldx         hv + 1
            jsr         n_rpush
            lda         n_r2
            sta         nrc
            jsr         r_store
            MOVW        eu, hv
            jsr         root_pop
            sta         et
            stx         et + 1
            lda         n_kv
n_make2:
            jsr         make2
            bcs         :+
            rts
:
            jmp         n_nomem

; .A, .X kept on the root stack (no room: n_nomem)
n_rpush:
            jsr         root_push
            bcs         :+
            rts
:
            jmp         n_nomem

; hv = R4 / R5 (R5 positive) in lowest terms: a rational, or an integer if it's whole.  Uses R0-R3, R6: their
; greatest common divisor by Euclid's way (R0, R1, R3 in turn; the quotients R2)
n_rat:
            lda         #4
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            ldx         #0
            stz         r_neg,x
            lda         #5
            sta         nra
            lda         #1
            sta         nrc
            jsr         r_copy
            lda         #0
            sta         nra
            lda         #1
            sta         nrb
            lda         #3
            sta         nrr
@euclid:
            ldx         nrb
            lda         r_len,x
            beq         @gcd
            lda         #2
            sta         nrq
            jsr         r_divmod
            lda         nra                                 ; (a, b, r = b, r, a)
            ldx         nrb
            ldy         nrr
            stx         nra
            sty         nrb
            sta         nrr
            bra         @euclid
@gcd:
            lda         nra                                 ; (It: nra.  1: they're in lowest terms)
            sta         nrc
            jsr         r_one_p
            beq         @made
            lda         nra
            sta         nrb
            lda         #4
            jsr         n_rdiv
            lda         #5
            jsr         n_rdiv
@made:
            lda         #5                                  ; (Whole: an integer)
            sta         nrc
            jsr         r_one_p
            bne         :+
            lda         #4
            sta         nrc
            jmp         r_store
:
            lda         #4
            ldx         #5
            ldy         #PK_RATIO
            jmp         n_pair

; R(.A) = R(.A) / nrb, exactly (the quotient in R2, copied back; the remainder in R6)
n_rdiv:
            sta         nra
            pha
            lda         #2
            sta         nrq
            lda         #6
            sta         nrr
            jsr         r_divmod
            pla
            sta         nrc
            lda         #2
            sta         nra
            jmp         r_copy

; hv = a fixed decimal of R4 * 10^n_pl / R5 (exactly), n_pl places, the zeros at its end dropped.  Uses R0-R3, R6, R7
n_fix:
            lda         #6
            sta         nrc
            lda         n_pl
            ldx         n_pl + 1
            jsr         r_pow10
            lda         #4
            ldx         #6
            ldy         #7
            jsr         n_mulr
            lda         #7
            sta         nra
            lda         #5
            sta         nrb
            lda         #0
            sta         nrq
            lda         #1
            sta         nrr
            jsr         r_divmod
; hv = a fixed decimal of the digits R0, n_pl places, the zeros at its end dropped.  Uses R1
n_fix0:
            lda         n_pl                                ; (Ten divides it: a place fewer)
            ora         n_pl + 1
            beq         @made
            lda         #0
            sta         nra
            lda         #1
            sta         nrc
            jsr         r_copy
            lda         #10
            jsr         r_divsmall
            bne         @made
            lda         #1
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            lda         n_pl
            bne         :+
            dec         n_pl + 1
:
            dec         n_pl
            bra         n_fix0
@made:
            lda         #0
            sta         nrc
            jsr         r_store
            MOVW        et, hv
            lda         n_pl
            ldx         n_pl + 1
            jsr         fix_ax
            sta         eu
            stx         eu + 1
            lda         #PK_FIXED
            jmp         n_make2

; R(.Y) = R(.A) * R(.X)
n_mulr:
            sta         nra
            stx         nrb
            sty         nrc
            jmp         r_mul

; ---- Real numbers

; hv = n_x .A n_y (OP_*), two real numbers (n_y not 0 if it's OP_DIV).  n_x and n_y are read before anything's made,
; so they needn't be roots
n_real:
            sta         n_opc
            lda         n_y
            ldx         n_y + 1
            jsr         n_kind
            sta         n_k2
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            sta         n_k1
            ora         n_k2
            bne         @fraction
            lda         n_opc
            cmp         #OP_DIV
            beq         @fraction
            lda         #0                                  ; (Two integers: their sum, difference or product)
            sta         nrc
            lda         n_x
            ldx         n_x + 1
            jsr         r_load
            lda         #2
            sta         nrc
            lda         n_y
            ldx         n_y + 1
            jsr         r_load
            lda         #0
            sta         nra
            lda         #2
            sta         nrb
            lda         #4
            sta         nrc
            jsr         n_intop
            jmp         r_store
@fraction:
            lda         n_x                                 ; (Fractions: R0/R1 and R2/R3)
            ldx         n_x + 1
            ldy         #0
            jsr         n_load
            MOVW        n_p1, n_pl
            lda         n_y
            ldx         n_y + 1
            ldy         #2
            jsr         n_load
            MOVW        n_p2, n_pl
            lda         n_opc
            cmp         #OP_MUL
            bcs         @muldiv
            lda         #0                                  ; (a/b + c/d: (ad + cb) / bd)
            ldx         #3
            ldy         #4
            jsr         n_mulr
            lda         #2
            ldx         #1
            ldy         #6
            jsr         n_mulr
            lda         #4
            sta         nra
            sta         nrc
            lda         #6
            sta         nrb
            jsr         n_intop
            lda         #1
            ldx         #3
            ldy         #5
            jsr         n_mulr
            lda         n_p1                                ; (Its places: the more of the two's)
            ldx         n_p1 + 1
            cpx         n_p2 + 1
            bne         :+
            cmp         n_p2
:
            bcs         :+
            lda         n_p2
            ldx         n_p2 + 1
:
            sta         n_pl
            stx         n_pl + 1
            bra         @kind
@muldiv:
            bne         @div
            lda         #0                                  ; (a/b * c/d: ac / bd)
            ldx         #2
            ldy         #4
            jsr         n_mulr
            lda         #1
            ldx         #3
            ldy         #5
            jsr         n_mulr
            clc                                             ; (Its places: both's)
            lda         n_p1
            adc         n_p2
            sta         n_pl
            lda         n_p1 + 1
            adc         n_p2 + 1
            sta         n_pl + 1
            bra         @kind
@div:
            lda         #0                                  ; (a/b / c/d: ad / bc, the sign on top)
            ldx         #3
            ldy         #4
            jsr         n_mulr
            lda         #1
            ldx         #2
            ldy         #5
            jsr         n_mulr
            ldx         #5
            lda         r_neg,x
            beq         @rat
            jsr         r_negate
            ldx         #4
            jsr         r_negate
            bra         @rat
@kind:
            lda         n_k1                                ; (A rational if either is; else fixed)
            cmp         #NK_RAT
            beq         @rat
            lda         n_k2
            cmp         #NK_RAT
            beq         @rat
            jmp         n_fix
@rat:
            jmp         n_rat

; nrc = nra + nrb (n_opc OP_ADD), nra - nrb (OP_SUB) or nra * nrb (OP_MUL)
n_intop:
            lda         n_opc
            beq         @add
            cmp         #OP_SUB
            beq         @sub
            jmp         r_mul
@add:
            jmp         r_add
@sub:
            jmp         r_sub

; hv = -n_x, a real number
n_rneg:
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            cmp         #NK_FIX
            beq         @pair
            cmp         #NK_RAT
            beq         @pair
            lda         #0                                  ; (An integer)
            sta         nrc
            lda         n_x
            ldx         n_x + 1
            jsr         r_load
            ldx         #0
            jsr         r_negate
            jmp         r_store
@pair:
            clc                                             ; (Its first part negated, its second as it is)
            adc         #PK_BIGNUM
            sta         n_kv
            lda         #0
            sta         nrc
            lda         n_x
            ldx         n_x + 1
            ldy         #0
            jsr         cell_get
            jsr         r_load
            ldx         #0
            jsr         r_negate
            jsr         r_store
            MOVW        et, hv
            lda         n_x
            ldx         n_x + 1
            ldy         #1
            jsr         cell_get
            sta         eu
            stx         eu + 1
            lda         n_kv
            jmp         n_make2

; .A = n_x against n_y, two real numbers: $FF less, 0 the same, 1 more (as fractions: ad against cb)
n_rcmp:
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            sta         n_k1
            lda         n_y
            ldx         n_y + 1
            jsr         n_kind
            ora         n_k1
            bne         @fraction
            lda         #0
            sta         nrc
            lda         n_x
            ldx         n_x + 1
            jsr         r_load
            lda         #2
            sta         nrc
            lda         n_y
            ldx         n_y + 1
            jsr         r_load
            lda         #0
            ldx         #2
            bra         @cmp
@fraction:
            lda         n_x
            ldx         n_x + 1
            ldy         #0
            jsr         n_load
            lda         n_y
            ldx         n_y + 1
            ldy         #2
            jsr         n_load
            lda         #0
            ldx         #3
            ldy         #4
            jsr         n_mulr
            lda         #2
            ldx         #1
            ldy         #5
            jsr         n_mulr
            lda         #4
            ldx         #5
@cmp:
            sta         nra
            stx         nrb
            jmp         r_cmp

; .A = the real number n_x's sign: $FF negative, 0 zero, 1 positive
n_rsign:
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            beq         @int
            lda         n_x                                 ; (A fixed decimal's or a rational's: its first part's)
            ldx         n_x + 1
            ldy         #0
            jsr         cell_get
            bra         @load
@int:
            lda         n_x
            ldx         n_x + 1
@load:
            ldy         #0
            sty         nrc
            jsr         r_load
            ldx         #0
            lda         r_len,x
            beq         @rts
            lda         r_neg,x
            bne         @rts
            lda         #1
@rts:
            rts

; ---- Any numbers

; hv = n_x .A n_y (OP_*), two numbers (n_y not 0 if it's OP_DIV): a complex one's by its parts (n_cpx)
n_op:
            sta         n_opc
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            cmp         #NK_CPX
            beq         @complex
            lda         n_y
            ldx         n_y + 1
            jsr         n_kind
            cmp         #NK_CPX
            beq         @complex
            lda         n_opc
            jmp         n_real
@complex:
            ldx         n_opc
            lda         cpx_progs,x
            jmp         n_cpx

; hv = -n_x, a number (a complex one's: 0 less it)
n_neg:
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            cmp         #NK_CPX
            beq         :+
            jmp         n_rneg
:
            MOVW        n_y, n_x
            lda         #<FIX(0)
            sta         n_x
            stz         n_x + 1
            lda         #CPX_SUB
            jmp         n_cpx

; .A = n_x against n_y, two numbers: $FF less, 0 the same, 1 more (complex ones by their real parts, then their
; imaginary)
n_cmp:
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            cmp         #NK_CPX
            beq         @complex
            lda         n_y
            ldx         n_y + 1
            jsr         n_kind
            cmp         #NK_CPX
            beq         @complex
            jmp         n_rcmp
@complex:
            lda         n_x                                 ; (Their real parts, their imaginary kept)
            ldx         n_x + 1
            jsr         n_parts
            sta         n_x
            stx         n_x + 1
            MOVW        n_xi, n_v
            lda         n_y
            ldx         n_y + 1
            jsr         n_parts
            sta         n_y
            stx         n_y + 1
            MOVW        n_yi, n_v
            jsr         n_rcmp
            bne         @rts
            MOVW        n_x, n_xi
            MOVW        n_y, n_yi
            jmp         n_rcmp
@rts:
            rts

; IN: .A, .X = a number.  OUT: .A, .X = its real part; n_v its imaginary (0, a real number's)
n_parts:
            sta         n_v
            stx         n_v + 1
            jsr         n_kind
            cmp         #NK_CPX
            beq         @complex
            lda         n_v
            ldx         n_v + 1
            ldy         #<FIX(0)
            sty         n_v
            stz         n_v + 1
            rts
@complex:
            lda         n_v
            ldx         n_v + 1
            ldy         #1
            jsr         cell_get
            pha
            phx
            lda         n_v
            ldx         n_v + 1
            ldy         #0
            jsr         cell_get
            ply
            sty         n_v + 1
            ply
            sty         n_v
            rts

; ---- Complex numbers: their parts worked on the root stack

; hv = n_x op n_y, either complex, by the program at cpx_code + .A: each byte pushes a part (CP_XR, CP_XI, CP_YR,
; CP_YI: x's and y's) or a value worked out before (CP_AT + k: the kth), or does an operation (OP_*) on the two on
; top, till CP_END, which makes the complex number of the two on top (a real one if its imaginary part is 0)
CP_XR           = $10
CP_XI           = $11
CP_YR           = $12
CP_YI           = $13
CP_AT           = $20
CP_END          = $FF

n_cpx:
            sta         n_pc
            lda         rsp
            sta         n_base
            lda         n_x
            ldx         n_x + 1
            jsr         @parts
            lda         n_y
            ldx         n_y + 1
            jsr         @parts
@step:
            ldx         n_pc
            inc         n_pc
            lda         cpx_code,x
            cmp         #CP_END
            beq         @end
            cmp         #CP_XR
            bcs         @push
            pha                                             ; (An operation, on the two on top)
            jsr         n_rpop2
            pla
            jsr         n_real
            lda         hv
            ldx         hv + 1
            jsr         n_rpush
            bra         @step
@push:
            sec                                             ; (A part, or a value worked out: its place)
            sbc         #CP_XR
            cmp         #CP_AT - CP_XR
            bcc         :+
            sbc         #CP_AT - CP_XR - 4
:
            clc
            adc         n_base
            tax
            lda         rs_hi,x
            pha
            lda         rs_lo,x
            plx
            jsr         n_rpush
            bra         @step
@end:
            jsr         n_rpop2                             ; (The real and imaginary parts)
            lda         n_base
            sta         rsp
            lda         n_y
            ldx         n_y + 1
            jsr         n_zero_p
            bne         @complex
            MOVW        hv, n_x
            rts
@complex:
            MOVW        et, n_x
            MOVW        eu, n_y
            lda         #PK_COMPLEX
            jmp         n_make2
@parts:
            jsr         n_parts
            jsr         n_rpush
            lda         n_v
            ldx         n_v + 1
            jmp         n_rpush

; n_x, n_y = the two values on the root stack's top, taken off
n_rpop2:
            jsr         root_pop
            sta         n_y
            stx         n_y + 1
            jsr         root_pop
            sta         n_x
            stx         n_x + 1
            rts

.segment "RODATA3"
; The complex operations: their programs (x = a + bi, y = c + di)
cpx_progs:  .byte       CPX_ADD, CPX_SUB, CPX_MUL, CPX_DIV
cpx_code:
CPX_ADD = * - cpx_code
            .byte       CP_XR, CP_YR, OP_ADD, CP_XI, CP_YI, OP_ADD, CP_END
CPX_SUB = * - cpx_code
            .byte       CP_XR, CP_YR, OP_SUB, CP_XI, CP_YI, OP_SUB, CP_END
CPX_MUL = * - cpx_code                                      ; ((ac - bd) + (ad + bc)i)
            .byte       CP_XR, CP_YR, OP_MUL, CP_XI, CP_YI, OP_MUL, OP_SUB
            .byte       CP_XR, CP_YI, OP_MUL, CP_XI, CP_YR, OP_MUL, OP_ADD, CP_END
CPX_DIV = * - cpx_code                                      ; ((ac + bd)/n + (bc - ad)/n i, n = cc + dd: the 4th)
            .byte       CP_YR, CP_YR, OP_MUL, CP_YI, CP_YI, OP_MUL, OP_ADD
            .byte       CP_XR, CP_YR, OP_MUL, CP_XI, CP_YI, OP_MUL, OP_ADD, CP_AT, OP_DIV
            .byte       CP_XI, CP_YR, OP_MUL, CP_XR, CP_YI, OP_MUL, OP_SUB, CP_AT, OP_DIV, CP_END

.segment "CODE3"
