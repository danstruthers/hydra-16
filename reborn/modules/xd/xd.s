; ****************************************************************************
; xd [file ...] - each file's bytes (none: fd 0's) in hex, 16 a line after their offset, then as text (. for what
; isn't printable): "0000010  68 65 6c 6c 6f 0a                                 hello."

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "xd", main

.bss
offset:     .res        4                                   ; The line's offset
row:        .res        16                                  ; Its bytes ...
n:          .res        1                                   ;   how many

.code
main:
            jsr         tl_start
            LDR         tl_ivec, file
            jsr         tl_eachin
            jmp         tl_end

; The input, a line each 16 bytes
file:
            stz         offset
            stz         offset + 1
            stz         offset + 2
            stz         offset + 3
@line:
            ldx         #0
:
            jsr         tl_getc
            bcs         :+
            sta         row,X
            inx
            cpx         #16
            bne         :-
:
            stx         n
            cpx         #0
            beq         @done
            lda         offset + 3                          ; Its offset: 7 hex digits
            and         #$0F
            jsr         digit
            lda         offset + 2
            jsr         hex
            lda         offset + 1
            jsr         hex
            lda         offset
            jsr         hex
            jsr         tl_space
            ldx         #0                                  ; Its bytes in hex ...
@hex:
            jsr         tl_space
            cpx         n
            bcs         :+
            lda         row,X
            jsr         hex
            bra         @next

:
            jsr         tl_space                            ; (Past the last: room)
            jsr         tl_space
@next:
            inx
            cpx         #16
            bne         @hex
            jsr         tl_space                            ; ... and as text
            jsr         tl_space
            ldx         #0
:
            lda         row,X
            cmp         #' '
            bcc         @dot
            cmp         #$7F
            bcc         :+
@dot:
            lda         #'.'
:
            jsr         tl_putc
            inx
            cpx         n
            bne         :--
            jsr         tl_nl
            clc                                             ; The next line's offset
            lda         offset
            adc         n
            sta         offset
            bcc         :+
            inc         offset + 1
            bne         :+
            inc         offset + 2
            bne         :+
            inc         offset + 3
:
            lda         n
            cmp         #16
            bne         @done
            jmp         @line

@done:
            rts

; .A in hex, two digits
hex:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         digit
            pla
            and         #$0F
digit:
            cmp         #10
            bcc         :+
            adc         #'a' - '0' - 10 - 1
:
            adc         #'0'
            jmp         tl_putc

.rodata
tl_name:    .byte       "xd", 0
tl_flagset: .byte       0
tl_usage:   .byte       "xd [file ...]", 0

.include "toollib.s"
