; ****************************************************************************
; wc [-lwc] [file ...] - each file's lines, words (runs of anything but spaces, tabs and new lines) and bytes (none:
; fd 0's), a line each ("      3       5      24 name"), and their total after several; -l, -w, -c: those counts
; alone.  A file that can't be read is said ("wc: name: why"), and wc ends with code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "wc", main

F_L             = $01           ; -l
F_W             = $02           ; -w
F_C             = $04           ; -c

.bss
counts:     .res        12                                  ; A file's lines, words and bytes (4 bytes each) ...
totals:     .res        12                                  ;   and all of them's
inword:     .res        1                                   ; <> 0: in a word
sep:        .res        1                                   ; <> 0: a count shown on the line
files:      .res        1                                   ; The files counted

.code
main:
            jsr         tl_start
            lda         tl_flags                            ; (None: all three)
            bne         :+
            lda         #F_L | F_W | F_C
            sta         tl_flags
:
            LDR         tl_ivec, count
            stz         files
            jsr         tl_eachin
            lda         files                               ; Several: their total
            cmp         #2
            bcc         @end
            ldx         #11
:
            lda         totals,X
            sta         counts,X
            dex
            bpl         :-
            LDR         tl_iname, s_total
            jsr         say
@end:
            jmp         tl_end

; The input counted, and said
count:
            ldx         #11
:
            stz         counts,X
            dex
            bpl         :-
            stz         inword
@byte:
            jsr         tl_getc
            bcs         @end
            ldx         #8                                  ; A byte
            jsr         one
            cmp         #LF
            bne         :+
            ldx         #0                                  ; A line
            jsr         one
:
            cmp         #' ' + 1                            ; A word: where one starts
            bcc         @space
            ldx         inword
            bne         @byte
            inc         inword
            ldx         #4
            jsr         one
            bra         @byte

@space:
            stz         inword
            bra         @byte

@end:
            inc         files
            ldx         #0                                  ; Into the totals, each from its low byte
@total:
            clc
            ldy         #4
:
            lda         totals,X
            adc         counts,X
            sta         totals,X
            inx
            dey
            bne         :-
            cpx         #12
            bne         @total
; The counts and the input's name, a line
say:
            stz         sep
            ldx         #0
            lda         #F_L
@count:
            pha
            and         tl_flags
            beq         @next
            lda         sep                                 ; (A space between, not before the first)
            beq         :+
            jsr         tl_space
:
            inc         sep
            lda         counts,X
            sta         tl_num
            lda         counts + 1,X
            sta         tl_num + 1
            lda         counts + 2,X
            sta         tl_num + 2
            lda         counts + 3,X
            sta         tl_num + 3
            lda         #7
            jsr         tl_dec
@next:
            pla
            inx
            inx
            inx
            inx
            asl
            cpx         #12
            bne         @count
            lda         tl_iname                            ; (Its name, if it has one)
            sta         r0
            lda         tl_iname + 1
            sta         r0 + 1
            lda         (r0)
            cmp         #'-'
            beq         :+
            jsr         tl_space
            jsr         tl_puts
:
            jmp         tl_nl

; Count .X (0 lines, 4 words, 8 bytes) one more.  Keeps .A
one:
            inc         counts,X
            bne         @done
            inc         counts + 1,X
            bne         @done
            inc         counts + 2,X
            bne         @done
            inc         counts + 3,X
@done:
            rts

.rodata
s_total:    .byte       "total", 0
tl_name:    .byte       "wc", 0
tl_flagset: .byte       "lwc", 0
tl_usage:   .byte       "wc [-lwc] [file ...]", 0

.include "toollib.s"
