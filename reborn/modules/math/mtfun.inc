; ****************************************************************************
; mtfun.inc - the math library's entries (spec/numbers.def's math: DIGITS, SQRT, EXP, LOG, RPOW, PI; TRIG and ATAN are
; mttrig.inc's, in the second bank) and
; their cores, each a function at mt_b bits for m_ziv (danlang's NumMath.cs: ExpCore, LogCore, TrigCore ...; atan
; here by atan(1/m) and the rest, not by halvings: any right way gives the same digits).  A square root is worked
; exactly in integers (its digits rounded from an integer square root), as danlang's is.

.segment "NUMBANK"
mt_bz:      .res        2                                   ; (exp's argument's bits)
mt_kb:      .res        2                                   ; (A reduction's extra bits)
mt_tb:      .res        2
mt_bsave:   .res        2
mt_kq:      .res        2                                   ; (A root's degree)
mt_re:      .res        2                                   ; (A complex result's real part: a value)
mt_lt:      .res        2                                   ; (log's power of 2)
mt_need2:   .res        1                                   ; (trig's: bit 7 sin r, bits 0-3 cos r)

.code

; ---- DIGITS: .A = a precision (1 to MAX_DIGITS; 0, none set).  OUT: .A = the precision before.  NE_DOMAIN

mt_digits:
            jsr         nm_begin
            cmp         #0
            beq         @get
            cmp         #MAX_DIGITS + 1
            bcc         :+
            lda         #NE_DOMAIN
            jmp         nm_fail
:
            ldx         st_digits
            sta         st_digits
            txa
            bra         @out
@get:
            lda         st_digits
@out:
            ldx         #0
            clc
            jmp         nm_end

; ---- PI: pi at r2

mt_pi:
            jsr         nm_begin
            LDR         mt_core, m_picore
            jsr         m_ziv
            jmp         mt_out

m_picore:
            MOVR        mt_q, mt_b
            jsr         m_pi
            RCOPY       MA, 3
            RSET        ME, 2
            stz         mt_k10
            stz         mt_k10 + 1
            rts

; ---- SQRT: r0 = x, a real number.  Exact when it is (x's kind), else a fixed decimal of the precision's digits; of
; a negative number, imaginary.  NE_REAL

mt_sqrt:
            jsr         nm_begin
            jsr         mt_arg
            ldx         #XN
            lda         r_neg,x
            sta         mt_xneg                             ; (Below 0: its root imaginary)
            stz         r_neg,x
            lda         r_len,x
            bne         :+
            lda         n_x                                 ; (0: itself)
            ldx         n_x + 1
            jmp         nm_out
:
            jsr         m_sqrt
            lda         mt_xneg
            beq         :+
            MOVR        n_y, hv                             ; (0 + its root i)
            LDAX        n_zero
            sta         n_x
            stx         n_x + 1
            jsr         n_cpx_make
:
            jmp         mt_out

; hv = sqrt(XN / XD) (above 0): exact, the root in x's kind (n_kv), if both are perfect squares; else rounded to the
; precision, as integers: R = round(sqrt(P Q) / Q), P / Q = x 10^2p, (isqrt(4 P Q) + Q) / 2Q
m_sqrt:
            lda         #XN
            ldy         #2
            jsr         m_isqrt
            RMUL        3, 2, 2
            RUCMP       3, XN
            cmp         #0
            bne         @inexact
            lda         #XD
            ldy         #4
            jsr         m_isqrt
            RMUL        3, 4, 4
            RUCMP       3, XD
            cmp         #0
            bne         @inexact
            jmp         m_kindout
@inexact:
            lda         st_digits
            sta         mt_n
            jsr         m_log10q                            ; (e = floor(log10 x))
            lda         mt_e + 1                            ; (p = n - 1 - floor(e / 2))
            cmp         #$80
            ror
            sta         mt_p + 1
            lda         mt_e
            ror
            sta         mt_p
            sec
            lda         mt_n
            sbc         #1
            sta         mt_t
            stz         mt_t + 1
            SUBVV       mt_p, mt_t, mt_p
@try:
            RCOPY       2, XN                               ; (P / Q = x 10^2p)
            RCOPY       3, XD
            lda         mt_p                                ; (2p)
            asl
            sta         mt_t
            lda         mt_p + 1
            rol
            sta         mt_t + 1
            bmi         @neg
            lda         #6
            sta         nrc
            lda         mt_t
            ldx         mt_t + 1
            jsr         r_pow10
            RMUL        0, 2, 6
            RCOPY       2, 0
            bra         @root
@neg:
            sec
            lda         #0
            sbc         mt_t
            pha
            lda         #0
            sbc         mt_t + 1
            tax
            lda         #6
            sta         nrc
            pla
            jsr         r_pow10
            RMUL        0, 3, 6
            RCOPY       3, 0
@root:
            RMUL        30, 2, 3                            ; (isqrt(4 P Q): R5)
            lda         #30
            sta         nrc
            lda         #4
            jsr         r_mulsmall
            lda         #30
            ldy         #5
            jsr         m_isqrt
            RADD        5, 5, 3                             ; (+ Q, over 2Q: R31)
            RCOPY       0, 3
            lda         #0
            sta         nrc
            lda         #2
            jsr         r_mulsmall
            RDIV        31, 1, 5, 0
            lda         #6                                  ; (n digits: else the places moved)
            sta         nrc
            lda         mt_n
            ldx         #0
            jsr         r_pow10
            RUCMP       31, 6
            cmp         #$FF
            beq         :+
            lda         mt_p
            bne         @less
            dec         mt_p + 1
@less:
            dec         mt_p
            jmp         @try
:
            lda         #6
            sta         nrc
            lda         mt_n
            dec         a
            ldx         #0
            jsr         r_pow10
            RUCMP       31, 6
            cmp         #$FF
            bne         :+
            ADDV        mt_p, 1
            jmp         @try
:
            RCOPY       0, 31
            jmp         m_fixout

; mt_e = floor(log10(XN / XD)) (above 0): from their bits, then by comparisons with 10^e.  Uses R0, R6
m_log10q:
            lda         #XN
            jsr         m_bitlen
            sta         mt_u
            stx         mt_u + 1
            lda         #XD
            jsr         m_bitlen
            sta         mt_v
            stx         mt_v + 1
            SUBVV       mt_u, mt_u, mt_v
            lda         mt_u
            ldx         mt_u + 1
            jsr         m_log10x
@check:
            jsr         m_cmp10                             ; (Below 10^e: e less)
            cmp         #$FF
            bne         :+
            lda         mt_e
            bne         @dec
            dec         mt_e + 1
@dec:
            dec         mt_e
            bra         @check
:
            ADDV        mt_e, 1                             ; (10^(e + 1) or more: e more)
            jsr         m_cmp10
            pha
            lda         mt_e
            bne         :+
            dec         mt_e + 1
:
            dec         mt_e
            pla
            cmp         #$FF
            beq         :+
            ADDV        mt_e, 1
            bra         @check
:
            rts

; .A = XN / XD against 10^mt_e: $FF less, 0 the same, 1 more.  Uses R0, R6
m_cmp10:
            lda         mt_e + 1
            bmi         @neg
            lda         #6
            sta         nrc
            lda         mt_e
            ldx         mt_e + 1
            jsr         r_pow10
            RMUL        0, XD, 6
            RUCMP       XN, 0
            rts
@neg:
            sec
            lda         #0
            sbc         mt_e
            pha
            lda         #0
            sbc         mt_e + 1
            tax
            lda         #6
            sta         nrc
            pla
            jsr         r_pow10
            RMUL        0, XN, 6
            RUCMP       0, XD
            rts

; hv = R2 / R4 (above 0, in lowest terms), as x's kind (n_kv): a fixed decimal's a fixed decimal (R4 a power of 2
; and 5's: the fewest places that hold it), else an integer or a rational
m_kindout:
            lda         n_kv
            cmp         #NK_FIXED
            beq         @fix
            RCOPY       5, 4
            RCOPY       4, 2
            jmp         n_ratw
@fix:
            stz         mt_p
            stz         mt_p + 1
@places:
            lda         #0                                  ; (10^places / R4, exactly?)
            sta         nrc
            lda         mt_p
            ldx         mt_p + 1
            jsr         r_pow10
            RDIV        1, 6, 0, 4
            ldx         #6
            lda         r_len,x
            beq         :+
            ADDV        mt_p, 1
            bra         @places
:
            RMUL        0, 2, 1
            jmp         m_fixout

; R(.Y) = floor(sqrt(R(.A))) (.A, .Y not R0, R1, R6 or R7): Newton's, from a power of 2 above it.  Uses R0, R1, R6, R7
m_isqrt:
            sta         mt_sx
            sty         mt_sy
            jsr         m_bitlen
            sta         mt_q
            stx         mt_q + 1
            ora         mt_q + 1
            bne         :+
            ldx         mt_sy
            jmp         r_clear
:
            ADDV        mt_q, 1                             ; (2^ceil(bits / 2))
            lsr         mt_q + 1
            ror         mt_q
            ldy         #0
            jsr         m_pow2
@step:
            lda         mt_sx                               ; (y = (x + n / x) / 2)
            sta         nra
            lda         #0
            sta         nrb
            lda         #6
            sta         nrq
            lda         #7
            sta         nrr
            jsr         r_divmod
            RADD        1, 0, 6
            RSHRN       1, 1
            RUCMP       1, 0                                ; (No less than x: x it is)
            cmp         #$FF
            bne         @done
            RCOPY       0, 1
            bra         @step
@done:
            lda         #0
            sta         nra
            lda         mt_sy
            sta         nrc
            jmp         r_copy

; ---- EXP: r0 = x, a real number (within EXP_MOST of 0).  NE_REAL, NE_DOMAIN

mt_exp:
            jsr         nm_begin
            jsr         mt_arg
            ldx         #XN
            lda         r_len,x
            bne         :+
            lda         #1                                  ; (exp 0: 1)
            jmp         mt_small
:
            LDR         mt_core, m_expx
            jsr         m_ziv
            jmp         mt_out

; exp's core: x at b + 40 bits (R4), then m_expcore
m_expx:
            MOVR        mt_bz, mt_b
            ADDV        mt_bz, 32
            MOVR        mt_q, mt_bz
            lda         #XN
            ldx         #XD
            ldy         #4
            jsr         m_fixed
; exp z, z = R4 / 2^mt_bz (bz at least w: b + 24), at mt_b bits: 10^k exp r, r = z - k log 10 (k its floor, near enough);
; exp r = exp(r / 256)^256, the series.  ME 3 (the sum's error, 3 ulps a term and 6, doubled at each squaring and
; under 10 times the value's: 10 (3n + 6) 2^9, under 2^22 of w's ulps for n to 200, w b + 24; z's error, under
; 2^(b + bits(y) + 10 - bz) of b's ulps)
m_expcore:
            MOVR        mt_w, mt_b
            ADDV        mt_w, 24
            RSET        0, EXP_MOST                         ; (|z| past EXP_MOST: NE_DOMAIN)
            RSHL        0, mt_bz
            RUCMP       4, 0
            cmp         #1
            bne         :+
            lda         #NE_DOMAIN
            jmp         nm_fail
:
            LDR         mt_q, 32                            ; (k: z at 32 bits over log 10 at 32, its floor)
            jsr         m_ln10
            RCOPY       5, 3
            RCOPY       0, 4
            SUBVV       mt_q, mt_bz, mt_32
            RSHR        0, mt_q
            RDIV        1, 2, 0, 5
            ldx         #4
            lda         r_neg,x
            beq         @k
            ldx         #2
            lda         r_len,x
            beq         @k
            RSET        2, 1
            RSUB        1, 1, 2
@k:
            lda         #1
            sta         nrc
            jsr         r_word
            sta         mt_k10
            stx         mt_k10 + 1
            lda         #1                                  ; (kb = bits(k) + 4)
            jsr         m_bitlen
            clc
            adc         #4
            sta         mt_kb
            txa
            adc         #0
            sta         mt_kb + 1
            ADDVV       mt_q, mt_bz, mt_kb                  ; (r = (z 2^kb - k log 10) / 2^(kb + bz - w): R5)
            jsr         m_ln10
            RSETV       0, mt_k10
            RMUL        1, 3, 0
            RCOPY       2, 4
            RSHL        2, mt_kb
            RSUB        2, 2, 1
            ADDVV       mt_q, mt_kb, mt_bz
            SUBVV       mt_q, mt_q, mt_w
            RSHR        2, mt_q
            RCOPY       5, 2
            RCOPY       30, 5                               ; (t = r / 256)
            RSHRN       30, 8
            MOVR        mt_q, mt_w                          ; (The sum R31, a term R4: 1 at first)
            ldy         #31
            jsr         m_pow2
            RCOPY       4, 31
            LDR         mt_i, 1
@term:
            RMULW       0, 4, 30
            lda         #0
            sta         nrc
            MOVR        mt_d, mt_i
            jsr         m_div16
            RCOPY       4, 0
            RADD        31, 31, 4
            ADDV        mt_i, 1
            ldx         #4
            lda         r_len,x
            bne         @term
            lda         #8                                  ; (Squared 8 times)
            sta         mt_cnt
@square:
            RMULW       0, 31, 31
            RCOPY       31, 0
            dec         mt_cnt
            bne         @square
            RCOPY       MA, 31
            lda         #MA
            jsr         m_tob
            RSET        ME, 3
            rts

; ---- LOG: r0 = x, a real number, not 0; of a negative number, complex (log |x| + pi i).  NE_REAL, NE_DOMAIN

mt_log:
            jsr         nm_begin
            jsr         mt_arg
            ldx         #XN
            lda         r_len,x
            bne         :+
            lda         #NE_DOMAIN
            jmp         nm_fail
:
            lda         r_neg,x
            sta         mt_xneg
            stz         r_neg,x
            RUCMP       XN, XD                              ; (|x| 1: its logarithm 0)
            cmp         #0
            bne         @ziv
            lda         mt_xneg
            bne         :+
            lda         #0
            jmp         mt_small
:
            LDR         hv, n_zero                          ; (log -1: pi i)
            bra         @neg
@ziv:
            LDR         mt_core, m_logcore
            jsr         m_ziv
            lda         mt_xneg
            bne         @neg
            jmp         mt_out
@neg:
            MOVR        mt_re, hv                           ; (log |x| + pi i)
            LDR         mt_core, m_picore
            jsr         m_ziv
            MOVR        n_y, hv
            MOVR        n_x, mt_re
            jsr         n_cpx_make
            jmp         mt_out

; log x (XN / XD, above 0, not 1) at mt_b bits: t log 2 + 2 atanh((f - 1) / (f + 1)), x = 2^t f, f from 3/4 to 3/2.
; ME 3 (4 ulps a term and 8, under 2^12 of w's ulps for its terms to 1,000: w is b + 14)
m_logcore:
            MOVR        mt_w, mt_b
            ADDV        mt_w, 14
            lda         #XN                                 ; (t = bits(XN) - bits(XD))
            jsr         m_bitlen
            sta         mt_lt
            stx         mt_lt + 1
            lda         #XD
            jsr         m_bitlen
            sta         mt_u
            stx         mt_u + 1
            SUBVV       mt_lt, mt_lt, mt_u
@scale:
            RCOPY       0, XN                               ; (f = x / 2^t: R0 / R1)
            RCOPY       1, XD
            lda         mt_lt + 1
            bmi         @up
            RSHL        1, mt_lt
            bra         @cmp
@up:
            sec
            lda         #0
            sbc         mt_lt
            sta         mt_u
            lda         #0
            sbc         mt_lt + 1
            sta         mt_u + 1
            RSHL        0, mt_u
@cmp:
            RCOPY       2, 0                                ; (Below 3/4: t less)
            lda         #2
            sta         nrc
            lda         #4
            jsr         r_mulsmall
            RCOPY       3, 1
            lda         #3
            sta         nrc
            lda         #3
            jsr         r_mulsmall
            RUCMP       2, 3
            cmp         #$FF
            bne         :+
            lda         mt_lt
            bne         @dec
            dec         mt_lt + 1
@dec:
            dec         mt_lt
            jmp         @scale
:
            RCOPY       2, 0                                ; (3/2 or more: t more)
            lda         #2
            sta         nrc
            lda         #2
            jsr         r_mulsmall
            RUCMP       2, 3
            cmp         #$FF
            beq         :+
            ADDV        mt_lt, 1
            jmp         @scale
:
            RUCMP       0, 1                                ; (z = (f - 1) / (f + 1): its sign apart)
            sta         mt_zn
            RSUB        2, 0, 1
            RABS        2
            RADD        3, 0, 1
            MOVR        mt_q, mt_w                          ; (z at w bits: R4; z^2 R5)
            lda         #2
            ldx         #3
            ldy         #4
            jsr         m_fixed
            RMULW       5, 4, 4
            RCOPY       30, 4                               ; (Each term z^(2k + 1) / (2k + 1): R31 their sum)
            RCLR        31
            LDR         mt_i, 0
@term:
            ldx         #30
            lda         r_len,x
            beq         @sum
            RCOPY       0, 30
            lda         mt_i
            asl
            sta         mt_d
            lda         mt_i + 1
            rol
            sta         mt_d + 1
            inc         mt_d
            lda         #0
            sta         nrc
            jsr         m_div16
            RADD        31, 31, 0
            RMULW       1, 30, 5
            RCOPY       30, 1
            ADDV        mt_i, 1
            bra         @term
@sum:
            RSHLN       31, 1                               ; (Twice it; below 1, below 0)
            lda         mt_zn
            cmp         #$FF
            bne         :+
            RNEG        31
:
            RSETV       0, mt_lt                             ; (+ t log 2: log 2 at w + bits(t) + 2)
            lda         #0
            jsr         m_bitlen
            clc
            adc         #2
            sta         mt_tb
            txa
            adc         #0
            sta         mt_tb + 1
            ADDVV       mt_q, mt_w, mt_tb
            jsr         m_ln2
            RSETV       0, mt_lt
            RMUL        1, 3, 0
            RSHR        1, mt_tb
            RADD        31, 31, 1
            RCOPY       MA, 31
            lda         #MA
            jsr         m_tob
            RSET        ME, 3
            stz         mt_k10
            stz         mt_k10 + 1
            rts

; ---- RPOW: r0 = x, r1 = y: x to the power y.  A whole y's power exact (numbers' POW, any x); else x real and 0 or
; more: exact when its root is rational (x's kind), else exp (y log x) to the precision.  NE_REAL, NE_DOMAIN (x below
; 0), NE_DIV0 (0 to a power below 0)

mt_pow:
            jsr         nm_begin
            jsr         nm_in2
            lda         n_y
            ldx         n_y + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            beq         @real
            lda         n_y                                 ; (y: YN / YD, in lowest terms)
            ldx         n_y + 1
            ldy         #YN
            jsr         n_load
            jsr         m_lowest
            lda         #YD
            sta         nrc
            jsr         r_one_p
            bne         :+
            jmp         n_powxy                             ; (Whole: numbers' POW's way)
:
            lda         n_x
            ldx         n_x + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            bne         :+
@real:
            lda         #NE_REAL
            jmp         nm_fail
:
            lda         n_x
            ldx         n_x + 1
            ldy         #XN
            jsr         n_load
            ldx         #XN
            lda         r_neg,x
            beq         :+
            lda         #NE_DOMAIN
            jmp         nm_fail
:
            lda         r_len,x
            bne         @pos
            ldx         #YN                                 ; (0: to a power above 0, 0)
            lda         r_neg,x
            beq         :+
            lda         #NE_DIV0
            jmp         nm_fail
:
            lda         #0
            jmp         mt_small
@pos:
            jsr         m_powexact
            bcs         :+
            jmp         n_powxy
:
            LDR         mt_core, m_powcore
            jsr         m_ziv
            jmp         mt_out

; YN / YD in lowest terms (a fixed decimal's fraction as n_load gives it isn't): each divided by their greatest
; common divisor (n_gcd's: R0 its odd part, n_gk its twos), as n_rat does.  Uses R0-R6
m_lowest:
            RCOPY       4, YN
            RCOPY       5, YD
            jsr         n_gcd
            beq         @done
            lda         #0
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
            lda         n_gk
            ora         n_gk + 1
            beq         @copy
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
@copy:
            RCOPY       YN, 4
            RCOPY       YD, 5
@done:
            rts

; A rational root: n_x = x's YDth root (x's kind) and n_y = YN (an integer), C = 0, if XN and XD are perfect YDth
; powers; else C = 1
m_powexact:
            ldx         #YD                                 ; (Its degree: past 16 bits, more than any number's
            lda         r_len,x                             ;   bits: only 1 is a perfect power)
            cmp         #3
            bcc         :+
            LDR         mt_kq, $FFFF
            bra         @roots
:
            lda         #YD
            sta         nrc
            jsr         r_word
            sta         mt_kq
            stx         mt_kq + 1
            cpx         #$80                                ; (32768 and more: r_word's 32767)
            bne         @roots
            LDR         mt_kq, $FFFF
@roots:
            lda         #XN
            ldy         #MA
            jsr         m_iroot
            bcs         @no
            lda         #XD
            ldy         #ME
            jsr         m_iroot
            bcs         @no
            RCOPY       2, MA
            RCOPY       4, ME
            jsr         m_kindout
            MOVR        n_x, hv
            lda         #YN
            sta         nrc
            jsr         r_store
            sta         n_y
            stx         n_y + 1
            clc
            rts
@no:
            sec
            rts

; R(.Y) = floor(R(.A)'s mt_kq-th root) (.A above 0, .Y not one m_iroot uses): C = 0 if it's exact (its power is
; R(.A)).  Newton's, from a power of 2 above it.  Uses R0-R3, R6, R7, R30, R31
m_iroot:
            sta         mt_sx
            sty         mt_sy
            lda         mt_kq                               ; (Its first root: itself)
            cmp         #1
            bne         :+
            lda         mt_kq + 1
            bne         :+
            lda         mt_sx
            sta         nra
            lda         mt_sy
            sta         nrc
            jsr         r_copy
            clc
            rts
:
            lda         mt_sx                               ; (Its degree no less than its bits: 1, exact if it's 1)
            jsr         m_bitlen
            sta         mt_t
            stx         mt_t + 1
            cpx         mt_kq + 1
            bcc         @one
            bne         @newton
            cmp         mt_kq
            bcs         @newton
@one:
            lda         mt_sy
            sta         nrc
            lda         #1
            ldx         #0
            jsr         r_set16
            lda         mt_sx
            sta         nrc
            jsr         r_one_p
            beq         :+
            sec
            rts
:
            clc
            rts
@newton:
            stz         mt_q                                ; (2^ceil(bits / k): R30)
            stz         mt_q + 1
            SUBVV       mt_t, mt_t, mt_one16
:
            ADDV        mt_q, 1
            SUBVV       mt_t, mt_t, mt_kq
            bcs         :-
            ldy         #30
            jsr         m_pow2
            SUBVV       mt_u, mt_kq, mt_one16               ; (k - 1)
@step:
            MOVR        mt_e, mt_u                          ; (y = ((k - 1) x + n / x^(k - 1)) / k)
            lda         #30
            jsr         m_ipow
            lda         mt_sx
            sta         nra
            lda         #31
            sta         nrb
            lda         #0
            sta         nrq
            lda         #1
            sta         nrr
            jsr         r_divmod
            RSETV       1, mt_u
            RMUL        2, 30, 1
            RADD        2, 2, 0
            RSETV       1, mt_kq
            ldx         #1                                  ; (k past 32767: r_set16's sign; its magnitude)
            stz         r_neg,x
            RDIV        3, 0, 2, 1
            RUCMP       3, 30
            cmp         #$FF
            bne         @done
            RCOPY       30, 3
            jmp         @step
@done:
            lda         #30
            sta         nra
            lda         mt_sy
            sta         nrc
            jsr         r_copy
            MOVR        mt_e, mt_kq                         ; (Exact: its kth power is it)
            lda         #30
            jsr         m_ipow
            lda         #31
            sta         nra
            lda         mt_sx
            sta         nrb
            jsr         r_ucmp
            cmp         #1
            rts
            ; (C = 1: not the same, or more; but less sets C = 1 too: compare's $FF is 1 or more)

; R31 = R(.A)^mt_e (.A not R6, R7 or R31), by squares.  Uses R6, R7
m_ipow:
            sta         nra
            lda         #7
            sta         nrc
            jsr         r_copy
            RSET        31, 1
@bit:
            lda         mt_e
            ora         mt_e + 1
            beq         @rts
            lsr         mt_e + 1
            ror         mt_e
            bcc         :+
            RMUL        6, 31, 7
            RCOPY       31, 6
:
            lda         mt_e
            ora         mt_e + 1
            beq         @rts
            RMUL        6, 7, 7
            RCOPY       7, 6
            bra         @bit
@rts:
            rts

; x to the power y (XN / XD above 0, YN / YD) at mt_b bits: exp(y log x), log x at b + bits(y) + 32 bits (its error,
; 3 ulps there, times |y|: m_expcore's z's; and at least m_expcore's w, b + 24)
m_powcore:
            lda         #YN                                 ; (bz = b + max(bits(YN) - bits(YD), 0) + 1 + 32)
            jsr         m_bitlen
            sta         mt_t
            stx         mt_t + 1
            lda         #YD
            jsr         m_bitlen
            sta         mt_u
            stx         mt_u + 1
            SUBVV       mt_t, mt_t, mt_u
            lda         mt_t + 1
            bpl         :+
            stz         mt_t
            stz         mt_t + 1
:
            ADDVV       mt_bz, mt_b, mt_t
            ADDV        mt_bz, 33
            MOVR        mt_bsave, mt_b                      ; (log x at bz bits: MA)
            MOVR        mt_b, mt_bz
            jsr         m_logcore
            MOVR        mt_b, mt_bsave
            RMUL        0, MA, YN                           ; (z = y log x: R4)
            RDIV        4, 1, 0, YD
            jmp         m_expcore

