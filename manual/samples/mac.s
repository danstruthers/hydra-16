; mac - a macro of its own
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "mac", main

; SAY char, times: the character, times times
.macro SAY char, times
            ldx #times
:           lda #char
            phx                 ; (PUTC may change .X: kept on the stack)
            jsr PUTC
            plx
            dex
            bne :-
.endmacro

.code
main:       SAY 'H', 1
            SAY 'o', 5
            SAY '!', 3
            SAY LF, 1
            rts
