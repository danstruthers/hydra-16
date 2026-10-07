; ****************************************************************************
; more [file ...] - each file (none: fd 0's) a screen at a time: 22 lines, then "--more--" and a line read from
; the console (/dev/cons: fd 0 may be the file): Enter for the next screen, q to stop.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "more", main

PAGE            = 22            ; Lines a screen

.bss
lines:      .res        1                                   ; The lines shown on this screen
cons:       .res        1                                   ; The console's fd ($FF: none)
key:        .res        64                                  ; What was typed

.code
main:
            jsr         tl_start
            LDR         r0, s_cons
            lda         #O_RDWR
            jsr         OPEN
            bcc         :+
            lda         #$FF                                ; (No console: all of it at once)
:
            sta         cons
            stz         lines
            LDR         tl_ivec, file
            jsr         tl_eachin
            lda         cons
            bmi         :+
            jsr         CLOSE
:
            jmp         tl_end

; The input, a screen at a time
file:
@byte:
            jsr         tl_getc
            bcs         @done
            jsr         tl_putc
            cmp         #LF
            bne         @byte
            inc         lines
            lda         lines
            cmp         #PAGE
            bcc         @byte
            stz         lines
            lda         cons
            bmi         @byte
            jsr         ask
            bcc         @byte
            jmp         tl_end                              ; (q: no more)

@done:
            rts

; "--more--", and a line from the console.  OUT: C = 1 if it starts with q
ask:
            jsr         tl_flush
            LDR         r0, s_more
            LDR         r1, 8
            lda         cons
            jsr         WRITE
            LDR         r0, key
            LDR         r1, 64
            lda         cons
            jsr         READ
            bcs         @quit                               ; (A note, or its end: no more)
            cmp         #0
            beq         @quit
            lda         key
            cmp         #'q'
            beq         @quit
            clc
            rts

@quit:
            sec
            rts

.rodata
s_cons:     .byte       "/dev/cons", 0
s_more:     .byte       "--more--"
tl_name:    .byte       "more", 0
tl_flagset: .byte       0
tl_usage:   .byte       "more [file ...]", 0

.include "toollib.s"
