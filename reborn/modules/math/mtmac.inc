; ****************************************************************************
; mtmac.inc - the math library's macros for its registers and words (mtcore.inc's): included once, before the code
; of both its banks

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
