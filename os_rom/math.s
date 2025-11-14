.debuginfo

.segment "BIOS"

; MATH
MOD_10:
            cmp             #0
            bmi             @negative
            cmp             #100
            bmi             @positive
            sbc             #100

@positive:
            sec

@pos_loop:
            sbc             #10
            beq             @pos_end
            bpl             @pos_loop
            adc             #10

@pos_end:
            rts

@negative:
            cmp             #$A6            ; -90
            bpl             @neg_start
            adc             #$A6

@neg_start:
            clc

@neg_loop:
            adc             #10
            bmi             @neg_loop
            rts

REM_10:
            ldx             #0
            cmp             #0
            bmi             @negative
            cmp             #100
            bmi             @positive
            sbc             #100
            ldx             #10

@positive:
            sec

@pos_loop:
            inx
            sbc             #10
            beq             @pos_end
            bpl             @pos_loop
            dex
            adc             #10

@pos_end:
            rts

@negative:
            cmp             #$9B            ; -99
            bpl             @neg_start
            adc             #$9B
            ldx             #$F6            ; -10

@neg_start:
            clc

@neg_loop:
            dex
            adc             #10
            bmi             @neg_loop
            beq             @neg_end
            inx
            sec
            sbc             #10

@neg_end:
            rts

DIV_10:
            jsr             REM_10
            SWAP_AX
            rts

ABS:
            cmp             #0
            bcs             INV_DONE

NEGATE:
            inc

INVERT:
            eor             #$FF

INV_DONE:
            rts

