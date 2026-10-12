; show file - a file's text, on the screen
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "show", main

.zeropage
fd:         .res 1              ; The file, open

.bss
buf:        .res 256            ; A piece of it
msg:        .res 32             ; An error's text

.code
main:       lda (r0)            ; The file's name: r0's first argument
            bne @open
            PRINT "usage: show file"
            lda #LF
            jsr PUTC
            rts
@open:      lda #O_READ         ; Open it, to read
            jsr OPEN
            bcs @fail           ; (C = 1: it failed, .A the error)
            sta fd
@read:      LDR r0, buf         ; 256 bytes of it at most
            LDR r1, 256
            lda fd
            jsr READ
            bcs @fail
            sta r1              ; The count read: .A (low), .X (high)
            stx r1 + 1
            ora r1 + 1
            beq @done           ; (0: the file's end)
            LDR r0, buf         ; Those bytes, to fd 1: the screen
            lda #1
            jsr WRITE
            bcs @fail
            bra @read
@done:      lda fd
            jsr CLOSE
            rts
@fail:      pha                 ; The error's text (LDR changes .A: kept)
            LDR r0, msg
            pla
            jsr ERRSTR
            PRINT "show: "
            PRINT msg
            lda #LF
            jsr PUTC
            lda #1              ; The end, with code 1
            stz r0
            stz r0 + 1
            jmp EXITS
