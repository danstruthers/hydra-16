; ****************************************************************************
; pwd - the current directory, and a new line, on fd 1

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "pwd", main

.bss
buf:        .res        PATH_MAX + 2

.code
main:
            LDR         r0, buf
            jsr         GETCWD
            tax
            lda         #LF
            sta         buf,X
            inx
            stx         r1
            stz         r1 + 1
            LDR         r0, buf
            lda         #1
            jsr         WRITE
            lda         #0
            rts
