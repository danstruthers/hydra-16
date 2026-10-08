; ****************************************************************************
; double.s - HyForth's Double-Number word set (/lib/forth/double.fl: lib double), and its extension's 2ROT, 2VALUE
; and DU<.  A double is two cells, its high cell on top.  DNEGATE and DABS are the core's (. and the rest use them:
; their headers here jump to it), as is 2VALUE's runtime (do2value: Core Extension's TO stores into a 2VALUE by it).
; M*/ divides symmetrically, as the core's / does.

.include "forthlib.inc"

.bss
ms_div:     .res        2                                   ; M*/'s divisor ...
ms_mul:     .res        2                                   ;   |n1| ...
ms_sign:    .res        1                                   ;   the product's sign (bit 7) ...
ms_t:       .res        6                                   ;   and the product, three cells (the low first)
.code

            HEADER      "2constant", 0
twoconstant:                                                ; ( x1 x2 "name" -- ): two literals and rts
            lda         #0
            jsr         make_hdr
            jsr         twoliteral
            lda         #RTS_OP
            jmp         ccomma_a

            HEADER      "2variable", 0
twovariable:
            jsr         variable
            lda         #0
            tay
            jmp         comma_ay

            HEADER      "2value", 0
twovalue:                                                   ; ( x1 x2 "name" -- ): its cells as 2! has them
            lda         #<do2value
            ldy         #>do2value
            jsr         make_word
            jsr         comma
            jmp         comma

            HEADERC     "2literal", F_IMMEDIATE
twoliteral:                                                 ; ( x1 x2 -- ): compiled, x1 pushed first
            jsr         swap
            jsr         literal
            jmp         literal

            HEADER      "dnegate", 0
dnegate_w:                                                  ; ( d -- -d )
            jmp         dnegate

            HEADER      "dabs", 0
dabs_w:                                                     ; ( d -- |d| )
            jmp         dabs

            HEADER      "d+", 0
dplus:                                                      ; ( d1 d2 -- d3 )
            clc
            lda         dlo + 3,x
            adc         dlo + 1,x
            sta         dlo + 3,x
            lda         dhi + 3,x
            adc         dhi + 1,x
            sta         dhi + 3,x
            lda         dlo + 2,x
            adc         dlo,x
            sta         dlo + 2,x
            lda         dhi + 2,x
            adc         dhi,x
            sta         dhi + 2,x
            inx
            inx
            rts

            HEADER      "d-", 0
dminus:                                                     ; ( d1 d2 -- d3 )
            sec
            lda         dlo + 3,x
            sbc         dlo + 1,x
            sta         dlo + 3,x
            lda         dhi + 3,x
            sbc         dhi + 1,x
            sta         dhi + 3,x
            lda         dlo + 2,x
            sbc         dlo,x
            sta         dlo + 2,x
            lda         dhi + 2,x
            sbc         dhi,x
            sta         dhi + 2,x
            inx
            inx
            rts

            HEADER      "m+", 0
mplus:                                                      ; ( d1 n -- d2 )
            lda         dhi,x                               ; (n a double: its sign, the high cell)
            dex
            asl
            lda         #0
            bcc         :+
            lda         #$FF
:
            sta         dlo,x
            sta         dhi,x
            jmp         dplus

            HEADER      "d0=", 0
dzeroequal:                                                 ; ( d -- flag )
            lda         dlo,x
            ora         dhi,x
            ora         dlo + 1,x
            ora         dhi + 1,x
            inx
            cmp         #0
            bne         :+
            jmp         true_tos
:
            jmp         zero_tos

            HEADER      "d0<", 0
dzeroless:                                                  ; ( d -- flag )
            lda         dhi,x
            inx
            asl
            bcc         :+
            jmp         true_tos
:
            jmp         zero_tos

            HEADER      "d2*", 0
dtwostar:                                                   ; ( d1 -- d2 )
            asl         dlo + 1,x
            rol         dhi + 1,x
            rol         dlo,x
            rol         dhi,x
            rts

            HEADER      "d2/", 0
dtwoslash:                                                  ; ( d1 -- d2 ): its sign kept
            lda         dhi,x
            cmp         #$80
            ror         dhi,x
            ror         dlo,x
            ror         dhi + 1,x
            ror         dlo + 1,x
            rts

            HEADER      "d=", 0
dequal:                                                     ; ( d1 d2 -- flag )
            ldy         #0
            lda         dlo,x
            cmp         dlo + 2,x
            bne         :+
            lda         dhi,x
            cmp         dhi + 2,x
            bne         :+
            lda         dlo + 1,x
            cmp         dlo + 3,x
            bne         :+
            lda         dhi + 1,x
            cmp         dhi + 3,x
            bne         :+
            dey
:
            inx
            inx
            inx
            tya
            sta         dlo,x
            sta         dhi,x
            rts

            HEADER      "d<", 0
dless:                                                      ; ( d1 d2 -- flag )
            jsr         d_lt
            bra         flag_3

            HEADER      "du<", 0
duless:                                                     ; ( ud1 ud2 -- flag )
            jsr         d_sub
            lda         #0
            bcs         :+
            lda         #$80
:
; The top three cells dropped and the fourth a flag: true if .A's bit 7 is
flag_3:
            inx
            inx
            inx
            asl
            bcc         :+
            jmp         true_tos
:
            jmp         zero_tos

            HEADER      "dmax", 0
dmax:                                                       ; ( d1 d2 -- d3 )
            jsr         d_lt
            asl
            bcs         d_keep2
            bra         d_keep1

            HEADER      "dmin", 0
dmin:                                                       ; ( d1 d2 -- d3 )
            jsr         d_lt
            asl
            bcc         d_keep2
; ( d1 d2 -- d1 )
d_keep1:
            inx
            inx
            rts
; ( d1 d2 -- d2 )
d_keep2:
            lda         dlo,x
            sta         dlo + 2,x
            lda         dhi,x
            sta         dhi + 2,x
            lda         dlo + 1,x
            sta         dlo + 3,x
            lda         dhi + 1,x
            sta         dhi + 3,x
            inx
            inx
            rts

; d1 - d2 (the top two doubles, kept), its high byte in .A, C its borrow (0: d1 < d2 unsigned)
d_sub:
            sec
            lda         dlo + 3,x
            sbc         dlo + 1,x
            lda         dhi + 3,x
            sbc         dhi + 1,x
            lda         dlo + 2,x
            sbc         dlo,x
            lda         dhi + 2,x
            sbc         dhi,x
            rts

; .A's bit 7: d1 < d2, signed (the top two doubles, kept)
d_lt:
            jsr         d_sub
            bvc         :+
            eor         #$80
:
            rts

            HEADER      "d>s", 0
dtos:                                                       ; ( d -- n )
            inx
            rts

            HEADER      "d.", 0
ddot:                                                       ; ( d -- )
            jsr         d_text
            jsr         type
            jmp         space

            HEADER      "d.r", 0
ddotr:                                                      ; ( d n -- ): right-aligned in n characters
            lda         dhi,x
            pha
            lda         dlo,x
            pha
            inx
            jsr         d_text
            dex
            pla
            sta         dlo,x
            pla
            sta         dhi,x
            jsr         over
            jsr         minus
            jsr         spaces
            jmp         type

; ( d -- addr u ): d as text, signed (in the numbers library's base, if it isn't a radix)
d_text:
            lda         #4
            jsr         num_textc
            bcs         :+
            rts
:
            lda         dhi,x                               ; (Its sign, kept)
            pha
            jsr         dabs
            jsr         lessnum
            jsr         nums
            pla
            bpl         :+
            lda         #'-'
            jsr         hold_a
:
            jmp         numgreater

            HEADER      "m*/", 0
mstarslash:                                                 ; ( d1 n1 +n2 -- d2 ): d1 * n1 (three cells) / n2
            lda         dlo,x                               ; (n2, |n1|, the sign)
            sta         ms_div
            lda         dhi,x
            sta         ms_div + 1
            lda         dhi + 1,x
            eor         dhi + 2,x
            sta         ms_sign
            inx
            jsr         abs
            lda         dlo,x
            sta         ms_mul
            lda         dhi,x
            sta         ms_mul + 1
            inx
            jsr         dabs                                ; |d1|: its high cell * |n1| ...
            jsr         ms_push_mul
            jsr         umstar
            lda         dlo + 1,x
            sta         ms_t + 2
            lda         dhi + 1,x
            sta         ms_t + 3
            lda         dlo,x
            sta         ms_t + 4
            lda         dhi,x
            sta         ms_t + 5
            inx
            inx
            jsr         ms_push_mul                         ;   and its low cell's, added (the product: ms_t)
            jsr         umstar
            lda         dlo + 1,x
            sta         ms_t
            lda         dhi + 1,x
            sta         ms_t + 1
            clc
            lda         dlo,x
            adc         ms_t + 2
            sta         ms_t + 2
            lda         dhi,x
            adc         ms_t + 3
            sta         ms_t + 3
            bcc         :+
            inc         ms_t + 4
            bne         :+
            inc         ms_t + 5
:
            inx                                             ; ( ) / n2, a cell at a time from the high one: the
            inx                                             ;   remainder before each
            ldy         #4
            jsr         ms_push_t
            dex
            jsr         zero_tos
            jsr         ms_div_step                         ; ( rem q2 ): q2 dropped (0, for a d2 that fits)
            inx
            ldy         #2
            jsr         ms_div_cell                         ; ( q1 rem )
            ldy         #0
            jsr         ms_div_cell                         ; ( q1 q0 rem )
            inx
            jsr         swap                                ; ( q0 q1 )
            bit         ms_sign
            bpl         :+
            jmp         dnegate
:
            rts

; ( rem -- q rem2 ): (ms_t's cell at .Y, rem above it) / n2
ms_div_cell:
            jsr         ms_push_t
            jsr         swap
            jsr         ms_div_step
            jmp         swap

; ( ud -- urem uquot ): / n2
ms_div_step:
            dex
            lda         ms_div
            sta         dlo,x
            lda         ms_div + 1
            sta         dhi,x
            jmp         ummod

; ( -- x ): ms_t's cell at .Y
ms_push_t:
            dex
            lda         ms_t,y
            sta         dlo,x
            lda         ms_t + 1,y
            sta         dhi,x
            rts

; ( u -- u |n1| )
ms_push_mul:
            dex
            lda         ms_mul
            sta         dlo,x
            lda         ms_mul + 1
            sta         dhi,x
            rts

            HEADER      "2rot", 0
tworot:                                                     ; ( d1 d2 d3 -- d2 d3 d1 )
            lda         dlo + 5,x                           ; (d1 kept)
            pha
            lda         dhi + 5,x
            pha
            lda         dlo + 4,x
            pha
            lda         dhi + 4,x
            pha
            ldy         #4                                  ; d2 and d3 up two cells, the deepest first
:
            lda         dlo + 3,x
            sta         dlo + 5,x
            lda         dhi + 3,x
            sta         dhi + 5,x
            dex
            dey
            bne         :-
            inx
            inx
            inx
            inx
            pla                                             ; d1 on top
            sta         dhi,x
            pla
            sta         dlo,x
            pla
            sta         dhi + 1,x
            pla
            sta         dlo + 1,x
            rts
