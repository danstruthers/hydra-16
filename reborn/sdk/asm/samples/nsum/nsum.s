; ****************************************************************************
; nsum [number ...] - a sample (the assembly SDK's: sdk/asm/README.md) of the number libraries, through numbers.inc's
; macros and numlib.s: the sum of the numbers given, each read in the base (decimal: 1/3, 0.5, 2i, #xFF ...), written
; exactly; then its square root (the math library's, 12 digits) when it has one.
;   % nsum 1/3 0.5 2
;   17/6
;   sqrt 1.68325082306
; A word that isn't a number ends it with code 1 and the message "not a number" (rc's $status).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "numbers.inc"                                      ; (The libraries' entries and NUMCALL, MATHCALL)

            HYX2_PROGRAM "nsum", main

TEXT_ROOM       = 2048                                      ; A number's text at most here (more: "too long")

.zeropage
arg:        .res        2                                   ; The argument (main's list)

.bss
sum:        .res        NUM_MAX                             ; The sum (a number in the stored format) ...
num:        .res        NUM_MAX                             ;   a number read, and the square root
text:       .res        TEXT_ROOM                           ; A number written

.code
main:
            MOVR        arg, r0                             ; Its arguments
            jsr         num_open                            ; The libraries (C = 1: .A the error)
            bcc         :+
            LDR         r0, s_nolib
            lda         #1
            jmp         EXITS
:
            stz         sum                                 ; The sum 0 (the integer 0: its tag alone)
@each:
            lda         (arg)                               ; Each argument (an empty one after the last)
            bne         :+
            jmp         @done
:
            MOVR        r0, arg                             ; Its text and length (passed over)
            ldy         #0
:
            iny
            lda         (arg),y
            bne         :-
            sty         r1
            stz         r1 + 1
            tya
            sec
            adc         arg
            sta         arg
            bcc         :+
            inc         arg + 1
:
            stz         r4                                  ; Read in the base (r4 = 0), the whole word a number
            stz         r4 + 1
            LDR         r2, num
            LDR         r3, NUM_MAX
            ldy         #NPARSE_WHOLE
            NUMCALL     NUM_PARSE
            bcc         :+
            LDR         r0, s_notnum
            lda         #1
            jmp         EXITS
:
            LDR         r0, sum                             ; sum = sum + num (a result may be written over an
            LDR         r1, num                             ;   operand: they're read first)
            LDR         r2, sum
            NUMCALL     NUM_ADD
            bcs         :+
            jmp         @each
:
            LDR         r0, s_big
            lda         #1
            jmp         EXITS

@done:
            LDR         r0, sum                             ; The sum
            jsr         show
            LDR         r0, sum                             ; Its square root, if it has one (a complex number
            LDR         r2, num                             ;   hasn't: NE_REAL; no math library: none)
            LDR         r3, NUM_MAX
            lda         math_mod
            beq         @end
            MATHCALL    MATH_SQRT
            bcs         @end
            PRINT       "sqrt "
            LDR         r0, num
            jsr         show
@end:
            lda         #0
            rts

; The number at r0 written in the base, and a new line
show:
            stz         r4
            stz         r4 + 1
            LDR         r2, text
            LDR         r3, TEXT_ROOM - 1
            NUMCALL     NUM_DISPLAY
            bcc         :+
            LDR         r0, s_long
            lda         #1
            jmp         EXITS
:
            clc                                             ; (A 0 after its text)
            adc         #<text
            sta         r5
            txa
            adc         #>text
            sta         r5 + 1
            lda         #0
            sta         (r5)
            LDR         r0, text
            jsr         PUTS
            lda         #LF
            jmp         PUTC

.rodata
s_nolib:    .byte       "no numbers library", 0
s_notnum:   .byte       "not a number", 0
s_big:      .byte       "too big", 0
s_long:     .byte       "too long", 0

.include "numlib.s"
