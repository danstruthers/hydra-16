; ****************************************************************************
; nmconv.inc - integers' entries (IDIV, GCD; POW is nmpow.inc's) and the conversions (TRUNCATE, FLOOR, ROUND, TO_FIXED, TO_RATIONAL,
; NUMERATOR, DENOMINATOR, COMPLEX, PART, FROM_INT, TO_INT), as danlang's (and numref.js's) are; hylang's numbi.inc's
; where it had them.

.segment "NUMBANK"
n_mode:     .res        1                                   ; (TRUNCATE's 0, FLOOR's 1, ROUND's 2; PART's part)
n_places:   .res        2                                   ; (TO_FIXED's places)
n_dig:      .res        1                                   ; (A digit, as TO_FIXED makes it)
n_sign:     .res        1                                   ; (A sign kept)

.code

; ---- Integers

; IDIV: r0 = x, r1 = y, integers; r5 = a place for the remainder, r6 its room.  The quotient (toward 0) at r2, the
; remainder (its sign x's) at r5, r6 its length.  NE_DIV0: y is 0
nm_idiv:
            jsr         nm_begin
            jsr         nm_in2
            lda         n_x
            ldx         n_x + 1
            ldy         #0
            jsr         n_intof
            lda         n_y
            ldx         n_y + 1
            ldy         #1
            jsr         n_intof
            ldx         #1
            lda         r_len,x
            bne         :+
            lda         #NE_DIV0
            jmp         nm_fail
:
            lda         #0
            sta         nra
            lda         #1
            sta         nrb
            lda         #2
            sta         nrq
            lda         #3
            sta         nrr
            jsr         r_divmod
            lda         #3
            sta         nrc
            jsr         r_store
            sta         n_v2
            stx         n_v2 + 1
            lda         #2
            sta         nrc
            jsr         r_store
            jmp         nm_out2

; GCD: r0 = x, r1 = y, integers; their greatest common divisor (0 or more) at r2
nm_gcd:
            jsr         nm_begin
            jsr         nm_in2
            lda         n_x
            ldx         n_x + 1
            ldy         #4
            jsr         n_intof
            lda         n_y
            ldx         n_y + 1
            ldy         #5
            jsr         n_intof
            ldx         #5                                  ; (One 0: the other's magnitude)
            lda         r_len,x
            bne         :+
            ldx         #4
            stz         r_neg,x
            lda         #4
            bra         @store
:
            ldx         #4
            lda         r_len,x
            bne         :+
            ldx         #5
            stz         r_neg,x
            lda         #5
            bra         @store
:
            jsr         n_gcd                               ; (Its odd part, R0, times its twos)
            MOVR        n_k, n_gk
            lda         #0
            sta         nrc
            jsr         r_shl
            lda         #0
@store:
            sta         nrc
            jsr         r_store
            jmp         nm_out

; ---- The conversions

; TRUNCATE, FLOOR, ROUND: r0 = x, a real number; its integer part toward 0, the greatest integer not above it, or the
; nearest integer (a half to the even one), at r2.  NE_REAL: it's complex
nm_truncate:
            ldy         #0
            bra         nm_whole
nm_floor:
            ldy         #1
            bra         nm_whole
nm_round:
            ldy         #2
nm_whole:
            jsr         nm_begin
            sty         n_mode
            jsr         nm_in1
            jsr         n_kind
            beq         @self                               ; (An integer: itself)
            cmp         #NK_COMPLEX
            bne         :+
            lda         #NE_REAL
            jmp         nm_fail
:
            cmp         #NK_FIXED                           ; (A fixed decimal of 615 places or more: below 0.13,
            bne         @frac                               ;   its digits 255 bytes at most)
            lda         n_x
            ldx         n_x + 1
            jsr         n_first
            lda         n_pl + 1
            cmp         #>615
            bcc         @frac
            bne         :+
            lda         n_pl
            cmp         #<615
            bcc         @frac
:
            jsr         n_rsign                             ; (0, or -1 for a negative one's floor)
            ldx         n_mode
            cpx         #1
            beq         :+
            lda         #0
:
            cmp         #1
            bne         :+
            lda         #0
:
            tax
            ldy         #0
            sty         nrc
            jsr         r_set16
@store0:
            lda         #0
            sta         nrc
            jsr         r_store
            jmp         nm_out
@self:
            lda         n_x
            ldx         n_x + 1
            jmp         nm_out
@frac:
            lda         n_x                                 ; (n / d: R0 / R1; its quotient R2, remainder R3)
            ldx         n_x + 1
            ldy         #0
            jsr         n_load
            lda         #0
            sta         nra
            lda         #1
            sta         nrb
            lda         #2
            sta         nrq
            lda         #3
            sta         nrr
            jsr         r_divmod
            lda         n_mode
            beq         @made
            ldx         #3                                  ; (Floor: one less, when there's a remainder below 0)
            lda         r_len,x
            beq         @floored
            lda         r_neg,x
            beq         @floored
            jsr         n_less1
            ldx         #3                                  ; (The remainder, as floor's: d + it)
            jsr         r_negate
            lda         #1
            sta         nra
            lda         #3
            sta         nrb
            sta         nrc
            jsr         r_sub
@floored:
            lda         n_mode
            cmp         #2
            bne         @made
            lda         #3                                  ; (Round: twice the remainder against d)
            sta         nra
            sta         nrb
            lda         #4
            sta         nrc
            jsr         r_add
            lda         #4
            sta         nra
            lda         #1
            sta         nrb
            jsr         r_cmp
            beq         @half
            bmi         @made
            bra         @up
@half:
            lda         #2                                  ; (A half: to the even one)
            jsr         r_page
            sta         na + 1
            stz         na
            ldx         #2
            lda         r_len,x
            beq         @made
            lda         (na)
            lsr
            bcc         @made
@up:
            lda         #4
            sta         nrc
            lda         #1
            ldx         #0
            jsr         r_set16
            lda         #2
            sta         nra
            sta         nrc
            lda         #4
            sta         nrb
            jsr         r_add
@made:
            lda         #2
            sta         nrc
            jsr         r_store
            jmp         nm_out

; R2 = R2 - 1 (R4: 1)
n_less1:
            lda         #4
            sta         nrc
            lda         #1
            ldx         #0
            jsr         r_set16
            lda         #2
            sta         nra
            sta         nrc
            lda         #4
            sta         nrb
            jmp         r_sub

; TO_FIXED: r0 = x, a real number; .A, .X = places (0 to 10000; $FFFF: 10).  x as a fixed decimal of that many places
; at r2, cut short (not rounded).  NE_REAL: it's complex; NE_DOMAIN: places past 10000
nm_to_fixed:
            jsr         nm_begin
            cpx         #$FF
            bne         :+
            cmp         #$FF
            bne         :+
            lda         #10
            ldx         #0
:
            sta         n_places
            stx         n_places + 1
            cpx         #>10001
            bcc         :+
            bne         @domain
            cmp         #<10001
            bcc         :+
@domain:
            lda         #NE_DOMAIN
            jmp         nm_fail
:
            jsr         nm_in1
            jsr         n_kind
            beq         @int
            cmp         #NK_FIXED
            beq         @fix
            cmp         #NK_COMPLEX
            beq         :+
            jmp         @rat
:
            lda         #NE_REAL
            jmp         nm_fail
@int:
            lda         #0                                  ; (An integer: itself, of no places)
            sta         nrc
            lda         n_x
            ldx         n_x + 1
            jsr         r_load
            stz         n_pl
            stz         n_pl + 1
            jsr         n_fix0
            jmp         nm_out
@fix:
            lda         n_x                                 ; (A fixed decimal: as it is, or its digits cut to the
            ldx         n_x + 1                             ;   places)
            jsr         n_first
            lda         n_pl
            cmp         n_places
            lda         n_pl + 1
            sbc         n_places + 1
            bcs         :+
            lda         n_x
            ldx         n_x + 1
            jmp         nm_out
:
            lda         n_x
            ldx         n_x + 1
            jsr         n_first
            ldy         #0
            sty         nrc
            jsr         r_load
@cut:
            lda         n_pl                                ; (A place less, while it has more than wanted)
            cmp         n_places
            bne         :+
            lda         n_pl + 1
            cmp         n_places + 1
            beq         @cutdone
:
            ldx         #0
            lda         r_len,x
            beq         @zero
            lda         #10
            jsr         r_divsmall
            lda         n_pl
            bne         :+
            dec         n_pl + 1
:
            dec         n_pl
            bra         @cut
@zero:
            MOVR        n_pl, n_places
@cutdone:
            jsr         n_fix0
            jmp         nm_out
@rat:
            lda         n_x                                 ; (A rational: hylang's way, a digit at a time.  |n|/d:
            ldx         n_x + 1                             ;   R0/R1, its sign kept)
            ldy         #0
            jsr         n_load
            ldx         #0
            lda         r_neg,x
            sta         n_sign
            stz         r_neg,x
            lda         #0                                  ; (Its whole part: R2; the rest, R3)
            sta         nra
            lda         #1
            sta         nrb
            lda         #2
            sta         nrq
            lda         #3
            sta         nrr
            jsr         r_divmod
            stz         n_pl
            stz         n_pl + 1
@place:
            ldx         #3                                  ; (While there's more, and places: a digit more)
            lda         r_len,x
            beq         @made
            lda         n_pl
            cmp         n_places
            lda         n_pl + 1
            sbc         n_places + 1
            bcs         @made
            inc         n_pl
            bne         :+
            inc         n_pl + 1
:
            lda         #3
            sta         nrc
            lda         #10
            jsr         r_mulsmall
            lda         #2
            sta         nrc
            lda         #10
            jsr         r_mulsmall
            stz         n_dig
@less:
            lda         #3                                  ; (The digit: R1 taken from R3 while it can be)
            sta         nra
            lda         #1
            sta         nrb
            jsr         r_ucmp
            cmp         #$FF
            beq         @digit
            lda         #3
            sta         nra
            sta         nrc
            lda         #1
            sta         nrb
            jsr         r_sub
            inc         n_dig
            bra         @less
@digit:
            lda         #4
            sta         nrc
            lda         n_dig
            ldx         #0
            jsr         r_set16
            lda         #2
            sta         nra
            sta         nrc
            lda         #4
            sta         nrb
            jsr         r_add
            bra         @place
@made:
            lda         #2                                  ; (Its digits, R0, with its sign)
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            ldx         #0
            lda         n_sign
            beq         :+
            jsr         r_negate
:
            jsr         n_fix0
            jmp         nm_out

; TO_RATIONAL: r0 = x, a real number; it as a rational at r2 (an integer when it's whole).  NE_REAL: it's complex
nm_to_rational:
            jsr         nm_begin
            jsr         nm_in1
            jsr         n_real1
            cmp         #NK_FIXED
            beq         :+
            lda         n_x                                 ; (An integer or a rational: itself)
            ldx         n_x + 1
            jmp         nm_out
:
            lda         n_x
            ldx         n_x + 1
            ldy         #4
            jsr         n_load
            jsr         n_rat
            jmp         nm_out

; .A = n_x's kind, a real number's.  NE_REAL: it's complex
n_real1:
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            bne         :+
            lda         #NE_REAL
            jmp         nm_fail
:
            rts

; NUMERATOR, DENOMINATOR: r0 = x, a real number; its numerator, or denominator, in lowest terms, at r2.  NE_REAL: it's
; complex
nm_numerator:
            ldy         #0
            bra         nm_ratpart
nm_denominator:
            ldy         #1
nm_ratpart:
            jsr         nm_begin
            sty         n_mode
            jsr         nm_in1
            jsr         n_real1
            cmp         #NK_FIXED                           ; (A fixed decimal: as a rational first)
            bne         @part
            lda         n_x
            ldx         n_x + 1
            ldy         #4
            jsr         n_load
            jsr         n_rat
            sta         n_x
            stx         n_x + 1
@part:
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            cmp         #NK_RATIONAL
            beq         @rat
            lda         n_mode                              ; (An integer: itself, over 1)
            bne         :+
            lda         n_x
            ldx         n_x + 1
            jmp         nm_out
:
            LDAX        n_one
            jmp         nm_out
@rat:
            lda         n_x
            ldx         n_x + 1
            jsr         n_first
            ldy         n_mode
            beq         :+
            lda         nt
            ldx         nt + 1
:
            jmp         nm_out

; COMPLEX: r0 = re, r1 = im, real numbers; re + im i at r2 (re when im is 0).  NE_REAL: either is complex
nm_complex:
            jsr         nm_begin
            jsr         nm_in2
            jsr         n_real1
            lda         n_y
            ldx         n_y + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            bne         :+
            lda         #NE_REAL
            jmp         nm_fail
:
            jsr         n_cpx_make
            jmp         nm_out

; PART: r0 = x; .Y = 0 its real part, 1 its imaginary part, at r2
nm_part:
            jsr         nm_begin
            sty         n_mode
            jsr         nm_in1
            jsr         n_parts
            ldy         n_mode
            beq         :+
            lda         n_v
            ldx         n_v + 1
:
            jmp         nm_out

; FROM_INT: r0/r1 = a 32-bit integer (r0 its low word); .Y = 1 if it's signed.  It as a number at r2
nm_from_int:
            jsr         nm_begin
            sty         n_mode
            lda         #0
            sta         nrc
            jsr         r_c
            ldx         #0
            stz         r_neg,x
            ldy         #3
:
            lda         r0,y
            sta         (nc),y
            dey
            bpl         :-
            lda         #4
            sta         r_len,x
            ldy         #3
            lda         (nc),y
            bpl         @norm
            lda         n_mode
            beq         @norm
            ldy         #0                                  ; (Signed, and its top bit 1: its two's complement)
            ldx         #4
            sec
:
            lda         (nc),y
            eor         #$FF
            adc         #0
            sta         (nc),y
            iny
            dex
            bne         :-
            ldx         #0
            dec         r_neg,x
@norm:
            jsr         r_normc
            jsr         r_store
            jmp         nm_out

; TO_INT: r0 = x, an integer.  OUT: r4/r5 = its low 32 bits (r4 low, two's complement); .A = 0 if it fits 32 bits
; signed, 1 unsigned only, 2 neither.  NE_INT: it isn't one
nm_to_int:
            jsr         nm_begin
            jsr         nm_in1
            ldy         #0
            jsr         n_intof
            jsr         r_c
            ldx         #0
            lda         #4                                  ; (Its magnitude to 4 bytes at least)
            sta         n_m
            jsr         r_zext
            ldy         #3
:
            lda         (nc),y
            sta         r4,y
            dey
            bpl         :-
            ldx         #0
            lda         r_neg,x
            beq         :+
            ldy         #0                                  ; (Below 0: its two's complement)
            ldx         #4
            sec
@tc:
            lda         r4,y
            eor         #$FF
            adc         #0
            sta         r4,y
            iny
            dex
            bne         @tc
:
            ldx         #0                                  ; (Whether it fits)
            lda         r_len,x
            cmp         #5
            bcs         @neither
            cmp         #4
            bcc         @signed
            ldy         #3
            lda         (nc),y
            bpl         @signed
            lda         r_neg,x
            beq         @unsigned
            lda         (nc),y                              ; (Below 0: -2^31 only)
            cmp         #$80
            bne         @neither
            ldy         #2
:
            lda         (nc),y
            bne         @neither
            dey
            bpl         :-
@signed:
            lda         #0
            bra         @fits
@unsigned:
            lda         #1
            bra         @fits
@neither:
            lda         #2
@fits:
            ldx         #0
            clc
            jmp         nm_end

