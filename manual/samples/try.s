; try - a few instructions, to watch in db
.include "hydra.inc"
.include "hyx2.inc"

            HYX2_PROGRAM "try", main

.code
main:       lda #$7F            ; A = $7F (127)
            clc                 ; C = 0
            adc #1              ; A = A + 1 + C
            ldx #$FF            ; X = $FF
            inx                 ; X = X + 1
            rts                 ; The end
