; ****************************************************************************
; uniq [-c] [file] - the file's lines (none: fd 0's), each run of the same line shown once; -c: each with how many
; there were before it.  A line is 255 bytes at most (the rest is a line of its own).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "uniq", main

F_C             = $01           ; -c

.bss
last:       .res        256                                 ; The line before ...
lastlen:    .res        1                                   ;   its length ...
have:       .res        1                                   ;   <> 0: there's one ...
runs:       .res        2                                   ;   and how many there have been of it
line:       .res        256                                 ; The line just read ...
len:        .res        1                                   ;   its length ...
end:        .res        1                                   ;   <> 0: the input's end

.code
main:
            jsr         tl_start
            jsr         tl_count
            cmp         #2
            bcc         :+
            jmp         tl_badusage

:
            LDR         tl_ivec, file
            jsr         tl_eachin
            jmp         tl_end

; The input's lines
file:
            stz         have
            stz         end
@line:
            ldx         #0                                  ; A line (its new line kept)
@byte:
            jsr         tl_getc
            bcs         @eof
            sta         line,X
            inx
            cmp         #LF
            beq         @got
            cpx         #255
            bne         @byte
            bra         @got

@eof:
            dec         end
            cpx         #0
            beq         @last
@got:
            stx         len
            lda         have                                ; The same as the one before?
            beq         @new
            cpx         lastlen
            bne         @new
            dex
:
            lda         line,X
            cmp         last,X
            bne         @new
            dex
            cpx         #$FF
            bne         :-
            inc         runs
            bne         @on
            inc         runs + 1
            bra         @on

@new:
            jsr         show                                ; Another: the one before shown, this kept
            ldx         len
            stx         lastlen
:
            dex
            cpx         #$FF
            beq         :+
            lda         line,X
            sta         last,X
            bra         :-
:
            lda         #1
            sta         have
            sta         runs
            stz         runs + 1
@on:
            lda         end
            beq         @line
@last:
            jmp         show

; The line kept, shown (-c: its count first)
show:
            lda         have
            beq         @done
            lda         tl_flags
            and         #F_C
            beq         :+
            lda         runs
            sta         tl_num
            lda         runs + 1
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            lda         #7
            jsr         tl_dec
            jsr         tl_space
:
            ldx         #0
:
            cpx         lastlen
            beq         @done
            lda         last,X
            jsr         tl_putc
            inx
            bra         :-

@done:
            rts

.rodata
tl_name:    .byte       "uniq", 0
tl_flagset: .byte       "c", 0
tl_usage:   .byte       "uniq [-c] [file]", 0

.include "toollib.s"
