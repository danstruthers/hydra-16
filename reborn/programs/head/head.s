; ****************************************************************************
; head [-N] [file ...] - each file's first N lines (10; none: fd 0's).  A file that can't be read is said ("head:
; name: why"), and head ends with code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "head", main

.bss
lines:      .res        2                                   ; The lines to show ...
left:       .res        2                                   ;   and those still to show of a file

.code
main:
            LDR         lines, 10
            MOVR        tl_arg, r0                          ; -N first?
            lda         (tl_arg)
            cmp         #'-'
            bne         @start
            ldy         #1
            lda         (tl_arg),Y
            beq         @start
            sec
            sbc         #'0'
            cmp         #10
            bcs         @start
            clc                                             ; (Its number: past the -)
            lda         tl_arg
            adc         #1
            sta         r0
            lda         tl_arg + 1
            adc         #0
            sta         r0 + 1
            jsr         tl_atoi
            bcs         :+
            lda         tl_num + 2
            ora         tl_num + 3
            bne         :+
            MOVR        lines, tl_num
            jsr         tl_next
            MOVR        r0, tl_arg
            bra         @start

:
            jmp         tl_badusage

@start:
            jsr         tl_start
            LDR         tl_ivec, file
            jsr         tl_eachin
            jmp         tl_end

; The input's first lines
file:
            MOVR        left, lines
@line:
            lda         left
            ora         left + 1
            beq         @done
@byte:
            jsr         tl_getc
            bcs         @done
            jsr         tl_putc
            cmp         #LF
            bne         @byte
            lda         left
            bne         :+
            dec         left + 1
:
            dec         left
            bra         @line

@done:
            rts

.rodata
tl_name:    .byte       "head", 0
tl_flagset: .byte       0
tl_usage:   .byte       "head [-N] [file ...]", 0

.include "toollib.s"
