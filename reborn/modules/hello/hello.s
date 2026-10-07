; ****************************************************************************
; hello - a small program: it greets its arguments and ends with code 7 and the message "bye".  Its data and BSS
; show the module's start-up works (a wrong one ends with code $FF and says what).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "hello", main

.zeropage
args:       .res        2

.data
count:      .byte       3                                   ; (Copied from the module at its start)

.bss
buf:        .res        16                                  ; (Cleared at its start)

.code
main:
            MOVR        args, r0
            PRINT       s_hello
            MOVR        r0, args
            jsr         PUTS
            PRINT       s_crlf
            lda         count
            cmp         #3
            bne         @nodata
            ldx         #15
:
            lda         buf,X
            bne         @nobss
            dex
            bpl         :-
            LDR         r0, s_bye
            lda         #7
            jmp         EXITS

@nodata:
            LDR         r0, s_nodata
            bra         @bad

@nobss:
            LDR         r0, s_nobss
@bad:
            lda         #$FF
            jmp         EXITS

.rodata
s_hello:    .byte       "hello, ", 0
s_crlf:     .byte       CR, LF, 0
s_bye:      .byte       "bye", 0
s_nodata:   .byte       "no data", 0
s_nobss:    .byte       "BSS not cleared", 0
