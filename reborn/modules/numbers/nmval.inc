; ****************************************************************************
; nmval.inc - the number tower (hylang's numval.inc, as danlang's Numbers.cs), on numbers in the stored format: a
; value is a number's address (in the arena, an operand's copy, or a constant in this bank of the paged ROM).
; Integers (nmreg.inc's), fixed decimals (digits / 10^places, with no 0 at the digits' end but in 0), rationals (a
; numerator and a denominator, in lowest terms, the denominator above 1) and complex numbers (a real and an imaginary
; part, real numbers, the imaginary not 0).
;   A sum, difference or product is complex if either is; else rational if either is; else fixed if either is (its
; places a sum's the more of the two's, a product's both's); else an integer.  A quotient is exact: a rational, an
; integer if it's whole.  A real number is worked as a fraction, n/d in registers (an integer's d 1, a fixed one's
; 10^places), then written the kind those rules give; a complex number by its parts, values on a stack (n_cpx).

OP_ADD          = 0             ; The operations (n_op's)
OP_SUB          = 1
OP_MUL          = 2
OP_DIV          = 3

.segment "NUMBANK"
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
n_base:     .res        1                                   ; (n_cpx's: its parts' place on the stack, the program's
n_pc:       .res        1                                   ;   byte it's at)
n_sz:       .res        2                                   ; (A number's length, as it's measured)
n_den:      .res        2                                   ; (A rational's denominator, as it's loaded)
n_free:     .res        2                                   ; (The arena's top, once an operation's operands are read)
n_gk:       .res        2                                   ; (n_gcd's twos; the twos each had)
n_tz1:      .res        2
n_tz2:      .res        2
n_v1:       .res        2                                   ; (Two results' values, nm_out2's)
n_v2:       .res        2
s_lo:       .res        NSTACK                              ; The complex numbers' machine's stack: values, and
s_hi:       .res        NSTACK                              ;   whether each is the arena's, made by the machine
s_own:      .res        NSTACK                              ;   (its room given back as it's used)
s_n:        .res        1

.code

; ---- Kinds, parts, lengths

; IN: .A, .X = a value.  OUT: .A = its kind (NK_*), Z = 1 if it's an integer.  Keeps .X, .Y
n_kind:
            sta         nt
            stx         nt + 1
            lda         (nt)
            cmp         #NT_FIXED
            bcc         @int
            cmp         #NT_RATIONAL
            bcc         @fix
            beq         @rat
            lda         #NK_COMPLEX
            rts
@fix:
            lda         #NK_FIXED
            rts
@rat:
            lda         #NK_RATIONAL
            rts
@int:
            lda         #NK_INT
            rts

; IN: .A, .X = a real number.  OUT: Z = 1 if it's 0 (the integer 0, or a fixed decimal whose digits are)
n_zero_p:
            sta         nt
            stx         nt + 1
            lda         (nt)
            beq         @rts
            cmp         #NT_FIXED                           ; (Not a fixed decimal: Z = 0)
            bcc         @no
            cmp         #NT_FIXED_LONG + 1
            bcs         @no
            lda         nt
            ldx         nt + 1
            jsr         n_first
            sta         nt
            stx         nt + 1
            lda         (nt)
@rts:
            rts
@no:
            lda         #1
            rts

; IN: .A, .X = a fixed decimal or a rational.  OUT: .A, .X = its first part (its digits, its numerator); nt its
; second's (a rational's denominator), n_pl a fixed decimal's places
n_first:
            sta         nt
            stx         nt + 1
            stz         n_pl + 1
            stz         n_pl
            lda         (nt)
            cmp         #NT_FIXED_LONG
            beq         @long
            bcs         :+                                  ; (A rational: no places)
            and         #$0F
            sta         n_pl
:
            ldy         #1
            bra         @part
@long:
            ldy         #1
            lda         (nt),y
            sta         n_pl
            iny
            lda         (nt),y
            sta         n_pl + 1
            iny
@part:
            tya
            clc
            adc         nt
            pha
            lda         nt + 1
            adc         #0
            tax
            pla
            pha
            phx
            jsr         n_isize                             ; (nt: past it, the second part)
            plx
            pla
            rts

; IN: .A, .X = a number.  OUT: .A, .X = its real part; n_v its imaginary (0, a real number's)
n_parts:
            sta         n_v
            stx         n_v + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            beq         @complex
            lda         n_v
            ldx         n_v + 1
            ldy         #<n_zero
            sty         n_v
            ldy         #>n_zero
            sty         n_v + 1
            rts
@complex:
            lda         n_v                                 ; (Its real part after the tag, its imaginary after that)
            clc
            adc         #1
            sta         n_v
            bcc         :+
            inc         n_v + 1
:
            lda         n_v
            ldx         n_v + 1
            jsr         n_size
            pha
            phx
            clc
            adc         n_v
            tay
            pla
            adc         n_v + 1
            tax
            pla
            lda         n_v                                 ; (.A, .X its real part; n_v its imaginary)
            pha
            lda         n_v + 1
            sty         n_v
            stx         n_v + 1
            tax
            pla
            rts

; IN: .A, .X = a number.  OUT: .A, .X = its length (bytes)
n_size:
            sta         nt
            stx         nt + 1
            stz         n_sz
            stz         n_sz + 1
            lda         (nt)
            cmp         #NT_COMPLEX
            bne         @real
            lda         #1
            jsr         sz_n
            jsr         sz_real
@real:
            jsr         sz_real
            lda         n_sz
            ldx         n_sz + 1
            rts

; IN: .A, .X = an integer.  OUT: .A, .X = its length (1 to 257)
n_isize:
            sta         nt
            stx         nt + 1
            stz         n_sz
            stz         n_sz + 1
            jsr         sz_int
            lda         n_sz
            ldx         n_sz + 1
            rts

; A real number's length counted, nt moved past it
sz_real:
            lda         (nt)
            cmp         #NT_FIXED
            bcc         sz_int
            cmp         #NT_FIXED + 16
            bcs         @long
            lda         #1
            jsr         sz_n
            bra         sz_int
@long:
            cmp         #NT_FIXED_LONG
            bne         @rat
            lda         #3
            jsr         sz_n
            bra         sz_int
@rat:
            lda         #1
            jsr         sz_n
            jsr         sz_int
; An integer's
sz_int:
            lda         (nt)
            cmp         #NT_POS
            bcc         @one
            cmp         #NT_LONG_POS
            bcs         @long
            and         #$0F
            clc
            adc         #2
            bra         sz_n
@one:
            lda         #1
            bra         sz_n
@long:
            ldy         #1
            lda         (nt),y
            pha
            lda         #2
            jsr         sz_n
            pla
; .A more counted, nt moved on as many
sz_n:
            pha
            clc
            adc         n_sz
            sta         n_sz
            bcc         :+
            inc         n_sz + 1
:
            pla
            clc
            adc         nt
            sta         nt
            bcc         :+
            inc         nt + 1
:
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
            cmp         #NK_FIXED
            beq         @fix
            cmp         #NK_RATIONAL
            beq         @rat
            lda         n_v                                 ; (An integer: d 1)
            ldx         n_v + 1
            jsr         r_load
            inc         nrc
            lda         #1
            ldx         #0
            jmp         r_set16
@fix:
            lda         n_v                                 ; (Its digits, and 10 ^ its places)
            ldx         n_v + 1
            jsr         n_first
            jsr         r_load
            inc         nrc
            lda         n_pl
            ldx         n_pl + 1
            jmp         r_pow10
@rat:
            lda         n_v
            ldx         n_v + 1
            jsr         n_first
            ldy         nt
            sty         n_den
            ldy         nt + 1
            sty         n_den + 1
            jsr         r_load
            inc         nrc
            lda         n_den
            ldx         n_den + 1
            jmp         r_load

; hv = R4 / R5 (R5 positive) in lowest terms: a rational, or an integer if it's whole.  Uses R0-R2, R6: their
; greatest common divisor (n_gcd)
n_rat:
            ldx         #5
            lda         r_len,x
            bne         :+
            lda         #NE_DIV0
            jmp         nm_fail
:
            jsr         n_gcd
            beq         @made
            lda         #0                                  ; (Each divided by its odd part, R0 ...
            sta         nrc
            jsr         r_one_p
            beq         @twos
            lda         #0
            sta         nrb
            lda         #4
            jsr         n_rdiv
            lda         #5
            jsr         n_rdiv
@twos:
            lda         n_gk                                ;   and its twos)
            ora         n_gk + 1
            beq         @made
            lda         #4
            sta         nrc
            lda         n_gk
            ldx         n_gk + 1
            jsr         r_shrax
            lda         #5
            sta         nrc
            lda         n_gk
            ldx         n_gk + 1
            jsr         r_shrax
@made:
; hv = R4 / R5 written: an integer if R5 is 1, else a rational
n_ratw:
            lda         #5                                  ; (Whole: an integer)
            sta         nrc
            jsr         r_one_p
            bne         :+
            lda         #4
            sta         nrc
            jmp         r_store
:
            lda         #4                                  ; (A rational: its tag, then the two)
            sta         nrc
            jsr         r_c
            jsr         r_normc
            ldx         #4                                  ; (Room: its tag, two integers' tags and lengths)
            lda         r_len,x
            ldx         #5
            clc
            adc         r_len,x
            ldx         #0
            bcc         :+
            inx
:
            clc
            adc         #2
            bcc         :+
            inx
:
            jsr         a_room
            lda         #NT_RATIONAL
            jsr         a_put
            jsr         r_write
            lda         #5
            sta         nrc
            jsr         r_c
            jsr         r_normc
            jmp         r_write

; The greatest common divisor of |R4| and |R5| (R5 not 0): R0 its odd part, n_gk its twos (the divisor R0 * 2^n_gk).
; The binary way: each's twos counted and shifted out (the fewer the divisor's), then the less taken from the more
; and its twos shifted out, till they're the same: shifts and subtractions.  But while one is far longer than the
; other (by more than a third of the shorter's bytes), a division: the longer made its remainder by the shorter (a
; division costs as their lengths multiplied, the subtractions as the longer's length for each bit they're apart).
; OUT: Z = 1 if it's 1.  Uses R1-R3
n_gcd:
            lda         #4                                  ; (R0 = |R4|, R1 = |R5|; R4 0: R0 = R1)
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
            ldx         #1
            stz         r_neg,x
            ldx         #0
            lda         r_len,x
            bne         :+
            lda         #1
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
:
            lda         #0                                  ; (Each's twos, the fewer the divisor's)
            sta         nrc
            jsr         r_tz
            sta         n_tz1
            stx         n_tz1 + 1
            jsr         r_shrax
            lda         #1
            sta         nrc
            jsr         r_tz
            sta         n_tz2
            stx         n_tz2 + 1
            jsr         r_shrax
            lda         n_tz1
            ldx         n_tz1 + 1
            cpx         n_tz2 + 1
            bne         :+
            cmp         n_tz2
:
            bcc         :+
            lda         n_tz2
            ldx         n_tz2 + 1
:
            sta         n_gk
            stx         n_gk + 1
@step:
            ldx         #0                                  ; (One far longer: its remainder by the other)
            lda         r_len,x
            ldx         #1
            sec
            sbc         r_len,x
            beq         @sub
            bcc         :+
            jsr         n_far                               ; (R0 the longer)
            bcc         @sub
            lda         #0
            ldx         #1
            bra         @mod
:
            eor         #$FF                                ; (R1 the longer)
            inc         a
            ldx         #0
            jsr         n_far
            bcc         @sub
            lda         #1
            ldx         #0
@mod:
            sta         nra                                 ; (R(a) = R(a) mod R(b))
            stx         nrb
            lda         #2
            sta         nrq
            lda         #3
            sta         nrr
            jsr         r_divmod
            lda         #3                                  ; (The remainder back to R(a))
            sta         nra
            lda         nrb
            eor         #1
            sta         nrc
            jsr         r_copy
            ldx         nrc
            lda         r_len,x
            bne         @odd
            lda         nrb                                 ; (0: the divisor R(b), to R0)
            beq         @same
            lda         #1
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            bra         @same
@sub:
            lda         #0                                  ; (Both odd: the less from the more)
            sta         nra
            lda         #1
            sta         nrb
            jsr         r_ucmp
            beq         @same
            cmp         #1
            bne         @less
            lda         #0
            sta         nrc
            jsr         r_usub
            bra         @odd
@less:
            lda         #1
            sta         nra
            sta         nrc
            lda         #0
            sta         nrb
            jsr         r_usub
@odd:
            jsr         r_tz                                ; (Its twos out: odd again)
            jsr         r_shrax
            jmp         @step
@same:
            lda         n_gk
            ora         n_gk + 1
            bne         @rts
            lda         #0
            sta         nrc
            jmp         r_one_p
@rts:
            rts

; C = 1 if .A (how much longer one is) is more than a third of register .X's length (the shorter's)
n_far:
            cmp         #86
            bcs         @far
            sta         n_t
            asl
            adc         n_t
            cmp         r_len,x
            rts
@far:
            rts

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
            lda         #0                                  ; (Its tag and places, then its digits)
            sta         nrc
            jsr         r_c
            jsr         r_normc
            ldx         #0                                  ; (Room: its tag and places, its digits' tag and length)
            lda         r_len,x
            ldx         #0
            clc
            adc         #2
            bcc         :+
            inx
:
            jsr         a_room
            lda         n_pl + 1
            bne         @long
            lda         n_pl
            cmp         #16
            bcs         @long
            ora         #NT_FIXED
            jsr         a_put
            jmp         r_write
@long:
            lda         #NT_FIXED_LONG
            jsr         a_put
            lda         n_pl
            jsr         a_put
            lda         n_pl + 1
            jsr         a_put
            jmp         r_write

; R(.Y) = R(.A) * R(.X)
n_mulr:
            sta         nra
            stx         nrb
            sty         nrc
            jmp         r_mul

; ---- Real numbers

; hv = n_x .A n_y (OP_*), two real numbers (n_y not 0 if it's OP_DIV).  n_x and n_y are read before anything's made,
; so a number made may take their room
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
            lda         n_opc                               ; (Integers and fixed decimals, not a quotient: n_fixop)
            cmp         #OP_DIV
            beq         :+
            lda         n_k1
            cmp         #NK_RATIONAL
            beq         :+
            lda         n_k2
            cmp         #NK_RATIONAL
            beq         :+
            jmp         n_fixop
:
            lda         n_x                                 ; (Fractions: R0/R1 and R2/R3)
            ldx         n_x + 1
            ldy         #0
            jsr         n_load
            MOVR        n_p1, n_pl
            lda         n_y
            ldx         n_y + 1
            ldy         #2
            jsr         n_load
            MOVR        n_p2, n_pl
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
            clc                                             ; (Its places: both's; past 65535, too big)
            lda         n_p1
            adc         n_p2
            sta         n_pl
            lda         n_p1 + 1
            adc         n_p2 + 1
            sta         n_pl + 1
            bcc         @kind
            jmp         n_big
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
@rat:
            jmp         n_rat                               ; (A rational: one of them is)

; hv = n_x .A n_y (OP_ADD, OP_SUB, OP_MUL), each an integer or a fixed decimal, one fixed at least: their digits (R0,
; R2) scaled to the same places for a sum or a difference (the more of the two's), their places added for a product
n_fixop:
            jsr         n_scaled
            lda         n_opc
            cmp         #OP_MUL
            beq         @mul
            lda         #0
            sta         nra
            sta         nrc
            lda         #2
            sta         nrb
            jsr         n_intop
            jmp         n_fix0
@mul:
            lda         n_tz1                               ; (n_scaled left them as they were: places added)
            clc
            adc         n_tz2
            sta         n_pl
            lda         n_tz1 + 1
            adc         n_tz2 + 1
            sta         n_pl + 1
            bcc         :+
            jmp         n_big
:
            lda         #0
            ldx         #2
            ldy         #4
            jsr         n_mulr
            lda         #4
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            jmp         n_fix0

; R0 = n_x's digits, R2 = n_y's (integers or fixed decimals); n_tz1, n_tz2 their places.  Unless n_opc is OP_MUL,
; the one with fewer places scaled to the other's (n_pl the more)
n_scaled:
            lda         n_x
            ldx         n_x + 1
            ldy         #0
            jsr         n_digits
            MOVR        n_tz1, n_pl
            lda         n_y
            ldx         n_y + 1
            ldy         #2
            jsr         n_digits
            MOVR        n_tz2, n_pl
            lda         n_opc
            cmp         #OP_MUL
            beq         @rts
            lda         n_tz1                               ; (n_pl: the more; the other scaled by the difference)
            ldx         n_tz1 + 1
            cpx         n_tz2 + 1
            bne         :+
            cmp         n_tz2
:
            bcc         @y
            sta         n_pl
            stx         n_pl + 1
            lda         #2
            sta         nrc
            lda         n_tz1
            sec
            sbc         n_tz2
            pha
            lda         n_tz1 + 1
            sbc         n_tz2 + 1
            tax
            pla
            jmp         r_scale10
@y:
            MOVR        n_pl, n_tz2
            lda         #0
            sta         nrc
            lda         n_tz2
            sec
            sbc         n_tz1
            pha
            lda         n_tz2 + 1
            sbc         n_tz1 + 1
            tax
            pla
            jmp         r_scale10
@rts:
            rts

; Register .Y = the digits of .A, .X, an integer (itself) or a fixed decimal; n_pl its places
n_digits:
            sty         nrc
            stz         n_pl
            stz         n_pl + 1
            sta         nt
            stx         nt + 1
            lda         (nt)
            cmp         #NT_FIXED
            lda         nt
            bcc         :+                                  ; (An integer)
            jsr         n_first
:
            jmp         r_load

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
            beq         @int
            cmp         #NK_FIXED
            beq         @fix
            lda         n_x                                 ; (A rational: its numerator negated, its denominator:
            ldx         n_x + 1                             ;   in lowest terms still)
            ldy         #4
            jsr         n_load
            ldx         #4
            jsr         r_negate
            jmp         n_ratw
@fix:
            lda         n_x                                 ; (A fixed decimal: its digits negated, its places)
            ldx         n_x + 1
            jsr         n_first
            ldy         #0
            sty         nrc
            jsr         r_load
            ldx         #0
            jsr         r_negate
            jmp         n_fix0
@int:
            lda         #0
            sta         nrc
            lda         n_x
            ldx         n_x + 1
            jsr         r_load
            ldx         #0
            jsr         r_negate
            jmp         r_store

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
            lda         n_k1                                ; (No rational: their digits, scaled to the same places)
            cmp         #NK_RATIONAL
            beq         :+
            ldx         n_y
            stx         nt
            ldx         n_y + 1
            stx         nt + 1
            lda         (nt)
            cmp         #NT_RATIONAL
            beq         :+
            lda         #OP_ADD
            sta         n_opc
            jsr         n_scaled
            lda         #0
            ldx         #2
            bra         @cmp
:
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

; .A = the real number n_x's sign: $FF negative, 0 zero, 1 positive (its integer's, its digits', its numerator's)
n_rsign:
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            php
            lda         n_x
            ldx         n_x + 1
            plp
            beq         @load
            jsr         n_first
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
            cmp         #NK_COMPLEX
            beq         @complex
            lda         n_y
            ldx         n_y + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            beq         @complex
            lda         n_opc
            jmp         n_real
@complex:
            ldx         n_opc
            lda         cpx_progs,x
            jmp         n_cpx

; hv = -n_x, a number (a complex one's parts each negated)
n_neg:
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            beq         :+
            jmp         n_rneg
:
            lda         n_x
            ldx         n_x + 1
            jsr         n_parts
            sta         n_x
            stx         n_x + 1
            MOVR        n_xi, n_v
            jsr         n_rneg                              ; (Its real part, then its imaginary)
            MOVR        n_y, hv
            MOVR        n_x, n_xi
            jsr         n_rneg
            MOVR        n_x, n_y
            MOVR        n_y, hv
            jmp         n_cpx_make

; .A = n_x against n_y, two numbers: $FF less, 0 the same, 1 more (complex ones by their real parts, then their
; imaginary)
n_cmp:
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            beq         @complex
            lda         n_y
            ldx         n_y + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            beq         @complex
            jmp         n_rcmp
@complex:
            lda         n_x                                 ; (Their real parts, their imaginary kept)
            ldx         n_x + 1
            jsr         n_parts
            sta         n_x
            stx         n_x + 1
            MOVR        n_xi, n_v
            lda         n_y
            ldx         n_y + 1
            jsr         n_parts
            sta         n_y
            stx         n_y + 1
            MOVR        n_yi, n_v
            jsr         n_rcmp
            bne         @rts
            MOVR        n_x, n_xi
            MOVR        n_y, n_yi
            jmp         n_rcmp
@rts:
            rts

; ---- Complex numbers: their parts worked on a stack

; hv = n_x op n_y, either complex, by the program at cpx_code + .A: each byte pushes a part (CP_XR, CP_XI, CP_YR,
; CP_YI: x's and y's) or a value worked out before (CP_AT + k: the kth), or does an operation (OP_*) on the two on
; top, till CP_END, which makes the complex number of the two on top (a real one if its imaginary part is 0).  A
; value an operation makes is the arena's top one, so an operation on two of them makes its own in their room
CP_XR           = $10
CP_XI           = $11
CP_YR           = $12
CP_YI           = $13
CP_AT           = $20
CP_END          = $FF

n_cpx:
            sta         n_pc
            stz         s_n
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
            pha                                             ; (An operation, on the two on top: its room theirs)
            jsr         n_spop2
            MOVR        a_top, n_free
            pla
            jsr         n_real
            ldy         #1
            jsr         n_spush
            bra         @step
@push:
            sec                                             ; (A part, or a value worked out: its place)
            sbc         #CP_XR
            cmp         #CP_AT - CP_XR
            bcc         :+
            sbc         #CP_AT - CP_XR - 4
:
            tax
            lda         s_hi,x
            pha
            lda         s_lo,x
            plx
            ldy         #0
            jsr         n_spush
            bra         @step
@end:
            jsr         n_spop2                             ; (The real and imaginary parts)
            bra         n_cpx_make
@parts:
            jsr         n_parts
            ldy         #0
            jsr         n_spush
            lda         n_v
            ldx         n_v + 1
            ldy         #0
            jmp         n_spush

; hv = the complex number n_x + n_y i (n_x if n_y is 0), written in the arena
n_cpx_make:
            lda         n_y
            ldx         n_y + 1
            jsr         n_zero_p
            bne         @complex
            MOVR        hv, n_x
            lda         hv
            ldx         hv + 1
            rts
@complex:
            lda         n_x                                 ; (Room: its tag, the two)
            ldx         n_x + 1
            jsr         n_size
            sta         n_p1
            stx         n_p1 + 1
            lda         n_y
            ldx         n_y + 1
            jsr         n_size
            sta         n_p2
            stx         n_p2 + 1
            clc
            adc         n_p1
            pha
            txa
            adc         n_p1 + 1
            tax
            pla
            jsr         a_room
            lda         #NT_COMPLEX
            jsr         a_put
            MOVR        nq, n_x
            MOVR        n_t, n_p1
            jsr         a_copy
            MOVR        nq, n_y
            MOVR        n_t, n_p2
            jsr         a_copy
            jmp         a_made

; n_t bytes from nq to the arena (nu), nq and nu moved past them
a_copy:
            lda         n_t
            ora         n_t + 1
            beq         @rts
            lda         (nq)
            jsr         a_put
            inc         nq
            bne         :+
            inc         nq + 1
:
            lda         n_t
            bne         :+
            dec         n_t + 1
:
            dec         n_t
            bra         a_copy
@rts:
            rts

; .A, .X pushed on the machine's stack, .Y = 1 if it's a value it made (the arena's top)
n_spush:
            phx
            ldx         s_n
            cpx         #NSTACK
            bcs         @big
            sta         s_lo,x
            pla
            sta         s_hi,x
            tya
            sta         s_own,x
            inc         s_n
            rts
@big:
            jmp         n_big

; n_x, n_y = the two values on the stack's top, taken off; n_free = the arena's top as it would be without them (the
; lower of the two's if they were the machine's own: those made last)
n_spop2:
            MOVR        n_free, a_top
            dec         s_n
            ldx         s_n
            lda         s_lo,x
            sta         n_y
            lda         s_hi,x
            sta         n_y + 1
            lda         s_own,x
            beq         :+
            MOVR        n_free, n_y
:
            dec         s_n
            ldx         s_n
            lda         s_lo,x
            sta         n_x
            lda         s_hi,x
            sta         n_x + 1
            lda         s_own,x
            beq         :+
            MOVR        n_free, n_x
:
            rts

.rodata
n_zero:     .byte       $00                                 ; (The number 0)

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

.code
