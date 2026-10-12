; bad - a program with mistakes in it, to see what as says
.include "hydra.inc"
.include "hyx2.inc"

            HYX2_PROGRAM "bad", main

.code
main:       lda #300            ; A byte is 0 to 255
            ldx #10
@loop:      jsr PUTC            ; (Spelt right now)
            dex
            bne @loop
            rts
