; ****************************************************************************
; t_bank3 - a module of three banks (sdk/asm/hyx2.inc: FARN), run as init: a routine in its third bank called from
; its first, with its registers and C both ways, reading its own bank's data; from there one in its second bank, and
; from that one back in the first; each bank set again as each call returns; and the module directory's entry
; (three banks).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_bank3", main, 3

.bss
me:         .res        ME_SIZE
got_y:      .res        1
k:          .res        1

.code
main:
            HYX2_BANKS_INIT
            stz         T_FAILS
            lda         #5
            ldx         #7
            ldy         #$5A
            clc
            FARN        3, b3_sum                           ; (.A + .X + bank 3's $40, + bank 2's $10, doubled
            sty         got_y                               ; in bank 1)
            bcs         :+
            EXPECT_A    (5 + 7 + $40 + $10) * 2, "bank 3, then bank 2, then bank 1: .A and .X in, .A out"
            bra         :++
:
            NOTOK       "a routine in bank 3: C = 0 back"
:
            lda         got_y
            EXPECT_A    $A5, ".Y in and out (bank 3 flips it)"
            FARN        3, b3_fail
            bcc         :+
            EXPECT_A    E_RANGE, "C = 1 and .A back from bank 3"
            bra         :++
:
            NOTOK       "C = 1 back from bank 3"
:
            lda         s_one                               ; Bank 1's data, in place again
            EXPECT_A    'o', "bank 1's data at $A000 again after the calls"
            FARN        3, b3_peek
            EXPECT_A    't', "bank 3's data, read in bank 3"
            FARN        2, b2_peek
            EXPECT_A    'b', "bank 2's data, read in bank 2"
            ldx         #0                                  ; Its directory entry: three banks
@entry:
            phx
            LDR         r0, me
            txa
            jsr         MODINFO
            plx
            bcs         @none
            inx
            lda         me + ME_NAME + 6
            cmp         #'3'
            bne         @entry
            lda         me + ME_BANKS
            EXPECT_A    3, "the module directory: three banks"
            bra         @done
@none:
            NOTOK       "its module directory entry"
@done:
            DONE        "t_bank3"

; Bank 1's routine for bank 2: .A doubled
b1_twice:
            asl
            rts

.rodata
s_one:      .byte       "one", 0

.segment "CODE2"
; .A + bank 2's constant, doubled in bank 1 (called from bank 3)
b2_add:
            clc
            adc         b2_k
            FARN        1, b1_twice
            rts

b2_peek:
            lda         s_two
            rts

.segment "RODATA2"
b2_k:       .byte       $10
s_two:      .byte       "bank 2", 0

.segment "CODE3"
; .A + .X + bank 3's constant, then bank 2's (b2_add); .Y flipped.  OUT: C = 0
b3_sum:
            stx         k
            clc
            adc         k
            adc         b3_k
            FARN        2, b2_add
            pha
            tya
            eor         #$FF
            tay
            pla
            clc
            rts

b3_fail:
            lda         #E_RANGE
            sec
            rts

b3_peek:
            lda         s_three
            rts

.segment "RODATA3"
b3_k:       .byte       $40
s_three:    .byte       "three", 0
