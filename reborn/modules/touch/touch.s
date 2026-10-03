; ****************************************************************************
; touch name ... - each file made, empty, if it isn't there (CREATE), or its time changed if it is (WSTAT: a
; record that changes nothing else, its name empty and the rest $FF, as Plan 9's).  One that can't be is said
; ("touch: name: why"), and touch ends with code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "touch", main

.bss
st:         .res        SR_SIZE

.code
main:
            jsr         tl_start
@arg:
            lda         (tl_arg)
            beq         @end
            MOVR        r0, tl_arg
            LDR         r1, st
            jsr         STAT
            bcc         @there
            cmp         #E_NOENT
            bne         @failed
            MOVR        r0, tl_arg                          ; Not there: made
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            bcs         @failed
            jsr         CLOSE
            bra         @next

@there:
            ldx         #SR_SIZE - 1                        ; There: its time (nothing else)
            lda         #$FF
:
            sta         st,X
            dex
            bpl         :-
            stz         st + SR_NAME
            MOVR        r0, tl_arg
            LDR         r1, st
            jsr         WSTAT
            bcc         @next
@failed:
            pha
            MOVR        r0, tl_arg
            pla
            jsr         tl_err
@next:
            jsr         tl_next
            bra         @arg

@end:
            jmp         tl_end

.rodata
tl_name:    .byte       "touch", 0
tl_flagset: .byte       0
tl_usage:   .byte       "touch name ...", 0

.include "toollib.s"
