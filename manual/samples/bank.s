; bank - a RAM bank of its own
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "bank", main

.zeropage
mine:       .res 1              ; The bank it's given

.code
main:       lda #1              ; One bank, please
            jsr BANKS_ALLOC
            bcs @none           ; (C = 1: none free)
            sta mine
            sta $00             ; It's at $8000-$9FFF now
            ldx #0              ; The message copied into it ...
:           lda message,x
            sta $8000,x
            beq :+              ;   (its 0 too)
            inx
            bra :-
:           LDR r0, $8000       ;   and printed from there
            jsr PUTS
            lda mine            ; The bank given back
            ldx #1
            jsr BANKS_FREE
            rts
@none:      PRINT "no bank free"
            rts

.rodata
message:    .byte "Hello from a RAM bank!", LF, 0
