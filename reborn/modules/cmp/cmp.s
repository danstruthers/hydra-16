; ****************************************************************************
; cmp file1 file2 - the two compared, byte by byte: the first that differs said ("file1 file2 differ: byte 5"), or
; where one ends first ("cmp: end of file1"), and cmp ends with code 1; the same, nothing said, and code 0.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "cmp", main

.zeropage
name1:      .res        2
name2:      .res        2
which:      .res        2                                   ; (The one that ended first)

.bss
fd1:        .res        1
fd2:        .res        1
buf1:       .res        256
buf2:       .res        256
len1:       .res        1
len2:       .res        1
at:         .res        4                                   ; The byte (1 on) being compared
i:          .res        1

.code
main:
            jsr         tl_start
            jsr         tl_count
            cmp         #2
            beq         :+
            jmp         tl_badusage

:
            MOVR        name1, tl_arg
            jsr         tl_next
            MOVR        name2, tl_arg
            MOVR        r0, name1
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         @fail1

:
            sta         fd1
            MOVR        r0, name2
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         @fail2

:
            sta         fd2
            lda         #1
            sta         at
            stz         at + 1
            stz         at + 2
            stz         at + 3
@block:
            LDR         r0, buf1                            ; The next 255 of each
            LDR         r1, 255
            lda         fd1
            jsr         READ
            bcc         :+
            jmp         @fail1

:
            sta         len1
            LDR         r0, buf2
            LDR         r1, 255
            lda         fd2
            jsr         READ
            bcc         :+
            jmp         @fail2

:
            sta         len2
            ldx         #0
@byte:
            cpx         len1
            beq         @end1
            cpx         len2
            beq         @end2
            lda         buf1,X
            cmp         buf2,X
            bne         @differ
            inx
            inc         at
            bne         @byte
            inc         at + 1
            bne         @byte
            inc         at + 2
            bne         @byte
            inc         at + 3
            bra         @byte

@end1:
            cpx         len2                                ; Both at once: the same so far
            bne         :+
            lda         len1
            bne         @block
            jmp         tl_end                              ; (Both ended: the same)

:
            MOVR        r0, name1                           ; The first ended first
            bra         @ended

@end2:
            MOVR        r0, name2
@ended:
            MOVR        which, r0                           ; "cmp: end of NAME"
            LDR         r0, s_eof
            jsr         tl_prefix
            MOVR        r0, which
            jsr         tl_puts2
            LDR         r0, tl_s_nl
            jsr         tl_puts2
            bra         @one

@differ:
            MOVR        r0, name1                           ; "file1 file2 differ: byte N"
            jsr         tl_puts
            jsr         tl_space
            MOVR        r0, name2
            jsr         tl_puts
            LDR         r0, s_differ
            jsr         tl_puts
            ldx         #3
:
            lda         at,X
            sta         tl_num,X
            dex
            bpl         :-
            lda         #1
            jsr         tl_dec
            jsr         tl_nl
@one:
            lda         #1
            sta         tl_code
            jmp         tl_end

@fail1:
            pha
            MOVR        r0, name1
            pla
            bra         @fail

@fail2:
            pha
            MOVR        r0, name2
            pla
@fail:
            jsr         tl_err
            jmp         tl_end

.rodata
s_eof:      .byte       "end of ", 0
s_differ:   .byte       " differ: byte ", 0
tl_name:    .byte       "cmp", 0
tl_flagset: .byte       0
tl_usage:   .byte       "cmp file1 file2", 0

.include "toollib.s"
