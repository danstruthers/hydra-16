; ****************************************************************************
; mtcplx.inc - the math functions of a complex argument, in the third bank (sim/tools/numref.js's csqrt, cexp, clog,
; ctrig, catan, cpow and cabs): their principal values, as mathematics defines them (the argument of a number above -pi
; and to pi; atan odd), each part correctly rounded as a real result is.  SQRT, EXP, LOG, TRIG and ATAN come here when
; their argument is complex (mt_arg: c_go, .Y the function), RPOW when x or y is complex or x is below 0, and ABS
; (c_abs) always.
;   What's exact is worked exactly, with the tower (c_tw): |x|^2, a square root that's rational (sqrt(3+4i) is 2+i,
; |3-4i| 5), a power of whole quarter turns ((-1)^(1/2) is i, i^i real).  A part known to be 0 isn't worked (Ziv's
; way can't decide a 0): log|x| of a modulus of 1, sin and cos of a whole number of quarter turns.  The rest is
; worked by m_ziv a part at a time, its core (c_*core) a little machine's program: approximations on a stack (c_push),
; each an integer A and a bound E on its error at B bits, times 10^K (exp's), within E / 2^B of the true value: exact
; fractions (c_q), pi (c_pi), the cores of exp, log, sin, cos and atan called in the other banks (c_expq ..., NMFAR:
; c_expa and c_triga of an approximation, its error carried on), and the arithmetic (c_add, c_mul, c_div, c_sqrt ...),
; each bounding its result's error.  The stack is in the arena, past the exact values (c_mark).

.macpack     longbranch

V_A             = 0             ; The values (c_v): x's real part, its imaginary part
V_Y             = 1
V_M             = 2             ; |x|^2
V_T             = 3             ; (Scratch)
V_T2            = 4
V_R             = 5             ; |x|, when it's rational
V_H             = 6             ; 1/2 (0.5 if a part is a fixed decimal)
V_YA            = 7             ; c_arg's y / a
V_ZERO          = 8             ; 0, 1, 2
V_ONE           = 9
V_TWO           = 10
V_WR            = 11            ; A power's real part, its imaginary part
V_WI            = 12
V_QV            = 13            ; arg x / pi, when it's whole quarter turns
V_P             = 14            ; Its Tim / pi, likewise
V_HP            = 15            ; trig's: e's powers' arguments, + and -
V_HN            = 16
V_TA            = 17            ; tan's 2a
V_ND            = 18            ; atan's (a^2 + (y + 1)^2) / (a^2 + (y - 1)^2)
V_AA            = 19            ; |a|, a^2
V_YP            = 20            ; sqrt's y (|y| when a is below 0)
V_S             = 21            ; A root, an exact power's
V_N             = 22
NV              = 23

CX              = 24            ; The machine's registers: an approximation X (its A, E), another Y, the result (MA, ME)
CY              = 26

.segment "NUMBANK"
c_v:        .res        2 * NV                              ; The values (exact numbers: their addresses)
c_prm:      .res        4                                   ; (c_tw's)
c_sp:       .res        1                                   ; The stack: its depth, and each approximation's address
c_plo:      .res        8
c_phi:      .res        8
c_lb:       .res        4                                   ; X's and Y's B, as loaded (c_ld); their K
c_lk:       .res        4
c_rb:       .res        2                                   ; The result's B and K
c_rk:       .res        2
c_d16:      .res        2
c_w:        .res        2                                   ; The bits worked at; the core's (m_ziv's) b
c_b:        .res        2
c_wsave:    .res        2
c_bx:       .res        2                                   ; (A power's: the more bits its T's are worked at)
c_mark:     .res        2                                   ; The arena's top, the values' and the parts' made kept
c_core:     .res        2
c_re:       .res        2                                   ; The real part, made
c_fn:       .res        1                                   ; The function (mt_cfn's: 0 SQRT ... 5 RPOW); trig's .Y
c_wh:       .res        1
c_k:        .res        1                                   ; The part worked: 0 real, 1 imaginary
c_zero:     .res        1                                   ; The parts known to be 0 (bit 0 the real, 1 the imaginary)
c_n:        .res        1
c_n2:       .res        1
c_cnt:      .res        1
c_as:       .res        1                                   ; x's parts' signs ($FF, 0, 1)
c_ys:       .res        1
c_ga:       .res        1                                   ; c_arg's: its a's and y's signs
c_gy:       .res        1
c_half1:    .res        1                                   ; (atan's: its real part not halved)
c_wrs:      .res        1                                   ; A power's: y's parts' signs; |x| 1 (1); arg x q pi (1:
c_wis:      .res        1                                   ;   V_QV); 0 (1); Tim p pi (1: V_P); cos and sin of it
c_lre0:     .res        1                                   ;   0, 1 or -1 (0-3: (1, 0), (0, 1), (-1, 0), (0, -1);
c_qf:       .res        1                                   ;   $FF: not); t1-t4, mag0 (the terms that aren't 0)
c_arg0:     .res        1
c_pf:       .res        1
c_cs:       .res        1
c_t1:       .res        1
c_t2:       .res        1
c_t3:       .res        1
c_t4:       .res        1
c_mag0:     .res        1

            NM_CODE

; ---- The calls

; ABS: r0 = x.  |x|: a real number's, or a complex number's modulus (the square root of re^2 + im^2: exact when it's
; rational)
c_abs:
            jsr         nm_begin
            jsr         nm_in1
            jsr         n_kind
            cmp         #NK_COMPLEX
            beq         @cpx
            jsr         n_rsign
            cmp         #$FF
            bne         :+
            jsr         n_rneg
            jmp         mt_out
:
            MOVR        hv, n_x
            jmp         mt_out
@cpx:
            jsr         c_split
            jsr         c_m2
            lda         #V_M
            jsr         c_rt
            jmp         mt_out

; A function of a complex argument (or RPOW's), from its entry (mt_tocpx: the call begun there, its stack as the
; entry had it, n_x its argument and n_y RPOW's y): .Y the function
c_go:
            tsx                                             ; (A failure back to here: the entry's caller)
            stx         v_sp
            sty         c_fn
            lda         mt_which
            sta         c_wh
            jsr         c_split
            lda         c_fn
            asl
            tax
            jmp         (c_fns,x)

            NM_RODATA
c_fns:      .addr       c_sqrt_f, c_exp_f, c_log_f, c_trig_f, c_atan_f, c_pow_f
            NM_CODE

; x's parts as V_A, V_Y, their signs; the constants; nothing known 0
c_split:
            lda         n_x
            ldx         n_x + 1
            jsr         n_parts
            sta         c_v + 2 * V_A
            stx         c_v + 2 * V_A + 1
            MOVR        c_v + 2 * V_Y, n_v
            LDR         c_v + 2 * V_ZERO, n_zero
            LDR         c_v + 2 * V_ONE, k_one
            LDR         c_v + 2 * V_TWO, k_two
            stz         c_zero
            lda         #V_A
            jsr         c_sgn
            sta         c_as
            lda         #V_Y
            jsr         c_sgn
            sta         c_ys
            rts

; ---- SQRT: exact if |x| and its parts' roots are rational, else (r = |x|): sqrt((r + |a|) / 2), and y over twice it
c_sqrt_f:
            jsr         c_sqx
            bcs         :+
            jmp         mt_out
:
            lda         c_as                                ; (|a|; y, or |y| if a is below 0)
            bmi         :+
            MOVR        c_v + 2 * V_AA, c_v + 2 * V_A
            MOVR        c_v + 2 * V_YP, c_v + 2 * V_Y
            bra         @ziv
:
            jsr         c_tw
            .byte       OP_SUB, V_ZERO, V_A, V_AA
            MOVR        c_v + 2 * V_YP, c_v + 2 * V_Y
            lda         c_ys
            bpl         @ziv
            jsr         c_tw
            .byte       OP_SUB, V_ZERO, V_Y, V_YP
@ziv:
            LDAX        c_sqcore
            jmp         c_both

c_sqcore:
            jsr         c_start
            lda         #V_M                                ; (R = |x|, P = sqrt((R + |a|) / 2), Q = y' / 2P)
            jsr         c_q
            jsr         c_sqrt
            lda         #V_AA
            jsr         c_q
            jsr         c_add
            lda         #1
            jsr         c_shr
            jsr         c_sqrt
            lda         #V_YP
            jsr         c_q
            lda         #1
            jsr         c_pick
            lda         #V_TWO
            jsr         c_mulq
            jsr         c_div
            lda         c_as                                ; (a 0 or more: P, Q; below 0: Q, P with y's sign)
            bmi         @neg
            lda         c_k
            bne         @top
@swap:
            jsr         c_swap
@top:
            jmp         c_tob
@neg:
            lda         c_k
            beq         @top
            jsr         c_swap
            lda         c_ys
            bpl         @top
            jsr         c_neg
            bra         @top

; hv = x's square root, C = 0, if it's exact (x complex, or a negative real number); else C = 1
c_sqx:
            lda         c_ys
            bne         @cpx
            jsr         c_tw                                ; (A negative real number: i sqrt(-a))
            .byte       OP_SUB, V_ZERO, V_A, V_T
            lda         #V_T
            jsr         c_rt
            lda         mt_inex
            jne         @no
            LDR         n_x, n_zero
            MOVR        n_y, hv
            jsr         n_cpx_make
            clc
            rts
@cpx:
            jsr         c_m2
            lda         #V_M
            jsr         c_rt
            lda         mt_inex
            jne         @no
            lda         #V_R
            jsr         c_sv
            LDR         c_v + 2 * V_H, k_half               ; (1/2, or 0.5 if a part's fixed)
            lda         #V_A
            jsr         c_kind
            cmp         #NK_FIXED
            beq         :+
            lda         #V_Y
            jsr         c_kind
            cmp         #NK_FIXED
            bne         :++
:
            LDR         c_v + 2 * V_H, k_halff
:
            jsr         c_tw                                ; (sqrt((r + a) / 2) ...
            .byte       OP_ADD, V_R, V_A, V_T
            jsr         c_tw
            .byte       OP_MUL, V_T, V_H, V_T
            lda         #V_T
            jsr         c_rt
            lda         mt_inex
            bne         @no
            MOVR        c_re, hv
            jsr         c_tw                                ;   + sqrt((r - a) / 2) i, y's sign)
            .byte       OP_SUB, V_R, V_A, V_T
            jsr         c_tw
            .byte       OP_MUL, V_T, V_H, V_T
            lda         #V_T
            jsr         c_rt
            lda         mt_inex
            bne         @no
            lda         c_ys
            bpl         :+
            MOVR        n_x, hv
            jsr         n_rneg
:
            jsr         c_make
            clc
            rts
@no:
            sec
            rts

; ---- EXP: e^a cos y + i e^a sin y
c_exp_f:
            LDAX        c_expcore
            jmp         c_both

c_expcore:
            jsr         c_start
            lda         c_k                                 ; (cos y, sin y)
            eor         #1
            tax
            lda         #V_Y
            jsr         c_trigq
            lda         c_as
            beq         :+
            lda         #V_A
            jsr         c_expq
            jsr         c_mul
:
            jmp         c_tob

; ---- LOG: log |x| (half log |x|^2: 0 if it's 1) + i arg x
c_log_f:
            jsr         c_m2
            lda         #V_M
            jsr         c_is1
            bne         :+
            lda         #1
            sta         c_zero
:
            jsr         c_argxy
            LDAX        c_logcore
            jmp         c_both

c_logcore:
            jsr         c_start
            lda         c_k
            bne         :+
            jsr         c_lre
            jmp         c_tob
:
            jsr         c_arg
            jmp         c_tob

; ---- TRIG: sin(a + yi) = sin a cosh y + i cos a sinh y; cos = cos a cosh y - i sin a sinh y; tan = (sin 2a + i
; sinh 2y) / (cos 2a + cosh 2y).  cosh and sinh from e^y and e^-y.  The part with sin a in it 0 if a is
c_trig_f:
            lda         c_wh
            cmp         #2
            bcc         :+
            jsr         c_tw                                ; (tan: 2a, 2y)
            .byte       OP_MUL, V_A, V_TWO, V_TA
            jsr         c_tw
            .byte       OP_MUL, V_Y, V_TWO, V_HP
            bra         @neg
:
            MOVR        c_v + 2 * V_HP, c_v + 2 * V_Y
@neg:
            jsr         c_tw
            .byte       OP_SUB, V_ZERO, V_HP, V_HN
            lda         c_as
            bne         :+
            lda         c_wh                                ; (a 0: sin's and tan's real part, cos's imaginary)
            and         #1
            clc
            adc         #1
            sta         c_zero
:
            LDAX        c_trigcore
            jmp         c_both

c_trigcore:
            jsr         c_start
            lda         c_wh
            cmp         #2
            beq         @tan
            ldx         c_k                                 ; (cosh y for the real part, sinh y the imaginary)
            jsr         c_hyp
            lda         c_wh                                ; (times sin a or cos a: which's XOR the part)
            eor         c_k
            tax
            lda         #V_A
            jsr         c_trigqz
            jsr         c_mul
            lda         c_wh                                ; (cos's imaginary part negative)
            and         c_k
            beq         :+
            jsr         c_neg
:
            jmp         c_tob
@tan:
            ldx         #0                                  ; (cosh 2y + cos 2a ...
            jsr         c_hyp
            ldx         #1
            lda         #V_TA
            jsr         c_trigqz
            jsr         c_add
            lda         c_k                                 ;   under sin 2a, or sinh 2y)
            beq         :+
            ldx         #1
            jsr         c_hyp
            bra         @div
:
            ldx         #0
            lda         #V_TA
            jsr         c_trigq
@div:
            jsr         c_swap
            jsr         c_div
            jmp         c_tob

; cosh (.X 0) or sinh (.X 1) of V_HP (V_HN its negative): (e^v + e^-v) / 2, (e^v - e^-v) / 2
c_hyp:
            phx
            lda         #V_HP
            jsr         c_expq
            lda         #V_HN
            jsr         c_expq
            pla
            beq         :+
            jsr         c_neg
:
            jsr         c_add
            lda         #1
            jmp         c_shr

; ---- ATAN: (1/2) arg(1 - a^2 - y^2 + 2a i) + (i/4) log((a^2 + (y + 1)^2) / (a^2 + (y - 1)^2)); on the imaginary
; axis, 0 between -i and i, a quarter turn past them (y's sign: atan is odd); of i and -i, NE_DOMAIN
c_atan_f:
            jsr         c_tw
            .byte       OP_MUL, V_A, V_A, V_AA
            jsr         c_tw
            .byte       OP_ADD, V_Y, V_ONE, V_T
            jsr         c_tw
            .byte       OP_MUL, V_T, V_T, V_T
            jsr         c_tw
            .byte       OP_ADD, V_AA, V_T, V_N
            jsr         c_tw
            .byte       OP_SUB, V_Y, V_ONE, V_T
            jsr         c_tw
            .byte       OP_MUL, V_T, V_T, V_T
            jsr         c_tw
            .byte       OP_ADD, V_AA, V_T, V_T
            lda         #V_T                                ; (0: x is i)
            jsr         c_val
            jsr         n_zero_p
            bne         :+
@dom:
            lda         #NE_DOMAIN
            jmp         nm_fail
:
            jsr         c_tw
            .byte       OP_DIV, V_N, V_T, V_ND
            lda         #V_N                                ; (0: x is -i)
            jsr         c_val
            jsr         n_zero_p
            beq         @dom
            stz         c_half1
            lda         c_as
            bne         @gen
            jsr         c_tw                                ; (a 0: |y| below 1, 0; else y's sign pi/2)
            .byte       OP_MUL, V_Y, V_Y, V_T
            lda         #V_T
            jsr         c_is1
            cmp         #$FF
            bne         :+
            lda         #1
            sta         c_zero
            bra         @go
:
            stz         c_ga
            lda         c_ys
            sta         c_gy
            inc         c_half1
            bra         @go
@gen:
            jsr         c_tw                                ; (arg(1 - a^2 - y^2, 2a))
            .byte       OP_SUB, V_ONE, V_AA, V_T
            jsr         c_tw
            .byte       OP_MUL, V_Y, V_Y, V_T2
            jsr         c_tw
            .byte       OP_SUB, V_T, V_T2, V_T
            jsr         c_tw
            .byte       OP_MUL, V_A, V_TWO, V_T2
            lda         c_as
            sta         c_gy
            lda         #V_T
            jsr         c_sgn
            sta         c_ga
            beq         @go
            jsr         c_tw
            .byte       OP_DIV, V_T2, V_T, V_YA
@go:
            LDAX        c_atcore
            jmp         c_both

c_atcore:
            jsr         c_start
            lda         c_k
            bne         @im
            jsr         c_arg
            lda         c_half1
            bne         :+
            lda         #1
            jsr         c_shr
:
            jmp         c_tob
@im:
            lda         #V_ND
            jsr         c_logq
            lda         #2
            jsr         c_shr
            jmp         c_tob

; ---- RPOW: x to the power y, the principal value exp(y log x) = exp(Tre) (cos Tim + i sin Tim), Tre = wr log|x| -
; wi arg x, Tim = wr arg x + wi log|x| (y = wr + wi i).  0 to a power: 0 if its real part is above 0, else
; NE_DOMAIN.  y = n/2: exact if x's square root is.  Each term known 0 isn't worked (t1-t4); arg x is q pi on an axis
; or a diagonal, and Tim then p pi: its cos and sin exact when 2p is whole
c_pow_f:
            lda         n_y
            ldx         n_y + 1
            jsr         n_parts
            sta         c_v + 2 * V_WR
            stx         c_v + 2 * V_WR + 1
            MOVR        c_v + 2 * V_WI, n_v
            lda         #V_WR
            jsr         c_sgn
            sta         c_wrs
            lda         #V_WI
            jsr         c_sgn
            sta         c_wis
            lda         c_as
            ora         c_ys
            bne         @nz
            lda         c_wrs                               ; (0: to a power with its real part above 0, 0)
            cmp         #1
            beq         :+
            lda         #NE_DOMAIN
            jmp         nm_fail
:
            LDR         hv, n_zero
            jmp         mt_out
@nz:
            lda         c_wis                               ; (y = n/2: x's square root's nth power, if it's exact)
            bne         @gen
            lda         #V_WR                               ; (YN / YD, in lowest terms)
            ldy         #YN
            jsr         c_vload
            NMFAR       m_lowest, -2
            RSET        30, 2
            RUCMP       YD, 30
            cmp         #0
            bne         @gen
            ldx         #YN                                 ; (n: its sign apart; past 15 bits, worked out)
            lda         r_neg,x
            sta         c_n2
            stz         r_neg,x
            lda         #YN
            sta         nrc
            jsr         r_word
            cpx         #$7F
            bcs         @gen
            sta         c_d16
            stx         c_d16 + 1
            jsr         c_sqx
            bcs         @gen
            lda         #V_S
            jsr         c_sv
            jsr         c_ipow
            jmp         mt_out
@gen:
            jsr         c_m2
            stz         c_lre0                              ; (1: |x| is 1)
            lda         #V_M
            jsr         c_is1
            bne         :+
            inc         c_lre0
:
            stz         c_qf                                ; (arg x: q pi?)
            stz         c_arg0
            lda         c_ys
            bne         @ax
            inc         c_qf                                ; (Real: 0, or pi)
            LDR         c_v + 2 * V_QV, n_zero
            lda         c_as
            bmi         :+
            inc         c_arg0
            bra         @flags
:
            LDR         c_v + 2 * V_QV, k_one
            bra         @flags
@ax:
            lda         c_as
            bne         @diag
            inc         c_qf                                ; (Imaginary: pi/2, y's sign)
            LDR         c_v + 2 * V_QV, k_half
            lda         c_ys
            bpl         @flags
            LDR         c_v + 2 * V_QV, k_mhalf
            bra         @flags
@diag:
            jsr         c_tw                                ; (|a| = |y|: pi/4 or 3pi/4, y's sign)
            .byte       OP_MUL, V_A, V_A, V_T
            jsr         c_tw
            .byte       OP_MUL, V_Y, V_Y, V_T2
            jsr         c_tw
            .byte       OP_SUB, V_T, V_T2, V_T
            lda         #V_T
            jsr         c_val
            jsr         n_zero_p
            bne         @none
            inc         c_qf
            lda         c_as                                ; (k_q: 1/4, -1/4, 3/4, -3/4)
            and         #2
            ldx         c_ys
            bpl         :+
            ora         #1
:
            asl
            tax
            lda         k_qs,x
            sta         c_v + 2 * V_QV
            lda         k_qs + 1,x
            sta         c_v + 2 * V_QV + 1
            bra         @flags
@none:
            jsr         c_argxy
@flags:
            lda         c_wrs                               ; (t1 = wr log|x|, t2 = wi arg x, t3 = wr arg x, t4 = wi
            beq         :+                                  ;   log|x|: each not 0?)
            lda         #1
:
            sta         c_t1
            sta         c_t3
            lda         c_wis
            beq         :+
            lda         #1
:
            sta         c_t2
            sta         c_t4
            lda         c_lre0
            beq         :+
            stz         c_t1
            stz         c_t4
:
            lda         c_arg0
            beq         :+
            stz         c_t2
            stz         c_t3
:
            lda         c_t1                                ; (|x^y| 1: neither of Tre's)
            ora         c_t2
            eor         #1
            sta         c_mag0
            stz         c_pf                                ; (Tim = p pi: t4 0, and t3 0 or arg x q pi)
            lda         #$FF
            sta         c_cs
            lda         c_t4
            bne         @bits
            LDR         c_v + 2 * V_P, n_zero
            lda         c_t3
            beq         @pf
            lda         c_qf
            beq         @bits
            jsr         c_tw
            .byte       OP_MUL, V_WR, V_QV, V_P
@pf:
            inc         c_pf
            jsr         c_tw                                ; (2p whole: cos and sin 0 and 1s, (2p) mod 4's)
            .byte       OP_MUL, V_P, V_TWO, V_T
            lda         #V_T
            ldy         #CX
            jsr         c_vload
            RDIV        30, 31, CX, CX + 1
            ldx         #31
            lda         r_len,x
            bne         @bits
            jsr         c_mod4
            sta         c_cs
@bits:
            lda         #V_WR                               ; (T's more bits: 48 and y's parts' bits)
            jsr         c_bits
            sta         c_bx
            stx         c_bx + 1
            lda         #V_WI
            jsr         c_bits
            clc
            adc         c_bx
            sta         c_bx
            txa
            adc         c_bx + 1
            sta         c_bx + 1
            ADDV        c_bx, 48
            lda         c_cs
            bmi         @ziv
            and         #1                                  ; (cos and sin, 0 and 1s: the part that's 0 ...
            clc
            adc         #1
            eor         #3
            sta         c_zero
            lda         c_mag0                              ;   and the other 1 or -1, if |x^y| is too)
            beq         @mag
            lda         c_cs
            cmp         #2
            LDR         n_x, k_one
            bcc         :+
            LDR         n_x, k_mone
:
            LDR         n_y, n_zero
            lda         c_cs
            and         #1
            beq         :+
            MOVR        n_y, n_x
            LDR         n_x, n_zero
:
            jsr         n_cpx_make
            jmp         mt_out
@mag:
            LDAX        c_magcore
            jmp         c_both
@ziv:
            LDAX        c_powcore
            jmp         c_both

c_magcore:
            jsr         c_start
            jsr         c_mag
            lda         c_cs
            cmp         #2
            bcc         :+
            jsr         c_neg
:
            jmp         c_tob

c_powcore:
            jsr         c_start
            jsr         c_bzon                              ; (cos Tim, or sin Tim ...
            jsr         c_tim
            jsr         c_bzoff
            lda         c_k
            eor         #1
            tax
            jsr         c_triga
            lda         c_mag0                              ;   times e^Tre)
            bne         :+
            jsr         c_mag
            jsr         c_mul
:
            jmp         c_tob

; e^Tre, Tre worked at bz bits
c_mag:
            jsr         c_bzon
            jsr         c_tre
            jsr         c_bzoff
            jmp         c_expa

; The bits worked at: bz (w + c_bx), and back
c_bzon:
            MOVR        c_wsave, c_w
            ADDVV       c_w, c_w, c_bx
            rts
c_bzoff:
            MOVR        c_w, c_wsave
            rts

; Tre = wr log|x| - wi arg x (t1, t2: one at least)
c_tre:
            stz         c_cnt
            lda         c_t1
            beq         :+
            jsr         c_lre
            lda         #V_WR
            jsr         c_mulq
            inc         c_cnt
:
            lda         c_t2
            beq         @rts
            jsr         c_lim
            lda         #V_WI
            jsr         c_mulq
            jsr         c_neg
            lda         c_cnt
            beq         @rts
            jmp         c_add
@rts:
            rts

; Tim = wr arg x + wi log|x|, or p pi
c_tim:
            lda         c_pf
            beq         :+
            jsr         c_pi
            lda         #V_P
            jmp         c_mulq
:
            stz         c_cnt
            lda         c_t3
            beq         :+
            jsr         c_lim
            lda         #V_WR
            jsr         c_mulq
            inc         c_cnt
:
            lda         c_t4
            beq         @rts
            jsr         c_lre
            lda         #V_WI
            jsr         c_mulq
            lda         c_cnt
            beq         @rts
            jmp         c_add
@rts:
            rts

; log|x| (half log |x|^2), arg x (q pi, or c_arg's)
c_lre:
            lda         #V_M
            jsr         c_logq
            lda         #1
            jmp         c_shr
c_lim:
            lda         c_qf
            beq         :+
            jsr         c_pi
            lda         #V_QV
            jmp         c_mulq
:
            jmp         c_arg

; hv = V_S ^ c_d16 (c_n2 $FF: its reciprocal), exactly: by squares
c_ipow:
            MOVR        c_v + 2 * V_N, c_v + 2 * V_ONE
@bit:
            lda         c_d16
            ora         c_d16 + 1
            beq         @done
            lsr         c_d16 + 1
            ror         c_d16
            bcc         :+
            jsr         c_tw
            .byte       OP_MUL, V_N, V_S, V_N
:
            lda         c_d16
            ora         c_d16 + 1
            beq         @done
            jsr         c_tw
            .byte       OP_MUL, V_S, V_S, V_S
            bra         @bit
@done:
            lda         c_n2
            beq         :+
            jsr         c_tw
            .byte       OP_DIV, V_ONE, V_N, V_N
:
            lda         #V_N
            jsr         c_val
            sta         hv
            stx         hv + 1
            rts

; .A, .X = the bits a value's whole part has (0 if it's below 1: its numerator's less its denominator's)
c_bits:
            ldy         #CX
            jsr         c_vload
            lda         #CX + 1
            jsr         m_bitlen
            sta         c_d16
            stx         c_d16 + 1
            lda         #CX
            jsr         m_bitlen
            sec
            sbc         c_d16
            pha
            txa
            sbc         c_d16 + 1
            tax
            pla
            bcs         :+
            lda         #0
            tax
:
            rts

; .A = R30's value mod 4 (0-3), from its low byte and its sign
c_mod4:
            ldx         #30
            lda         r_len,x
            beq         @rts
            lda         regs + 30 * 256                     ; (R30: the digits' pages, past R0-R7's)
            and         #3
            ldy         r_neg,x
            beq         @rts
            eor         #$FF                                ; (Below 0: 4 - (|n| mod 4), mod 4)
            clc
            adc         #1
            and         #3
@rts:
            rts

; ---- The values: exact numbers, with the tower

; .A, .X = value .A
c_val:
            asl
            tax
            lda         c_v + 1,x
            pha
            lda         c_v,x
            plx
            rts

; Value .A = hv
c_sv:
            asl
            tax
            lda         hv
            sta         c_v,x
            lda         hv + 1
            sta         c_v + 1,x
            rts

; Value .A loaded: register .Y its numerator, .Y + 1 its denominator; n_kv its kind
c_vload:
            phy
            jsr         c_val
            ply
            jmp         n_load

; Value k = value i op value j: the four bytes after the jsr (op, i, j, k)
c_tw:
            pla
            sta         nt
            pla
            sta         nt + 1
            ldy         #4
:
            lda         (nt),y
            sta         c_prm - 1,y
            dey
            bne         :-
            clc
            lda         nt
            adc         #4
            tax
            lda         nt + 1
            adc         #0
            pha
            phx
            lda         c_prm + 1
            jsr         c_val
            sta         n_x
            stx         n_x + 1
            lda         c_prm + 2
            jsr         c_val
            sta         n_y
            stx         n_y + 1
            lda         c_prm
            jsr         n_op
            lda         c_prm + 3
            jmp         c_sv

; .A = value .A's sign ($FF, 0, 1: a real number's)
c_sgn:
            jsr         c_val
            sta         n_x
            stx         n_x + 1
            jmp         n_rsign

; .A = value .A's kind (Z = 1: an integer)
c_kind:
            jsr         c_val
            jmp         n_kind

; .A = value .A against 1: $FF less, 0 the same (Z = 1), 1 more
c_is1:
            jsr         c_val
            sta         n_x
            stx         n_x + 1
            LDR         n_y, k_one
            jsr         n_rcmp
            cmp         #0
            rts

; Value V_M = |x|^2 = a^2 + y^2
c_m2:
            jsr         c_tw
            .byte       OP_MUL, V_A, V_A, V_T
            jsr         c_tw
            .byte       OP_MUL, V_Y, V_Y, V_M
            jsr         c_tw
            .byte       OP_ADD, V_T, V_M, V_M
            rts

; hv = the square root of value .A (above 0): exact if it's rational (mt_inex 0), else rounded (1): m_sqrt's
c_rt:
            ldy         #XN
            jsr         c_vload
            NMFAR       m_sqrt, -2
            rts

; c_arg's a and y: x's, y / a if a isn't 0
c_argxy:
            lda         c_as
            sta         c_ga
            lda         c_ys
            sta         c_gy
            lda         c_as
            beq         :+
            jsr         c_tw
            .byte       OP_DIV, V_Y, V_A, V_YA
:
            rts

; The complex number c_re + hv i (c_re if hv is 0)
c_make:
            MOVR        n_x, c_re
            MOVR        n_y, hv
            jmp         n_cpx_make

; ---- The parts: m_ziv for each

; hv = the function whose core is at .A, .X: its real part and its imaginary, each m_ziv's (or 0, known: c_zero)
c_both:
            sta         c_core
            stx         c_core + 1
            MOVR        c_mark, a_top
            ldy         #0
            jsr         c_part
            MOVR        c_re, hv
            ldy         #1
            jsr         c_part
            jsr         c_make
            jmp         mt_out

c_part:
            sty         c_k
            lda         c_zero
            and         c_bit,y
            beq         :+
            LDR         hv, n_zero
            rts
:
            MOVR        mt_core, c_core
            jsr         m_ziv
            MOVR        c_mark, a_top                       ; (The part made kept)
            rts

            NM_RODATA
c_bit:      .byte       1, 2
            NM_CODE

; A core's start: c_b = b, c_w = b + 24 (its working bits), the stack empty (the arena as c_mark left it)
c_start:
            MOVR        c_b, mt_b
            MOVR        c_w, mt_b
            ADDV        c_w, 24
            MOVR        a_top, c_mark
            stz         c_sp
            rts

; A core's end: MA, ME, mt_k10 = the top at b bits
c_tob:
            lda         #0
            tay
            jsr         c_ld
            MOVR        mt_b, c_b
            SUBVV       c_d16, c_lb, c_b
            lda         #CX
            jsr         c_shrd
            lda         #CX + 1
            jsr         c_shrd
            RCOPY       MA, CX
            RCOPY       30, CX + 1
            lda         #1
            jsr         c_pluse
            MOVR        mt_k10, c_lk
            rts

; |R(.A)| = |R(.A)| / 2^c_d16
c_shrd:
            sta         nrc
            MOVR        n_k, c_d16
            jmp         r_shr

; ---- The machine's stack: approximations, each in the arena: its B, its K, its A and its E (registers' lengths,
; signs and bytes)

; Approximation .A down (0 the top, 1 the next; $FF the one just taken off) loaded: .Y 0, X (CX, CX + 1, c_lb,
; c_lk); .Y 2, Y (CY, CY + 1, c_lb + 2, c_lk + 2)
c_ld:
            sty         c_n
            eor         #$FF                                ; (Its slot: c_sp - 1 - .A)
            clc
            adc         c_sp
            tax
            lda         c_plo,x
            sta         nt
            lda         c_phi,x
            sta         nt + 1
            ldx         c_n
            ldy         #0
            lda         (nt),y
            sta         c_lb,x
            iny
            lda         (nt),y
            sta         c_lb + 1,x
            iny
            lda         (nt),y
            sta         c_lk,x
            iny
            lda         (nt),y
            sta         c_lk + 1,x
            lda         #4
            jsr         c_ntadd
            lda         c_n
            clc
            adc         #CX
            pha
            jsr         c_rget
            pla
            inc         a
; Register .A = the one written at nt, nt moved past it
c_rget:
            tax
            lda         (nt)
            sta         r_len,x
            ldy         #1
            lda         (nt),y
            sta         r_neg,x
            txa
            RPAGE
            sta         nd + 1
            stz         nd
            lda         #2
            jsr         c_ntadd
            ldy         #0
:
            tya
            cmp         r_len,x
            beq         :+
            lda         (nt),y
            sta         (nd),y
            iny
            bra         :-
:
            tya
c_ntadd:
            clc
            adc         nt
            sta         nt
            bcc         :+
            inc         nt + 1
:
            rts

; Pushed: MA, ME, at c_rb bits, times 10^c_rk.  Past the arena's room: NE_BIG
c_push:
            lda         a_top + 1
            cmp         #>(DIGS - $300)
            bcc         :+
            jmp         n_big
:
            ldx         c_sp
            lda         a_top
            sta         c_plo,x
            sta         nu
            lda         a_top + 1
            sta         c_phi,x
            sta         nu + 1
            inc         c_sp
            ldy         #3
:
            lda         c_rb,y                              ; (c_rb, then c_rk)
            sta         (nu),y
            dey
            bpl         :-
            lda         #4
            jsr         c_nuadd
            ldx         #MA
            jsr         c_rput
            ldx         #ME
            jsr         c_rput
            MOVR        a_top, nu
            rts

; Register .X written at nu, nu moved past it
c_rput:
            lda         r_len,x
            sta         (nu)
            ldy         #1
            lda         r_neg,x
            sta         (nu),y
            txa
            RPAGE
            sta         nd + 1
            stz         nd
            lda         #2
            jsr         c_nuadd
            ldy         #0
:
            tya
            cmp         r_len,x
            beq         :+
            lda         (nd),y
            sta         (nu),y
            iny
            bra         :-
:
            tya
c_nuadd:
            clc
            adc         nu
            sta         nu
            bcc         :+
            inc         nu + 1
:
            rts

; The top two the other way round; approximation .A down pushed again (c_pick: 0 the top, 1 the next)
c_swap:
            ldx         c_sp
            lda         c_plo - 1,x
            ldy         c_plo - 2,x
            sta         c_plo - 2,x
            tya
            sta         c_plo - 1,x
            lda         c_phi - 1,x
            ldy         c_phi - 2,x
            sta         c_phi - 2,x
            tya
            sta         c_phi - 1,x
            rts
c_pick:
            eor         #$FF
            clc
            adc         c_sp
            tay
            ldx         c_sp
            lda         c_plo,y
            sta         c_plo,x
            lda         c_phi,y
            sta         c_phi,x
            inc         c_sp
            rts

; ---- The machine's numbers

; An exact value (.A): A = it at c_w bits (toward 0), E 1
c_q:
            ldy         #CX
            jsr         c_vload
            MOVR        mt_q, c_w
            lda         #CX
            ldx         #CX + 1
            ldy         #MA
            jsr         m_fixed
            RSET        ME, 1
c_pushw:
            MOVR        c_rb, c_w                           ; (At c_w bits, times 10^0)
            stz         c_rk
            stz         c_rk + 1
            jmp         c_push

; 1, exactly
c_one:
            MOVR        mt_q, c_w
            ldy         #MA
            jsr         m_pow2
            RCLR        ME
            bra         c_pushw

; Pi, within 2
c_pi:
            MOVR        mt_q, c_w
            jsr         m_pi
            RCOPY       MA, 3
            RSET        ME, 2
            bra         c_pushw

; ---- The cores, the other banks': exp, log, sin or cos, atan of an exact value (.A; trig's .X 0 sin, 1 cos)

c_expq:
            ldy         #XN
            jsr         c_vload
            MOVR        mt_b, c_w
            NMFAR       m_expx, -2
            MOVR        c_rb, c_w
            MOVR        c_rk, mt_k10
            jmp         c_push
c_logq:
            ldy         #XN
            jsr         c_vload
            MOVR        mt_b, c_w
            NMFAR       m_logcore, -2
            jmp         c_pushw
c_trigqz:                                                   ; (cos 0 is 1)
            stx         c_n2
            pha
            jsr         c_val
            jsr         n_zero_p
            bne         :+
            pla
            jmp         c_one
:
            pla
            ldx         c_n2
c_trigq:
            stx         mt_which
            ldy         #XN
            jsr         c_vload
            MOVR        mt_b, c_w
            NMFAR       b2::m_trigcore, -1
            jmp         c_pushw
c_atanq:
            ldy         #XN
            jsr         c_vload
            MOVR        mt_b, c_w
            NMFAR       b2::m_atancore, -1
            jmp         c_pushw

; arg(a + y i) (a's and y's signs c_ga, c_gy; y / a V_YA): pi/2 with y's sign if a is 0; else atan(y / a), and pi
; more (y 0 or more) or less if a is below 0
c_arg:
            lda         c_ga
            bne         @atan
            jsr         c_pi
            lda         #1
            jsr         c_shr
            lda         c_gy
            bpl         @rts
            jmp         c_neg
@atan:
            lda         #V_YA
            jsr         c_atanq
            lda         c_ga
            bpl         @rts
            jsr         c_pi
            lda         c_gy
            bmi         :+
            jmp         c_add
:
            jmp         c_sub
@rts:
            rts

; e^Z (Z the top, at its B bits: bz), at c_w bits: m_expcore's, and Z's error carried: (|A| + E) Ez / 2^(bz - 1)
c_expa:
            lda         #0
            tay
            jsr         c_ld
            dec         c_sp
            RCOPY       4, CX
            MOVR        mt_bz, c_lb
            MOVR        mt_b, c_w
            NMFAR       m_expcore, -2
            MOVR        c_rk, mt_k10
            lda         #$FF                                ; (Z again: its E)
            ldy         #2
            jsr         c_ld
            RCOPY       30, MA
            RABS        30
            RADD        30, 30, ME
            RMUL        31, 30, CY + 1
            SUBVV       c_d16, c_lb + 2, mt_one16
            lda         #31
            jsr         c_shrd
            RADD        30, ME, 31
            lda         #1
            jsr         c_pluse
            MOVR        c_rb, c_w
            jmp         c_push

; sin (.X 0) or cos (1) of Z (the top), at c_w bits: m_trigcore's of Z's A / 2^bz, and Z's error (|sin'|, |cos'| 1
; at most): Ez / 2^(bz - w) more
c_triga:
            stx         c_n2
            lda         #0
            tay
            jsr         c_ld
            dec         c_sp
            ldx         #CX
            lda         r_len,x
            bne         @core
            RCLR        MA                                  ; (sin 0 is 0, cos 0 is 1)
            RCLR        ME
            lda         c_n2
            beq         @err
            MOVR        mt_q, c_w
            ldy         #MA
            jsr         m_pow2
            bra         @err
@core:
            MOVR        mt_q, c_lb
            ldy         #CX + 1
            jsr         m_pow2
            lda         c_n2
            sta         mt_which
            MOVR        mt_b, c_w
            NMFAR       b2::m_trigcore, -1
@err:
            lda         #$FF
            ldy         #2
            jsr         c_ld
            SUBVV       c_d16, c_lb + 2, c_w
            lda         #CY + 1
            jsr         c_shrd
            RADD        30, ME, CY + 1
            lda         #1
            jsr         c_pluse
            jmp         c_pushw

; ---- The machine's arithmetic, each with its error's bound

; X = the next, Y = the top, both taken off; c_rb X's B
c_ld2:
            lda         #1
            ldy         #0
            jsr         c_ld
            lda         #0
            ldy         #2
            jsr         c_ld
            dec         c_sp
            dec         c_sp
            MOVR        c_rb, c_lb
            rts

; X = the top, taken off; c_rb, c_rk its B and K
c_ld1:
            lda         #0
            tay
            jsr         c_ld
            dec         c_sp
            MOVR        c_rb, c_lb
            MOVR        c_rk, c_lk
            rts

; ME = R30 + .A
c_pluse:
            sta         c_n
            lda         #31
            sta         nrc
            lda         c_n
            ldx         #0
            jsr         r_set16
            RADD        ME, 30, 31
            rts

; The next - the top; the next + the top: K the more of theirs, the other divided by 10^(the difference) (E / 10^d +
; 1; past 10^619, more than a register holds, 0 within 1)
c_sub:
            jsr         c_neg
c_add:
            jsr         c_ld2
            SUBVV       c_d16, c_lk, c_lk + 2
            lda         c_d16 + 1
            bmi         @x
            MOVR        c_rk, c_lk
            lda         #CY
            bra         @scale
@x:
            sec
            lda         #0
            sbc         c_d16
            sta         c_d16
            lda         #0
            sbc         c_d16 + 1
            sta         c_d16 + 1
            MOVR        c_rk, c_lk + 2
            lda         #CX
@scale:
            sta         c_n2
            lda         c_d16
            ora         c_d16 + 1
            beq         @add
            lda         c_d16 + 1
            cmp         #>620
            bcc         :+
            bne         @far
            lda         c_d16
            cmp         #<620
            bcs         @far
:
            lda         #6
            sta         nrc
            lda         c_d16
            ldx         c_d16 + 1
            jsr         r_pow10
            lda         c_n2
            jsr         c_div6
            inc         c_n2
            lda         c_n2
            jsr         c_div6
            lda         c_n2
            sta         nrc
            jsr         c_inc
            bra         @add
@far:
            ldx         c_n2
            jsr         r_clear
            lda         c_n2
            inc         a
            sta         nrc
            lda         #1
            ldx         #0
            jsr         r_set16
@add:
            RADD        MA, CX, CY
            RADD        ME, CX + 1, CY + 1
            jmp         c_push

; R(.A) = R(.A) / R6, toward 0
c_div6:
            sta         c_n
            sta         nra
            lda         #6
            sta         nrb
            lda         #30
            sta         nrq
            lda         #31
            sta         nrr
            jsr         r_divmod
            lda         #30
            sta         nra
            lda         c_n
            sta         nrc
            jmp         r_copy

; R(nrc) = R(nrc) + 1 (at or above 0)
c_inc:
            lda         nrc
            pha
            lda         #31
            sta         nrc
            lda         #1
            ldx         #0
            jsr         r_set16
            pla
            sta         nrc
            sta         nra
            lda         #31
            sta         nrb
            jmp         r_add

; -(the top)
c_neg:
            jsr         c_ld1
            RCOPY       MA, CX
            RNEG        MA
            RCOPY       ME, CX + 1
            jmp         c_push

; The next times the top: A = Ax Ay / 2^B, E = (|Ax| Ey + |Ay| Ex + Ex Ey) / 2^B + 2, K their sum
c_mul:
            jsr         c_ld2
            ADDVV       c_rk, c_lk, c_lk + 2
            RMUL        MA, CX, CY
            lda         #MA
            jsr         c_shrb
            RABS        CX
            RABS        CY
            RMUL        30, CX, CY + 1
            RMUL        31, CY, CX + 1
            RADD        30, 30, 31
            RMUL        31, CX + 1, CY + 1
            RADD        30, 30, 31
            lda         #30
            jsr         c_shrb
            lda         #2
            jsr         c_pluse
            jmp         c_push

; |R(.A)| = |R(.A)| / 2^c_rb; << (c_shlb)
c_shrb:
            sta         nrc
            MOVR        n_k, c_rb
            jmp         r_shr
c_shlb:
            sta         nrc
            MOVR        n_k, c_rb
            jmp         r_shl

; The next over the top: A = Ax 2^B / Ay, E = (Ex |Ay| + |Ax| Ey) 2^B / ((|Ay| - Ey) |Ay|) + 2, K Kx - Ky; if Ay
; isn't known from 0 (|Ay| no more than Ey), A 0 and E 2^2B (more bits)
c_div:
            jsr         c_ld2
            SUBVV       c_rk, c_lk, c_lk + 2
            RCOPY       30, CY
            RABS        30
            RUCMP       30, CY + 1
            cmp         #1
            beq         :+
            RCLR        MA
            ADDVV       mt_q, c_rb, c_rb
            ldy         #ME
            jsr         m_pow2
            jmp         c_push
:
            RCOPY       0, CX
            lda         #0
            jsr         c_shlb
            RDIV        MA, 1, 0, CY
            RMUL        0, CX + 1, 30
            RABS        CX
            RMUL        1, CX, CY + 1
            RADD        0, 0, 1
            lda         #0
            jsr         c_shlb
            RSUB        1, 30, CY + 1
            RMUL        2, 1, 30
            RDIV        30, 3, 0, 2
            lda         #2
            jsr         c_pluse
            jmp         c_push

; The square root of the top (0 or more): K made even (A and E times 10), A = isqrt(A 2^B), E = E 2^B /
; (2 isqrt((A - E) 2^B)) + 2, K / 2; if A - E isn't above 0, A 0 and E isqrt((A + E) 2^B) + 2
c_sqrt:
            jsr         c_ld1
            lda         c_rk
            lsr
            bcc         :+
            lda         #CX
            jsr         c_ten
            lda         #CX + 1
            jsr         c_ten
            lda         c_rk
            bne         @dec
            dec         c_rk + 1
@dec:
            dec         c_rk
:
            lda         c_rk + 1                            ; (K / 2)
            cmp         #$80
            ror         c_rk + 1
            ror         c_rk
            RSUB        30, CX, CX + 1
            ldx         #30
            lda         r_len,x
            beq         @low
            lda         r_neg,x
            bne         @low
            RCOPY       31, CX
            lda         #31
            jsr         c_shlb
            lda         #31
            ldy         #MA
            jsr         c_isqrt
            lda         #30
            jsr         c_shlb
            lda         #30
            ldy         #2
            jsr         c_isqrt
            RADD        3, 2, 2
            RCOPY       0, CX + 1
            lda         #0
            jsr         c_shlb
            RDIV        30, 1, 0, 3
            lda         #2
            jsr         c_pluse
            jmp         c_push
@low:
            RCLR        MA
            RADD        31, CX, CX + 1
            lda         #31
            jsr         c_shlb
            lda         #31
            ldy         #30
            jsr         c_isqrt
            lda         #2
            jsr         c_pluse
            jmp         c_push

; R(.A) times 10
c_ten:
            sta         nrc
            lda         #10
            jmp         r_mulsmall

; R(.Y) = floor(sqrt(R(.A))): m_isqrt's, in the first bank
c_isqrt:
            NMFAR       m_isqrt, -2
            rts

; The top / 2^.A: A and E shifted, E 1 more
c_shr:
            sta         c_d16
            stz         c_d16 + 1
            jsr         c_ld1
            lda         #CX
            jsr         c_shrd
            lda         #CX + 1
            jsr         c_shrd
            RCOPY       MA, CX
            RCOPY       30, CX + 1
            lda         #1
            jsr         c_pluse
            jmp         c_push

; The top times exact value .A (n / d): A = Ax n / d, E = (Ex |n| + d) / d + 1
c_mulq:
            pha
            jsr         c_ld1
            pla
            ldy         #CY
            jsr         c_vload
            RMUL        30, CX, CY
            RDIV        MA, 31, 30, CY + 1
            RMUL        30, CX + 1, CY
            RABS        30
            RADD        30, 30, CY + 1
            RDIV        0, 31, 30, CY + 1
            RCOPY       30, 0
            lda         #1
            jsr         c_pluse
            jmp         c_push

            NM_RODATA
k_one:      .byte       $01                                 ; (1, 2, -1)
k_two:      .byte       $02
k_mone:     .byte       $7F
k_half:     .byte       NT_RATIONAL, $01, $02               ; (1/2, -1/2; 0.5)
k_mhalf:    .byte       NT_RATIONAL, $7F, $02
k_halff:    .byte       NT_FIXED + 1, $05
k_q1:       .byte       NT_RATIONAL, $01, $04               ; (1/4, -1/4, 3/4, -3/4)
k_q2:       .byte       NT_RATIONAL, $7F, $04
k_q3:       .byte       NT_RATIONAL, $03, $04
k_q4:       .byte       NT_RATIONAL, $7D, $04
k_qs:       .addr       k_q1, k_q2, k_q3, k_q4
            NM_CODE
