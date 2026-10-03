; ****************************************************************************
; t_big - a RAM program of 16K (BIG_SIZE bytes of read-only data), for the time it takes to load (t_load's budget):
; its first act is a mark, "big>" (its arguments "r": "rbig>", loaded from the RAM disk), then it ends with a check
; of its data (byte i is i * 7 + i / 256; each step the check rotated left, the byte added) as its code: $E9, the
; image read whole and in order.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_big", main

BIG_SIZE        = 16384

.zeropage
ptr:        .res        2
sum:        .res        1

.code
main:
            lda         (r0)
            cmp         #'r'
            beq         :+
            MARK        "big>"
            bra         @sum
:
            MARK        "rbig>"
@sum:
            LDR         ptr, data                           ; Its data's check
            stz         sum
            ldx         #>BIG_SIZE
            ldy         #0
:
            lda         sum
            asl
            adc         #0                                  ; (Rotated left)
            clc
            adc         (ptr),Y
            sta         sum
            iny
            bne         :-
            inc         ptr + 1
            dex
            bne         :-
            lda         sum
            stz         r0
            stz         r0 + 1
            jmp         EXITS

.rodata
data:
            .repeat     BIG_SIZE, I
            .byte       <(I * 7 + I / 256)
            .endrepeat
