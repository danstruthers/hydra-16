; ****************************************************************************
; fmath.inc - forth's multiplication and division (included by forth.s): two routines, 16 x 16 to 32 bits and 32 / 16
; to 16 and 16, under all of them.  Division is symmetric (/ and MOD as SM/REM); a divisor of 0 throws -10

; numacc (4) = tmp * tmp2, unsigned.  Modifies .A, .Y
umul16:
            stz         numacc + 2
            stz         numacc + 3
            ldy         #16
@bit:
            lsr         tmp + 1                             ; (The multiplier's next bit)
            ror         tmp
            bcc         :+
            clc
            lda         numacc + 2
            adc         tmp2
            sta         numacc + 2
            lda         numacc + 3
            adc         tmp2 + 1
            sta         numacc + 3
:
            ror         numacc + 3
            ror         numacc + 2
            ror         numacc + 1
            ror         numacc
            dey
            bne         @bit
            rts

; numacc (4: the dividend) / tmp (not 0), unsigned: numacc's low cell the quotient (16 bits: more, undefined), its
; high cell the remainder.  Modifies .A, .Y, tmp2, cnt
udiv16:
            ldy         #16
@bit:
            asl         numacc
            rol         numacc + 1
            rol         numacc + 2
            rol         numacc + 3
            lda         #0
            rol         a                                   ; (The 17th bit)
            sta         cnt
            sec
            lda         numacc + 2
            sbc         tmp
            sta         tmp2
            lda         numacc + 3
            sbc         tmp + 1
            sta         tmp2 + 1
            lda         cnt
            sbc         #0
            bcc         :+                                  ; (Less than the divisor)
            lda         tmp2
            sta         numacc + 2
            lda         tmp2 + 1
            sta         numacc + 3
            inc         numacc                              ; (The quotient's bit)
:
            dey
            bne         @bit
            rts

; tmp = the top (not 0: else THROW -10), dropped
divisor:
            lda         dlo,x
            sta         tmp
            lda         dhi,x
            sta         tmp + 1
            inx
            ora         tmp
            bne         :+
            lda         #<-10
            jmp         throw_a
:
            rts

            HEADER      "um*", 0
umstar:                                                     ; ( u1 u2 -- ud )
            lda         dlo,x
            sta         tmp
            lda         dhi,x
            sta         tmp + 1
            lda         dlo + 1,x
            sta         tmp2
            lda         dhi + 1,x
            sta         tmp2 + 1
            jsr         umul16
put_acc:                                                    ; The top two = numacc (the low cell under)
            lda         numacc
            sta         dlo + 1,x
            lda         numacc + 1
            sta         dhi + 1,x
            lda         numacc + 2
            sta         dlo,x
            lda         numacc + 3
            sta         dhi,x
            rts

            HEADER      "um/mod", 0
ummod:                                                      ; ( ud u -- urem uquot )
            jsr         divisor
get_acc:                                                    ; numacc = the double on top, then
            lda         dlo + 1,x
            sta         numacc
            lda         dhi + 1,x
            sta         numacc + 1
            lda         dlo,x
            sta         numacc + 2
            lda         dhi,x
            sta         numacc + 3
            jsr         udiv16
put_qr:                                                     ; The top two: the remainder, the quotient on top
            lda         numacc + 2
            sta         dlo + 1,x
            lda         numacc + 3
            sta         dhi + 1,x
            lda         numacc
            sta         dlo,x
            lda         numacc + 1
            sta         dhi,x
            rts

            HEADER      "m*", 0
mstar:                                                      ; ( n1 n2 -- d )
            lda         dhi,x                               ; (The product's sign)
            eor         dhi + 1,x
            pha
            jsr         abs
            jsr         swap
            jsr         abs
            jsr         umstar
            pla
            bpl         :+
            jmp         dnegate
:
            rts

            HEADER      "*", 0
star:
            jsr         umstar
            inx
            rts

            HEADER      "s>d", 0
stod:
            dex
            lda         dhi + 1,x
            bmi         :+
            jmp         zero_tos
:
            jmp         true_tos

dnegate:                                                    ; ( d -- -d ) (DNEGATE's: the Double-Number library's header)
            sec
            lda         #0
            sbc         dlo + 1,x
            sta         dlo + 1,x
            lda         #0
            sbc         dhi + 1,x
            sta         dhi + 1,x
            lda         #0
            sbc         dlo,x
            sta         dlo,x
            lda         #0
            sbc         dhi,x
            sta         dhi,x
            rts

dabs:                                                       ; ( d -- |d| ) (DABS's: the Double-Number library's header)
            lda         dhi,x
            bmi         dnegate
            rts

            HEADER      "sm/rem", 0
smrem:                                                      ; ( d n -- rem quot ): symmetric
            lda         dhi + 1,x                           ; (The remainder's sign: the dividend's ...
            pha
            eor         dhi,x                               ;   the quotient's: theirs together)
            pha
            jsr         abs
            jsr         save_top                            ; (|n| aside, |d|, then |n| back)
            jsr         dabs
            jsr         push_tmp3
            jsr         ummod
            pla
            bpl         :+
            jsr         negate
:
            pla
            bpl         :+
            jsr         swap
            jsr         negate
            jsr         swap
:
            rts

            HEADER      "fm/mod", 0
fmmod:                                                      ; ( d n -- rem quot ): floored
            lda         dhi,x                               ; (The divisor, kept)
            pha
            lda         dlo,x
            pha
            jsr         smrem
            pla
            sta         tmp3
            pla
            sta         tmp3 + 1
            lda         dlo + 1,x                           ; A remainder of the divisor's other sign: one less,
            ora         dhi + 1,x                           ;   and the divisor added to it
            beq         @done
            lda         dhi + 1,x
            eor         tmp3 + 1
            bpl         @done
            jsr         oneminus
            clc
            lda         dlo + 1,x
            adc         tmp3
            sta         dlo + 1,x
            lda         dhi + 1,x
            adc         tmp3 + 1
            sta         dhi + 1,x
@done:
            rts

            HEADER      "/mod", 0
slashmod:                                                   ; ( n1 n2 -- rem quot )
            jsr         save_top
            jsr         stod
            jsr         push_tmp3
            jmp         smrem

            HEADER      "/", 0
slash:
            jsr         slashmod
            jmp         nip

            HEADER      "mod", 0
mod:
            jsr         slashmod
            inx
            rts

            HEADER      "*/mod", 0
starslashmod:                                               ; ( n1 n2 n3 -- rem quot ): n1 * n2, 32 bits, / n3
            jsr         save_top
            jsr         mstar
            jsr         push_tmp3
            jmp         smrem

            HEADER      "*/", 0
starslash:
            jsr         starslashmod
            jmp         nip

; The top into tmp3, dropped; and back
save_top:
            lda         dlo,x
            sta         tmp3
            lda         dhi,x
            sta         tmp3 + 1
            inx
            rts

push_tmp3:
            dex
            lda         tmp3
            sta         dlo,x
            lda         tmp3 + 1
            sta         dhi,x
            rts

; ud (the double on top) / BASE: the quotient in its place, the remainder (a digit) in .A.  Modifies tmp, numacc
ud_base:
            lda         base
            sta         tmp
            lda         base + 1
            sta         tmp + 1
            stz         numacc + 2                          ; Its high cell first: 0:hi / base ...
            stz         numacc + 3
            lda         dlo,x
            sta         numacc
            lda         dhi,x
            sta         numacc + 1
            jsr         udiv16
            lda         numacc
            sta         dlo,x
            lda         numacc + 1
            sta         dhi,x
            lda         dlo + 1,x                           ;   then rem:lo / base
            sta         numacc
            lda         dhi + 1,x
            sta         numacc + 1
            jsr         udiv16
            lda         numacc
            sta         dlo + 1,x
            lda         numacc + 1
            sta         dhi + 1,x
            lda         numacc + 2
            rts

; ud (the double at stack index .Y: its high cell, its low under it) = ud * BASE + .A.  Keeps .X.  Modifies tmp,
; tmp2, tmp3, numacc, numtmp, cnt.  A BASE under 256 (any the standard has): ud shifted and added for each of its bits, from the
; digit (numacc: the sum; numtmp: ud, doubled each bit), num_ovf 1 if it passes 32 bits; else two 16 x 16 multiplies
ud_mul_add:
            sty         cnt
            ldy         base + 1
            beq         :+
            jmp         ud_mul_add16
:
            sty         numacc + 1
            sty         numacc + 2
            sty         numacc + 3
            sta         numacc
            ldy         cnt
            lda         dlo + 1,y
            sta         numtmp
            lda         dhi + 1,y
            sta         numtmp + 1
            lda         dlo,y
            sta         numtmp + 2
            lda         dhi,y
            sta         numtmp + 3
            lda         base
            sta         tmp
@bit:
            lsr         tmp
            bcc         @next
            clc
            lda         numacc
            adc         numtmp
            sta         numacc
            lda         numacc + 1
            adc         numtmp + 1
            sta         numacc + 1
            lda         numacc + 2
            adc         numtmp + 2
            sta         numacc + 2
            lda         numacc + 3
            adc         numtmp + 3
            sta         numacc + 3
            bcc         @next
@ovf:
            lda         #1                                  ; (Past 32 bits: noted, for number)
            sta         num_ovf
@next:
            lda         tmp
            beq         @done
            asl         numtmp
            rol         numtmp + 1
            rol         numtmp + 2
            rol         numtmp + 3
            bcc         @bit
            bra         @ovf                                ; (A bit out of ud, and more of BASE's to come)
@done:
            lda         numacc
            sta         dlo + 1,y
            lda         numacc + 1
            sta         dhi + 1,y
            lda         numacc + 2
            sta         dlo,y
            lda         numacc + 3
            sta         dhi,y
            rts
ud_mul_add16:
            sta         tmp3
            stz         tmp3 + 1
            lda         base                                ; The high cell * base (its low 16 bits)
            sta         tmp
            lda         base + 1
            sta         tmp + 1
            lda         dlo,y
            sta         tmp2
            lda         dhi,y
            sta         tmp2 + 1
            jsr         umul16
            lda         numacc
            pha
            lda         numacc + 1
            pha
            lda         base                                ; The low cell * base
            sta         tmp
            lda         base + 1
            sta         tmp + 1
            ldy         cnt
            lda         dlo + 1,y
            sta         tmp2
            lda         dhi + 1,y
            sta         tmp2 + 1
            jsr         umul16
            ldy         cnt
            clc                                             ; Low: its low part + the digit
            lda         numacc
            adc         tmp3
            sta         dlo + 1,y
            lda         numacc + 1
            adc         #0
            sta         dhi + 1,y
            pla                                             ; High: its high part + the high cell's, + carry
            sta         tmp + 1
            pla
            adc         numacc + 2
            sta         dlo,y
            lda         tmp + 1
            adc         numacc + 3
            sta         dhi,y
            rts
