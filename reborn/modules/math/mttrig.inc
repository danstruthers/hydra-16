; ****************************************************************************
; mttrig.inc - the math library's TRIG and ATAN and their cores (in its second bank: math.s), as mtfun.inc's entries

            NM_CODE
; ---- TRIG: r0 = x (radians), .Y = 0 sine, 1 cosine, 2 tangent.  NE_REAL, NE_DOMAIN (another .Y)

mt_trig:
            jsr         nm_begin
            sty         mt_which
            cpy         #3
            bcc         :+
            lda         #NE_DOMAIN
            jmp         nm_fail
:
            jsr         mt_arg
            ldx         #XN
            lda         r_len,x
            bne         :+
            lda         mt_which                            ; (Of 0: cos 1, the others 0)
            cmp         #1
            beq         @one
            lda         #0
            jmp         mt_small
@one:
            lda         #1
            jmp         mt_small
:
            LDR         mt_core, m_trigcore
            jsr         m_ziv
            jmp         mt_out

; sin, cos or tan of x (XN / XD, not 0) at mt_b bits: r = x - k pi/2 (k: x over pi/2, rounded), then the series for
; sin r and cos r, and the quadrant's.  ME 3 for sin and cos (3 ulps a term and 6, under 2^14: w is b + 16); for tan
; s / c's own bound
m_trigcore:
            MOVR        mt_w, mt_b
            ADDV        mt_w, 16
            lda         #XN                                 ; (x's bits past the point: mt_v, 0 at least)
            jsr         m_bitlen
            sta         mt_v
            stx         mt_v + 1
            lda         #XD
            jsr         m_bitlen
            sta         mt_u
            stx         mt_u + 1
            SUBVV       mt_v, mt_v, mt_u
            lda         mt_v + 1
            bpl         :+
            stz         mt_v
            stz         mt_v + 1
:
            ADDVV       mt_q, mt_v, mt_32                   ; (pi at those and 32: R4)
            jsr         m_pi
            RCOPY       4, 3
            ADDVV       mt_q, mt_v, mt_32                   ; (x at them: R5)
            lda         #XN
            ldx         #XD
            ldy         #5
            jsr         m_fixed
            RSHLN       5, 1                                ; (k = floor((2x + pi/2) / pi): R30)
            RCOPY       0, 4
            RSHRN       0, 1
            RADD        5, 5, 0
            RDIV        30, 31, 5, 4
            ldx         #5
            lda         r_neg,x
            beq         @k
            ldx         #31
            lda         r_len,x
            beq         @k
            RSET        0, 1
            RSUB        30, 30, 0
@k:
            ldx         #30                                 ; (Its quadrant: k mod 4)
            stz         mt_quad
            lda         r_len,x
            beq         @kb
            lda         #30
            RPAGE
            sta         nd + 1
            stz         nd
            lda         (nd)
            and         #3
            sta         mt_quad
            ldx         #30
            lda         r_neg,x
            beq         @kb
            sec
            lda         #4
            sbc         mt_quad
            and         #3
            sta         mt_quad
@kb:
            lda         #30                                 ; (kb = bits(k) + 4)
            jsr         m_bitlen
            clc
            adc         #4
            sta         mt_kb
            txa
            adc         #0
            sta         mt_kb + 1
            ADDVV       mt_q, mt_w, mt_kb                   ; (r = (x - k pi / 2) at w + kb, / 2^kb: R5)
            lda         #XN
            ldx         #XD
            ldy         #5
            jsr         m_fixed
            ADDVV       mt_q, mt_w, mt_kb
            jsr         m_pi
            RMUL        0, 3, 30
            RSHRN       0, 1
            RSUB        5, 5, 0
            RSHR        5, mt_kb
            RMULW       4, 5, 5                             ; (r^2: R4)
            RCOPY       30, 5                               ; (sin: R30 the sum, R31 a term; cos: R2, R3)
            RCOPY       31, 5
            MOVR        mt_q, mt_w
            ldy         #2
            jsr         m_pow2
            RCOPY       3, 2
            LDR         mt_i, 1
            LDR         mt_cnt, 0
            lda         mt_which                            ; (Which it needs: sin r, cos r or both: tan's both;
            cmp         #2                                  ;   sin x's sin r in quadrants 0 and 2, else cos r;
            lda         #$FF                                ;   cos x's the other way)
            bcs         @needs
            lda         mt_quad
            eor         mt_which
            and         #1
            beq         :+
            lda         #$0F                                ; (cos r)
            bra         @needs
:
            lda         #$F0                                ; (sin r)
@needs:
            sta         mt_need2
@term:
            lda         mt_i                                ; (2i)
            asl
            sta         mt_t
            lda         mt_i + 1
            rol
            sta         mt_t + 1
            bit         mt_need2
            bpl         @cosr
            RMULW       0, 31, 4                            ; (sin's: -term r^2 / (2i (2i + 1)))
            MOVR        mt_u, mt_t
            ADDVV       mt_v, mt_t, mt_one16
            jsr         m_divpair
            RNEG        0
            RCOPY       31, 0
            RADD        30, 30, 31
@cosr:
            lda         mt_need2
            and         #$0F
            beq         @next
            RMULW       0, 3, 4                             ; (cos's: -term r^2 / ((2i - 1) 2i))
            MOVR        mt_u, mt_t
            SUBVV       mt_v, mt_t, mt_one16
            jsr         m_divpair
            RNEG        0
            RCOPY       3, 0
            RADD        2, 2, 3
@next:
            ADDV        mt_i, 1
            ADDV        mt_cnt, 1
            bit         mt_need2                            ; (Till the terms it needs are 0)
            bpl         :+
            ldx         #31
            lda         r_len,x
            beq         :+
            jmp         @term
:
            lda         mt_need2
            and         #$0F
            beq         @lb205
            ldx         #3
            lda         r_len,x
            beq         @lb205
            jmp         @term
@lb205:
            lda         mt_quad                             ; (The quadrant's: sin, cos = c, -s; -s, -c; -c, s)
            beq         @made
            cmp         #2
            beq         @q2
            bcs         @q3
            RCOPY       0, 30
            RCOPY       30, 2
            RCOPY       2, 0
            RNEG        2
            bra         @made
@q2:
            RNEG        30
            RNEG        2
            bra         @made
@q3:
            RCOPY       0, 30
            RCOPY       30, 2
            RNEG        30
            RCOPY       2, 0
@made:
            stz         mt_k10
            stz         mt_k10 + 1
            lda         mt_cnt                              ; (Their error: 3 ulps a term and 6)
            sta         mt_e
            lda         mt_cnt + 1
            sta         mt_e + 1
            asl         mt_e
            rol         mt_e + 1
            clc
            lda         mt_e
            adc         mt_cnt
            sta         mt_e
            lda         mt_e + 1
            adc         mt_cnt + 1
            sta         mt_e + 1
            ADDV        mt_e, 6
            lda         mt_which
            bne         @lb204
            jmp         @sin
@lb204:
            cmp         #1
            bne         @lb203
            jmp         @cos
@lb203:
            RSETV       31, mt_e                            ; (tan: s / c, if |c| is past its error)
            RUCMP       2, 31
            cmp         #1
            beq         :+
            RCLR        MA
            MOVR        mt_q, mt_b
            ldy         #ME
            jmp         m_pow2
:
            RCOPY       0, 30                               ; (T = s 2^w / c: R4)
            RSHL        0, mt_w
            RDIV        4, 5, 0, 2
            RCOPY       1, 31                               ; (Its error: ((2Es + |T| 2Es / 2^w) 2^w / (|c| - Es)
            RSHLN       1, 1                                ;   + 2) / 2^(w - b) + 2)
            RCOPY       5, 4
            RABS        5
            RMULW       3, 5, 1
            RADD        3, 3, 1
            RSHL        3, mt_w
            RCOPY       0, 2
            RABS        0
            RSUB        0, 0, 31
            RDIV        5, 1, 3, 0
            RSET        0, 2
            RADD        5, 5, 0
            lda         #5
            jsr         m_tob
            RADD        ME, 5, 0
            RCOPY       MA, 4
            lda         #MA
            jmp         m_tob
@sin:
            RCOPY       MA, 30
            bra         :+
@cos:
            RCOPY       MA, 2
:
            lda         #MA
            jsr         m_tob
            RSET        ME, 3
            rts

; |R0| = |R0| / (mt_u * mt_v): at once if the product's under 65536 (both under 256), else one then the other
m_divpair:
            lda         #0
            sta         nrc
            lda         mt_u + 1
            ora         mt_v + 1
            bne         @two
            lda         mt_v
            sta         n_m8
            lda         mt_u
            jsr         mul8
            sta         mt_d
            lda         n_phi
            sta         mt_d + 1
            jmp         m_div16
@two:
            MOVR        mt_d, mt_u
            jsr         m_div16
            lda         #0
            sta         nrc
            MOVR        mt_d, mt_v
            jmp         m_div16

; ---- ATAN: r0 = x, a real number; its arc tangent, in radians.  NE_REAL

mt_atan:
            jsr         nm_begin
            jsr         mt_arg
            ldx         #XN
            lda         r_len,x
            bne         :+
            lda         #0                                  ; (atan 0: 0)
            jmp         mt_small
:
            LDR         mt_core, m_atancore
            jsr         m_ziv
            jmp         mt_out

; atan x (XN / XD, not 0) at mt_b bits: y = |x|, or 1 / |x| past 1 (pi/2 less its atan); m = 1/y rounded: under 16,
; atan y = atan(1/m) + atan((m y - 1) / (m + y)) (pi/4 for m 1); then the series.  ME 3 (2 ulps a term and 10,
; under 2^12 for their terms to 1,000: w is b + 14)
m_atancore:
            MOVR        mt_w, mt_b
            ADDV        mt_w, 14
            ldx         #XN
            lda         r_neg,x
            sta         mt_f                                ; (Below 0: its atan's negative)
            RCOPY       4, XN                               ; (y = R4 / R5)
            RABS        4
            RCOPY       5, XD
            stz         mt_inv
            RUCMP       4, 5
            cmp         #1
            bne         :+
            RCOPY       0, 4                                ; (Past 1: 1 / y, pi/2 less its atan)
            RCOPY       4, 5
            RCOPY       5, 0
            inc         mt_inv
:
            RCOPY       0, 5                                ; (m = floor((2Q + P) / 2P))
            lda         #0
            sta         nrc
            lda         #2
            jsr         r_mulsmall
            RADD        0, 0, 4
            RCOPY       1, 4
            lda         #1
            sta         nrc
            lda         #2
            jsr         r_mulsmall
            RDIV        2, 3, 0, 1
            lda         #2
            sta         nrc
            jsr         r_word
            cpx         #0
            beq         @lb202
            jmp         @plain
@lb202:
            cmp         #16
            bcc         @lb201
            jmp         @plain
@lb201:
            sta         mt_m
            RCOPY       30, 4                               ; (y' = (m P - Q) / (m Q + P): R30 / R31)
            lda         #30
            sta         nrc
            lda         mt_m
            jsr         r_mulsmall
            RSUB        30, 30, 5
            RCOPY       31, 5
            lda         #31
            sta         nrc
            lda         mt_m
            jsr         r_mulsmall
            RADD        31, 31, 4
            lda         mt_m                                ; (atan(1/m): MA)
            cmp         #1
            bne         :+
            MOVR        mt_q, mt_w
            jsr         m_pi
            RSHRN       3, 2
            RCOPY       MA, 3
            bra         @y
:
            lda         mt_w + 1                            ; (w AT_BITS or fewer: atan(1/m) from the table)
            bne         @series
            lda         mt_w
            cmp         #AT_BITS + 1
            bcs         @series
            jsr         m_attab
            bra         @y
@series:
            MOVR        mt_t, mt_w
            lda         mt_m
            jsr         m_atan_inv
            RCOPY       MA, 2
            bra         @y
@plain:
            RCOPY       30, 4
            RCOPY       31, 5
            RCLR        MA
@y:
            ldx         #30                                 ; (|y'| at w bits: R5; its sign mt_yneg)
            lda         r_neg,x
            sta         mt_yneg
            stz         r_neg,x
            MOVR        mt_q, mt_w
            lda         #30
            ldx         #31
            ldy         #5
            jsr         m_fixed
            RMULW       4, 5, 5                             ; (y'^2: R4)
            RCOPY       30, 5                               ; (Each term y'^(2k + 1) / (2k + 1), every other one
            RCLR        31                                  ;   less: R31)
            LDR         mt_i, 0
@term:
            ldx         #30
            lda         r_len,x
            bne         @lb200
            jmp         @sum
@lb200:
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
            lda         mt_i
            lsr
            bcs         :+
            RADD        31, 31, 0
            bra         @next
:
            RSUB        31, 31, 0
@next:
            RMULW       1, 30, 4
            RCOPY       30, 1
            ADDV        mt_i, 1
            jmp         @term
@sum:
            lda         mt_yneg
            beq         :+
            RNEG        31
:
            RADD        31, 31, MA
            lda         mt_inv                              ; (1 / y's: pi/2 less it)
            beq         :+
            MOVR        mt_q, mt_w
            jsr         m_pi
            RSHRN       3, 1
            RSUB        31, 3, 31
:
            lda         mt_f
            beq         :+
            RNEG        31
:
            RCOPY       MA, 31
            lda         #MA
            jsr         m_tob
            RSET        ME, 3
            stz         mt_k10
            stz         mt_k10 + 1
            rts

; MA = atan(1/mt_m) at mt_w bits (AT_BITS or fewer; mt_m 2-15): the table's, shifted down (under an ulp off, as the
; series' would be at most)
m_attab:
            lda         mt_m                                ; (Its entry: at_tab + (m - 2) * AT_BITS / 8, nt)
            sec
            sbc         #2
            tax
            lda         #<at_tab
            sta         nt
            lda         #>at_tab
            sta         nt + 1
:
            dex
            bmi         :+
            lda         nt
            clc
            adc         #AT_BITS / 8
            sta         nt
            bcc         :-
            inc         nt + 1
            bra         :-
:
            lda         #MA
            RPAGE
            sta         nd + 1
            stz         nd
            ldy         #0
:
            lda         (nt),y
            sta         (nd),y
            iny
            cpy         #AT_BITS / 8
            bne         :-
            ldx         #MA
            lda         #AT_BITS / 8
            sta         r_len,x
            stz         r_neg,x
            lda         #MA
            sta         nrc
            jsr         r_c
            jsr         r_normc
            sec                                             ; (Down to w bits)
            lda         #<AT_BITS
            sbc         mt_w
            sta         n_k
            lda         #>AT_BITS
            sbc         mt_w + 1
            sta         n_k + 1
            jmp         r_shr

.include "atantab.inc"
