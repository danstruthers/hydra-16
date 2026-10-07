; ****************************************************************************
; t_bank2 - a module of two banks (sdk/asm/hyx2.inc: FAR2, FAR1), run as init: a routine in its second bank called
; from its first, with its registers and C both ways, reading its own bank's data; a call back into the first bank
; from there; each bank's data where it should be afterwards; and the module directory's entry (two banks).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_bank2", main, 2

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
            FAR2        b2_sum                              ; (.A + .X + bank 2's $40, doubled in bank 1)
            sty         got_y
            bcs         :+
            EXPECT_A    (5 + 7 + $40) * 2, "a routine in bank 2: .A and .X in, .A out, through a routine in bank 1"
            bra         :++
:
            NOTOK       "a routine in bank 2: C = 0 back"
:
            lda         got_y
            EXPECT_A    $A5, ".Y in and out (it flips it)"
            FAR2        b2_fail
            bcc         :+
            EXPECT_A    E_RANGE, "C = 1 and .A back from bank 2"
            bra         :++
:
            NOTOK       "C = 1 back from bank 2"
:
            lda         s_one                               ; Bank 1's data, in place again
            EXPECT_A    'o', "bank 1's data at $A000 again after the calls"
            FAR2        b2_peek
            EXPECT_A    'b', "bank 2's data, read in bank 2"
            ldx         #0                                  ; Its directory entry: two banks
@entry:
            phx
            LDR         r0, me
            txa
            jsr         MODINFO
            plx
            bcs         @none
            inx
            lda         me + ME_NAME + 2
            cmp         #'b'
            bne         @entry
            lda         me + ME_BANKS
            EXPECT_A    2, "the module directory: two banks"
            bra         @done
@none:
            NOTOK       "its module directory entry"
@done:
            DONE        "t_bank2"

; Bank 1's routine for bank 2: .A doubled
b1_twice:
            asl
            rts

.rodata
s_one:      .byte       "one", 0

.segment "CODE2"
; .A + .X + bank 2's constant, doubled in bank 1; .Y flipped.  OUT: C = 0
b2_sum:
            stx         k
            clc
            adc         k
            adc         b2_k
            FAR1        b1_twice
            pha
            tya
            eor         #$FF
            tay
            pla
            clc
            rts

b2_fail:
            lda         #E_RANGE
            sec
            rts

b2_peek:
            lda         s_two
            rts

.segment "RODATA2"
b2_k:       .byte       $40
s_two:      .byte       "bank 2", 0
