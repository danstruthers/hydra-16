; ****************************************************************************
; mtcore.inc - the math library's way of working (danlang's NumMath.cs, as sim/tools/numref.js has it): a function's
; value worked in binary fixed point, an integer A in a register standing for A / 2^b, within E / 2^b of the true
; value (and that times 10^k), then rounded to the precision's significant digits only if everything within E rounds
; the same; else worked again with more bits (Ziv's way).  So the digits are the true value's, correctly rounded,
; whatever way it's worked: these routines' or danlang's.  Here: the registers' arithmetic at a number of bits, the
; constants (pi, log 2, log 10) at any, the rounding, and the loop.
;   Registers: XN, XD the argument (its numerator and denominator), YN, YD a power's; MA, ME a core's A and E; R0-R3
; the constants' (m_pi ... leave theirs in R3), R6 and R7 the helpers' (m_fixed, m_isqrt ...); the rest a core's.

XN              = 24            ; The argument x: its numerator and (positive) denominator
XD              = 25
YN              = 26            ; A power's y
YD              = 27
MA              = 28            ; A core's approximation, at mt_b bits ...
ME              = 29            ;   and its error's bound
MAX_DIGITS      = 100           ; The precision at most (DIGITS)
EXP_MOST        = 20000         ; exp's argument, and a power's y log x, at most this far from 0

.segment "NUMBANK"
mt_n:       .res        1                                   ; The precision (significant digits)
mt_need:    .res        2                                   ; (The bits a value needs: the precision's, and 8)
mt_b:       .res        2                                   ; (A core's bits: its A's)
mt_w:       .res        2                                   ; (A core's own bits, more: its guard)
mt_k10:     .res        2                                   ; (A core's power of 10: its value A / 2^b times 10^k)
mt_core:    .res        2                                   ; (The core: m_ziv's)
mt_q:       .res        2                                   ; (Bits: a shift's, a constant's)
mt_d:       .res        2                                   ; (m_div16's divisor; a series' term's number)
mt_r0:      .res        3                                   ; (m_div16's remainder)
mt_q8:      .res        1
mt_t:       .res        2                                   ; (Scratch: counts, bits, an exponent)
mt_u:       .res        2
mt_v:       .res        2
mt_i:       .res        2                                   ; (A series' term's number, and how many)
mt_cnt:     .res        2
mt_p1:      .res        2                                   ; (The rounding's places, each end's)
mt_p2:      .res        2
mt_p:       .res        2
mt_e:       .res        2
mt_f:       .res        1                                   ; (Flags: a core's signs, which function, a quadrant)
mt_g:       .res        1
mt_which:   .res        1
mt_m:       .res        1                                   ; (atan's 1/m)
mt_rs:      .res        1                                   ; (m_round's sign)
mt_xneg:    .res        1                                   ; (An entry's: its argument below 0)
mt_zn:      .res        1                                   ; (A core's flags: log's z below 0, trig's quadrant,
mt_quad:    .res        1                                   ;   atan's 1 / y and y' below 0)
mt_inv:     .res        1
mt_yneg:    .res        1
mt_sx:      .res        1                                   ; (m_sig's x and its result: registers)
mt_sy:      .res        1
mt_bl:      .res        2                                   ; (m_bitlen's)
mt_bt:      .res        1
mt_cid:     .res        1                                   ; (m_const's constant: 0 pi, 1 log 2, 2 log 10)
mt_cq:      .res        2                                   ; (The bits asked for, and made at)
mt_cm:      .res        2
mt_s6:      .res        2                                   ; (m_round's: the power of 10 R6 holds (m_sig's 10^|s|),
mt_s6ok:    .res        1                                   ;   if it holds one, and whether R31 is 2^(b - 1))
mt_b31ok:   .res        1

.code

; ---- The registers, by number (R, M ...: the macros' first argument the result's)

.macro RSET dst, v                                          ; R(dst) = v (16 bits, signed)
            lda         #dst
            sta         nrc
            lda         #<(v)
            ldx         #>(v)
            jsr         r_set16
.endmacro
.macro RSETV dst, var                                       ; R(dst) = var (16 bits, signed)
            lda         #dst
            sta         nrc
            lda         var
            ldx         var + 1
            jsr         r_set16
.endmacro
.macro RCOPY dst, src                                       ; R(dst) = R(src)
            lda         #src
            sta         nra
            lda         #dst
            sta         nrc
            jsr         r_copy
.endmacro
.macro RADD rc, ra, rb                                      ; R(rc) = R(ra) + R(rb)
            lda         #ra
            sta         nra
            lda         #rb
            sta         nrb
            lda         #rc
            sta         nrc
            jsr         r_add
.endmacro
.macro RSUB rc, ra, rb                                      ; R(rc) = R(ra) - R(rb)
            lda         #ra
            sta         nra
            lda         #rb
            sta         nrb
            lda         #rc
            sta         nrc
            jsr         r_sub
.endmacro
.macro RMUL rc, ra, rb                                      ; R(rc) = R(ra) * R(rb) (rc another register)
            lda         #ra
            sta         nra
            lda         #rb
            sta         nrb
            lda         #rc
            sta         nrc
            jsr         r_mul
.endmacro
.macro RMULW rc, ra, rb                                     ; R(rc) = R(ra) * R(rb) / 2^w (rc another register)
            RMUL        rc, ra, rb
            lda         #rc
            jsr         m_shrw
.endmacro
.macro RDIV rq, rr, ra, rb                                  ; R(rq) = R(ra) / R(rb) (toward 0), R(rr) the remainder
            lda         #ra
            sta         nra
            lda         #rb
            sta         nrb
            lda         #rq
            sta         nrq
            lda         #rr
            sta         nrr
            jsr         r_divmod
.endmacro
.macro RSHR r, var                                          ; |R(r)| = |R(r)| / 2^var
            lda         #r
            sta         nrc
            MOVR        n_k, var
            jsr         r_shr
.endmacro
.macro RSHL r, var                                          ; |R(r)| = |R(r)| * 2^var
            lda         #r
            sta         nrc
            MOVR        n_k, var
            jsr         r_shl
.endmacro
.macro RSHRN r, n                                           ; |R(r)| = |R(r)| / 2^n (a constant)
            lda         #r
            sta         nrc
            LDR         n_k, n
            jsr         r_shr
.endmacro
.macro RSHLN r, n                                           ; |R(r)| = |R(r)| * 2^n (a constant)
            lda         #r
            sta         nrc
            LDR         n_k, n
            jsr         r_shl
.endmacro
.macro RNEG r                                               ; R(r) = -R(r)
            ldx         #r
            jsr         r_negate
.endmacro
.macro RABS r                                               ; R(r) = |R(r)|
            ldx         #r
            stz         r_neg,x
.endmacro
.macro RCLR r                                               ; R(r) = 0
            ldx         #r
            jsr         r_clear
.endmacro
.macro RDIV16 r, var                                        ; |R(r)| = |R(r)| / var (16 bits, 1 or more)
            lda         #r
            sta         nrc
            MOVR        mt_d, var
            jsr         m_div16
.endmacro
.macro RUCMP ra, rb                                         ; .A = |R(ra)| against |R(rb)|: $FF less, 0, 1 more
            lda         #ra
            sta         nra
            lda         #rb
            sta         nrb
            jsr         r_ucmp
.endmacro
.macro ADDV var, v                                          ; var += v (16 bits, a constant)
            clc
            lda         var
            adc         #<(v)
            sta         var
            lda         var + 1
            adc         #>(v)
            sta         var + 1
.endmacro
.macro ADDVV vt, va, vb                                     ; vt = va + vb (16 bits)
            clc
            lda         va
            adc         vb
            sta         vt
            lda         va + 1
            adc         vb + 1
            sta         vt + 1
.endmacro
.macro SUBVV vt, va, vb                                     ; vt = va - vb (16 bits)
            sec
            lda         va
            sbc         vb
            sta         vt
            lda         va + 1
            sbc         vb + 1
            sta         vt + 1
.endmacro

; R(.A) = R(.A) / 2^w (its magnitude)
m_shrw:
            sta         nrc
            MOVR        n_k, mt_w
            jmp         r_shr

; Register .A: Z = 1 if it's 0, N = 1 if it's below 0 (r_neg's)
m_sign:
            tax
            lda         r_len,x
            beq         :+
            lda         r_neg,x
            ora         #1                                  ; (Z = 0; N its sign)
:
            rts

; .A, .X = register .A's bits (its magnitude's: 0 for 0)
m_bitlen:
            tax
            lda         r_len,x
            bne         :+
            ldx         #0
            rts
:
            sta         mt_bl
            txa
            jsr         r_page
            sta         nd + 1
            stz         nd
            ldy         mt_bl
            dey
            lda         (nd),y                              ; (Its top byte's bits)
            ldy         #0
:
            iny
            lsr
            bne         :-
            sty         mt_bt
            lda         mt_bl                               ; ((len - 1) * 8 + them)
            dec         a
            stz         mt_bl + 1
            asl
            rol         mt_bl + 1
            asl
            rol         mt_bl + 1
            asl
            rol         mt_bl + 1
            clc
            adc         mt_bt
            ldx         mt_bl + 1
            bcc         :+
            inx
:
            rts

; |R(nrc)| = |R(nrc)| / mt_d (1 to 65535), in place
m_div16:
            lda         mt_d + 1                            ; (Under 256: r_divsmall's, quicker)
            bne         :+
            lda         mt_d
            jmp         r_divsmall
:
            jsr         r_c
            ldx         nrc
            ldy         r_len,x
            stz         mt_r0
            stz         mt_r0 + 1
            stz         mt_r0 + 2
@byte:
            dey
            cpy         #$FF
            beq         @done
            lda         (nc),y
            sta         mt_q8
            ldx         #8
@bit:
            asl         mt_q8
            rol         mt_r0
            rol         mt_r0 + 1
            rol         mt_r0 + 2
            lda         mt_r0 + 2                           ; (The remainder not less than the divisor: taken)
            bne         @take
            lda         mt_r0 + 1
            cmp         mt_d + 1
            bcc         @no
            bne         @take
            lda         mt_r0
            cmp         mt_d
            bcc         @no
@take:
            sec
            lda         mt_r0
            sbc         mt_d
            sta         mt_r0
            lda         mt_r0 + 1
            sbc         mt_d + 1
            sta         mt_r0 + 1
            lda         mt_r0 + 2
            sbc         #0
            sta         mt_r0 + 2
            inc         mt_q8
@no:
            dex
            bne         @bit
            lda         mt_q8
            sta         (nc),y
            bra         @byte
@done:
            jmp         r_normc

; R(.Y) = 2^mt_q (.Y not R6 or R7)
m_pow2:
            sty         nrc
            lda         #1
            ldx         #0
            jsr         r_set16
            MOVR        n_k, mt_q
            jmp         r_shl

; R(mt_f's register: .Y) = R(.A) * 2^mt_q / R(.X), toward 0: a fraction at mt_q bits.  Uses R6, R7
m_fixed:
            sty         mt_g
            sta         nra
            stx         nrb
            lda         #6
            sta         nrc
            jsr         r_copy
            RSHL        6, mt_q
            lda         #6
            sta         nra
            lda         mt_g
            sta         nrq
            lda         #7
            sta         nrr
            jmp         r_divmod

; ---- The constants, at mt_q bits, in R3 (each within 2 ulps).  Use R0-R3

; R2 = atan(1/.A) * 2^mt_t (.A 2-255; its square by .A twice), within an ulp a term.  Uses R0, R1
m_atan_inv:
            ldx         #0
            bra         m_series_inv
; R2 = atanh(1/.A) * 2^mt_t, likewise
m_atanh_inv:
            ldx         #1
m_series_inv:
            sta         mt_m
            stx         mt_g
            ldy         #0                                  ; (t = 2^bits / m)
            MOVR        mt_q, mt_t
            jsr         m_pow2
            lda         mt_m
            jsr         r_divsmall
            RCLR        2
            LDR         mt_i, 0
@term:
            ldx         #0                                  ; (Each term t / (2k + 1), t / m^2 the next t)
            lda         r_len,x
            bne         @lb201
            jmp         @done
@lb201:
            RCOPY       1, 0
            lda         mt_i                                ; (2k + 1)
            asl
            sta         mt_d
            lda         mt_i + 1
            rol
            sta         mt_d + 1
            inc         mt_d
            lda         #1
            sta         nrc
            jsr         m_div16
            lda         mt_g                                ; (atan's every other one less)
            bne         @plus
            lda         mt_i
            lsr
            bcc         @plus
            RSUB        2, 2, 1
            bra         @next
@plus:
            RADD        2, 2, 1
@next:
            lda         #0
            sta         nrc
            lda         mt_m
            jsr         r_divsmall
            lda         #0
            sta         nrc
            lda         mt_m
            jsr         r_divsmall
            ADDV        mt_i, 1
            jmp         @term
@done:
            rts

; mt_t = mt_q + the constants' guard (bits(mt_q) + 10)
m_guard:
            lda         #2                                  ; (mt_q's bits: R2 a while)
            sta         nrc
            lda         mt_q
            ldx         mt_q + 1
            jsr         r_set16
            lda         #2
            jsr         m_bitlen
            clc
            adc         #10
            sta         mt_u
            stz         mt_u + 1
            ADDVV       mt_t, mt_q, mt_u
            rts

; R3 = pi, log 2 or log 10 at mt_q bits, from the cache (the bank's state: st_const) if it holds them at as many bits
; or more (shifted down), else made at mt_q bits or 192 if that's more, and kept.  Uses R0-R3
m_pi:
            lda         #0
            bra         m_const
m_ln2:
            lda         #1
            bra         m_const
m_ln10:
            lda         #2
m_const:
            sta         mt_cid
            asl
            tax
            lda         st_cbits + 1,x                      ; (Cached at mt_q bits or more?)
            cmp         mt_q + 1
            bcc         @make
            bne         @hit
            lda         st_cbits,x
            cmp         mt_q
            bcc         @make
@hit:
            lda         st_cbits,x                          ; (Its bytes to R3, shifted down to mt_q bits)
            sec
            sbc         mt_q
            sta         n_k
            lda         st_cbits + 1,x
            sbc         mt_q + 1
            sta         n_k + 1
            jsr         m_cslot
            ldx         #3
            stz         r_neg,x
            ldy         #0
:
            lda         (nd),y
            sta         regs + 3 * 256,y
            iny
            cpy         #CONST_BYTES
            bne         :-
            tya
            sta         r_len,x
            lda         #3
            sta         nrc
            jsr         r_c
            jsr         r_normc
            jmp         r_shr
@make:
            MOVR        mt_cq, mt_q                         ; (Made at 192 bits at least, kept if it fits)
            lda         mt_q + 1
            bne         :+
            lda         mt_q
            cmp         #192
            bcs         :+
            LDR         mt_q, 192
:
            MOVR        mt_cm, mt_q                         ; (The bits it's made at: the makers change mt_q)
            lda         mt_cid
            beq         @pi
            cmp         #1
            beq         @ln2
            jsr         m_ln10_0
            bra         @made
@ln2:
            jsr         m_ln2_0
            bra         @made
@pi:
            jsr         m_pi_0
@made:
            ldx         #3                                  ; (Kept: its bytes, then 0s, if they fit)
            lda         r_len,x
            cmp         #CONST_BYTES + 1
            bcs         @shift
            sta         mt_bt
            jsr         m_cslot
            ldy         #0
:
            cpy         mt_bt
            bcs         @zero
            lda         regs + 3 * 256,y
            bra         :+
@zero:
            lda         #0
:
            sta         (nd),y
            iny
            cpy         #CONST_BYTES
            bne         :--
            lda         mt_cid
            asl
            tax
            lda         mt_cm
            sta         st_cbits,x
            lda         mt_cm + 1
            sta         st_cbits + 1,x
@shift:
            SUBVV       n_k, mt_cm, mt_cq                   ; (Down to the bits asked for)
            MOVR        mt_q, mt_cq
            lda         #3
            sta         nrc
            jmp         r_shr

; nd = the cache's slot for constant mt_cid (its CONST_BYTES; a register's bytes past its length are 0 there)
m_cslot:
            lda         #<st_const
            sta         nd
            lda         #>st_const
            sta         nd + 1
            ldx         mt_cid
            beq         @rts
:
            clc
            lda         nd
            adc         #CONST_BYTES
            sta         nd
            bcc         :+
            inc         nd + 1
:
            dex
            bne         :--
@rts:
            rts

; R3 = pi at mt_q bits: 16 atan(1/5) - 4 atan(1/239), Machin's
m_pi_0:
            jsr         m_guard
            lda         #5
            jsr         m_atan_inv
            RSHLN       2, 4
            RCOPY       3, 2
            lda         #239
            jsr         m_atan_inv
            RSHLN       2, 2
            RSUB        3, 3, 2
            RSHR        3, mt_u
            rts

; R3 = log 2 at mt_q bits: 2 atanh(1/3)
m_ln2_0:
            jsr         m_guard
            lda         #3
            jsr         m_atanh_inv
            RSHLN       2, 1
            RCOPY       3, 2
            RSHR        3, mt_u
            rts

; R3 = log 10 at mt_q bits: 6 atanh(1/3) + 2 atanh(1/9)
m_ln10_0:
            jsr         m_guard
            lda         #3
            jsr         m_atanh_inv
            RCOPY       3, 2
            lda         #3
            sta         nrc
            lda         #6
            jsr         r_mulsmall
            lda         #9
            jsr         m_atanh_inv
            RSHLN       2, 1
            RADD        3, 3, 2
            RSHR        3, mt_u
            rts

; ---- Rounding to the precision

; R(.A) = R(.A) / 2^(mt_w - mt_b): a core's result at mt_b bits
m_tob:
            sta         nrc
            SUBVV       n_k, mt_w, mt_b
            jmp         r_shr

; mt_e = floor(.A, .X * 77 / 256) (.A, .X signed: log10 2's 0.30078; near enough, as m_sig moves on from it)
m_log10x:
            sta         mt_u
            stx         mt_u + 1
            ldy         #0
            txa
            bpl         :+
            dey                                             ; (Below 0: its magnitude, the floor its ceiling's negative)
            sec
            lda         #0
            sbc         mt_u
            sta         mt_u
            lda         #0
            sbc         mt_u + 1
            sta         mt_u + 1
:
            sty         mt_g
            stz         mt_e                                ; (mt_e.mt_e+1.mt_v = mt_u * 77, 24 bits: 77's 7 bits,
            stz         mt_e + 1                            ;   the top first, a shift and an add each)
            stz         mt_v
            lda         #77 << 1
            sta         mt_q8
            ldx         #7
@bit:
            asl         mt_v
            rol         mt_e
            rol         mt_e + 1
            asl         mt_q8
            bcc         @next
            clc
            lda         mt_v
            adc         mt_u
            sta         mt_v
            lda         mt_e
            adc         mt_u + 1
            sta         mt_e
            bcc         @next
            inc         mt_e + 1
@next:
            dex
            bne         @bit
            lda         mt_g                                ; (Its high 16 bits the floor; below 0, up if any's left)
            beq         @rts
            lda         mt_v
            beq         :+
            inc         mt_e
            bne         :+
            inc         mt_e + 1
:
            sec
            lda         #0
            sbc         mt_e
            sta         mt_e
            lda         #0
            sbc         mt_e + 1
            sta         mt_e + 1
@rts:
            rts

; R(.Y) = R(.A) rounded to mt_n significant digits, x = R(.A) / 2^mt_b times 10^mt_k10 (above 0): mt_p its places
; (R .Y * 10^-mt_p the value).  R4 = 10^(n - 1) and R5 = 10^n already.  Uses R0, R1, R6, R7, R30, R31
m_sig:
            sta         mt_sx
            sty         mt_sy
            jsr         m_bitlen                            ; (e = floor((bits - 1 - b) log10 2) + k10)
            sec
            sbc         #1
            bcs         :+
            dex
:
            sec
            sbc         mt_b
            pha
            txa
            sbc         mt_b + 1
            tax
            pla
            jsr         m_log10x
            ADDVV       mt_e, mt_e, mt_k10
            sec                                             ; (p = n - 1 - e)
            lda         mt_n
            sbc         #1
            sta         mt_p
            stz         mt_p + 1
            SUBVV       mt_p, mt_p, mt_e
@try:
            ADDVV       mt_t, mt_k10, mt_p                  ; (s = k10 + p)
            lda         mt_b31ok                            ; (2^(b - 1): R31, unless it's there)
            bne         :+
            SUBVV       mt_q, mt_b, mt_one16
            ldy         #31
            jsr         m_pow2
            inc         mt_b31ok
:
            lda         mt_t + 1
            bmi         @down
            jsr         m_p10s                              ; (s 0 or more: (x 10^s + 2^(b - 1)) / 2^b; 10^s R6)
            lda         mt_sx
            sta         nra
            lda         #6
            sta         nrb
            lda         #7
            sta         nrc
            jsr         r_mul
            RADD        7, 7, 31
            RSHR        7, mt_b
            lda         mt_sy
            sta         nrc
            lda         #7
            sta         nra
            jsr         r_copy
            bra         @check
@down:
            jsr         m_p10s                              ; (Below 0: ((x + 2^(b - 1) 10^-s) / 2^b) / 10^-s; 10^-s R6)
            RMUL        7, 31, 6
            lda         mt_sx
            sta         nra
            lda         #7
            sta         nrb
            lda         #7
            sta         nrc
            jsr         r_add
            RSHR        7, mt_b
            lda         #7
            sta         nra
            lda         #6
            sta         nrb
            lda         mt_sy
            sta         nrq
            lda         #30
            sta         nrr
            jsr         r_divmod
@check:
            lda         mt_sy                               ; (Its digits: n of them, else the places moved)
            sta         nra
            lda         #5
            sta         nrb
            jsr         r_ucmp
            cmp         #$FF
            beq         :+
            lda         mt_p                                ; (10^n or more: a place fewer)
            bne         @less
            dec         mt_p + 1
@less:
            dec         mt_p
            jmp         @try
:
            lda         mt_sy
            sta         nra
            lda         #4
            sta         nrb
            jsr         r_ucmp
            cmp         #$FF
            bne         @rts
            ADDV        mt_p, 1                             ; (Below 10^(n - 1): a place more)
            jmp         @try
@rts:
            rts

; R6 = 10^|s| (s: mt_t), unless it holds it already (mt_s6ok, mt_s6: a rounding's own)
m_p10s:
            lda         mt_s6ok
            beq         @make
            lda         mt_t
            cmp         mt_s6
            bne         @make
            lda         mt_t + 1
            cmp         mt_s6 + 1
            beq         @rts
@make:
            lda         mt_t
            ldx         mt_t + 1
            bpl         :+
            sec
            lda         #0
            sbc         mt_t
            pha
            lda         #0
            sbc         mt_t + 1
            tax
            pla
:
            ldy         #6
            sty         nrc
            jsr         r_pow10
            MOVR        mt_s6, mt_t
            lda         #1
            sta         mt_s6ok
@rts:
            rts

; R4 = 10^(n - 1), R5 = 10^n (n: mt_n): from the state if they're kept for n, else made and kept
m_p10:
            lda         mt_n
            cmp         st_p10n
            beq         @copy
            lda         #4
            sta         nrc
            lda         mt_n
            dec         a
            ldx         #0
            jsr         r_pow10
            lda         #5
            sta         nrc
            lda         mt_n
            ldx         #0
            jsr         r_pow10
            ldx         #4                                  ; (Kept: their lengths and bytes)
            lda         r_len,x
            sta         st_p10l
            ldx         #5
            lda         r_len,x
            sta         st_p10l + 1
            ldy         #P10_BYTES - 1
:
            lda         regs + 4 * 256,y
            sta         st_p10,y
            lda         regs + 5 * 256,y
            sta         st_p10 + P10_BYTES,y
            dey
            bpl         :-
            lda         mt_n
            sta         st_p10n
            rts
@copy:
            ldy         #P10_BYTES - 1
:
            lda         st_p10,y
            sta         regs + 4 * 256,y
            lda         st_p10 + P10_BYTES,y
            sta         regs + 5 * 256,y
            dey
            bpl         :-
            ldx         #4
            lda         st_p10l
            sta         r_len,x
            stz         r_neg,x
            ldx         #5
            lda         st_p10l + 1
            sta         r_len,x
            stz         r_neg,x
            rts

; hv = MA / 2^mt_b times 10^mt_k10, within ME, rounded to mt_n digits: C = 0.  C = 1 if what ME allows doesn't
; round the same.  Uses R0-R7, R30, R31
m_round:
            stz         mt_s6ok                             ; (R6 and R31 not m_sig's yet)
            stz         mt_b31ok
            jsr         m_p10                               ; (10^(n - 1), 10^n: R4, R5)
            ldx         #MA
            lda         r_neg,x
            sta         mt_rs                               ; (Its sign)
            RCOPY       2, MA                               ; (|A| - E, above 0: R2; |A| + E: R3)
            RABS        2
            RCOPY       3, 2
            lda         #2
            sta         nra
            lda         #ME
            sta         nrb
            lda         #2
            sta         nrc
            jsr         r_usub_safe
            bcs         @not
            RADD        3, 3, ME
            lda         #2
            ldy         #0
            jsr         m_sig
            MOVR        mt_p1, mt_p
            lda         #3
            ldy         #1
            jsr         m_sig
            lda         mt_p                                ; (The same places, the same digits)
            cmp         mt_p1
            bne         @not
            lda         mt_p + 1
            cmp         mt_p1 + 1
            bne         @not
            RUCMP       0, 1
            cmp         #0
            bne         @not
            lda         mt_rs                               ; (Its sign)
            ldx         #0
            sta         r_neg,x
            jmp         m_fixout
@not:
            sec
            rts

; hv = R0 / 10^mt_p, a fixed decimal (R0 its digits; mt_p below 0, R0 * 10^-mt_p of no places).  C = 0
m_fixout:
            lda         mt_p + 1
            bpl         @places
            sec
            lda         #0
            sbc         mt_p
            pha
            lda         #0
            sbc         mt_p + 1
            tax
            lda         #0
            sta         nrc
            pla
            jsr         r_scale10
            stz         mt_p
            stz         mt_p + 1
@places:
            MOVR        n_pl, mt_p
            jsr         n_fix0
            clc
            rts

; |R(nrc)| = |R(nra)| - |R(nrb)|, if that's above 0: C = 0; else C = 1
r_usub_safe:
            jsr         r_ucmp
            cmp         #1
            beq         :+
            sec
            rts
:
            jsr         r_usub
            ldx         nrc
            stz         r_neg,x
            clc
            rts

; ---- The loop

; hv = the value of the core at mt_core, rounded to the precision (mt_n): it worked at more bits each time till it
; rounds surely.  Past what the registers hold: NE_BIG
m_ziv:
            lda         st_digits
            sta         mt_n
            sta         mt_need                             ; (need = n * 3 + n / 3 + 8, near n log2 10 + 8)
            stz         mt_need + 1
            asl         mt_need
            rol         mt_need + 1
            clc
            adc         mt_need
            sta         mt_need
            bcc         :+
            inc         mt_need + 1
:
            lda         mt_n
            ldx         #3
            jsr         m_divax
            clc
            adc         mt_need
            sta         mt_need
            bcc         :+
            inc         mt_need + 1
:
            ADDV        mt_need, 8
            MOVR        mt_b, mt_need
            ADDV        mt_b, 2
@try:
            jsr         @core
            jsr         m_round
            bcs         :+
            rts
:
            lda         #MA                                 ; (More bits: the more of b / 2, and what A lacks)
            jsr         m_bitlen
            sta         mt_t
            stx         mt_t + 1
            lda         #ME
            jsr         m_bitlen
            sta         mt_u
            stx         mt_u + 1
            SUBVV       mt_t, mt_need, mt_t                 ; (need - have + 8: have = bits(A) - bits(E))
            ADDVV       mt_t, mt_t, mt_u
            ADDV        mt_t, 8
            lda         mt_b + 1                            ; (b / 2)
            lsr
            sta         mt_u + 1
            lda         mt_b
            ror
            sta         mt_u
            lda         mt_t + 1                            ; (the more)
            bmi         @half
            cmp         mt_u + 1
            bcc         @half
            bne         @add
            lda         mt_t
            cmp         mt_u
            bcs         @add
@half:
            MOVR        mt_t, mt_u
@add:
            ADDVV       mt_b, mt_b, mt_t
            lda         mt_b + 1                            ; (Past 1,800 bits: a product won't fit)
            cmp         #>1800
            bcs         @lb200
            jmp         @try
@lb200:
            jmp         n_big
@core:
            jmp         (mt_core)

; .A = .A / .X (8 bits)
m_divax:
            stx         mt_q8
            ldx         #8
            sta         mt_t
            lda         #0
@bit:
            asl         mt_t
            rol
            cmp         mt_q8
            bcc         :+
            sbc         mt_q8
            inc         mt_t
:
            dex
            bne         @bit
            lda         mt_t
            rts

.rodata
mt_one16:   .word       1
