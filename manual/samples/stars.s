; stars - a triangle of stars
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "stars", main

ROWS        = 5                 ; How many rows

.zeropage
row:        .res 1              ; The row it's on: 1 to ROWS
count:      .res 1              ; The stars still to print on it

.code
main:       lda #1              ; The first row
            sta row
@row:       lda row             ; As many stars as the row's number
            jsr line
            inc row             ; The next row, till the last is done
            lda row
            cmp #ROWS + 1
            bne @row
            rts

; A line of stars: .A of them (1 or more)
line:       sta count
@star:      lda #'*'
            jsr PUTC
            dec count
            bne @star
            lda #LF             ; The line's end
            jsr PUTC
            rts
