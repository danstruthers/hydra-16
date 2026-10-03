; ****************************************************************************
; ls [names] - each name: a directory's names (its stat records), a / after a directory's, one a line; a file's
; own name (none: the current directory, .).  A name that isn't there is said on fd 2 ("ls: name: why"), and ls
; ends with code 1.  Its lines are written a buffer at a time; a write that fails is said ("ls: write error: why"),
; and ls ends with "write error" (code 1).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "ls", main

.zeropage
arg:        .res        2                                   ; An argument
rec:        .res        2                                   ; A stat record, in buf

.bss
buf:        .res        512
out:        .res        256                                 ; Lines to write
olen:       .res        1
left:       .res        2                                   ; The bytes of records a read gave, still to show
fd:         .res        1
code:       .res        1
msg:        .res        32
st:         .res        SR_SIZE

.code
main:
            MOVR        arg, r0
            stz         code
            stz         olen
            lda         (arg)
            bne         @name
            LDR         arg, s_dot                          ; None: .
@name:
            lda         (arg)
            beq         @end
            jsr         one
@next:
            lda         (arg)
            beq         :+
            inc         arg
            bne         @next
            inc         arg + 1
            bra         @next
:
            inc         arg
            bne         @name
            inc         arg + 1
            bra         @name

@end:
            jsr         flush
            stz         r0                                  ; (Its code: EXITS, as returning is 0)
            stz         r0 + 1
            lda         code
            jmp         EXITS

; The name at arg: a directory's names, or its own
one:
            MOVR        r0, arg
            LDR         r1, st
            jsr         STAT
            bcs         @failed
            lda         st + SR_QTYPE
            and         #QT_DIR
            bne         @dir
            LDR         rec, st                             ; A file: its name, as given
            MOVR        r0, arg
            jmp         line_r0

@failed:
            jsr         say
            lda         #1
            sta         code
            rts

@dir:
            MOVR        r0, arg
            lda         #O_READ
            jsr         OPEN
            bcs         @failed
            sta         fd
@read:
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
            bcs         @close
            sta         left
            stx         left + 1
            ora         left + 1
            beq         @close
            LDR         rec, buf
@record:
            lda         left + 1                            ; A whole record left?
            bne         :+
            lda         left
            cmp         #SR_SIZE
            bcc         @read
:
            MOVR        r0, rec                             ; Its name (SR_NAME: 0)
            jsr         line_r0
            clc
            lda         rec
            adc         #SR_SIZE
            sta         rec
            bcc         :+
            inc         rec + 1
:
            sec
            lda         left
            sbc         #SR_SIZE
            sta         left
            bcs         @record
            dec         left + 1
            bra         @record

@close:
            lda         fd
            jmp         CLOSE

; A line: the string at r0, a / if record rec is a directory's, a new line
line_r0:
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            jsr         put
            iny
            bne         :-
:
            ldy         #SR_QTYPE
            lda         (rec),Y
            and         #QT_DIR
            beq         :+
            lda         #'/'
            jsr         put
:
            lda         #LF
; .A into the lines (full: written first).  Keeps .Y
put:
            ldx         olen
            sta         out,X
            inc         olen
            bne         :+
            phy
            lda         #0                                  ; (256)
            jsr         write
            ply
:
            rts

; The lines written
flush:
            lda         olen
            bne         write
            rts

; The same, .A bytes of them (0: 256)
write:
            pha
            LDR         r0, out
            pla
            sta         r1
            stz         r1 + 1
            bne         :+
            inc         r1 + 1                              ; (0: 256)
:
            lda         #1
            jsr         WRITE
            stz         olen
            bcs         :+
            rts
:
            pha                                             ; A failed write: said, and ls ends
            LDR         arg, s_werr
            pla
            jsr         say
            LDR         r0, s_werr
            lda         #1
            jmp         EXITS

; Error .A on fd 2: "ls: name: why"
say:
            pha
            jsr         flush
            LDR         r0, s_ls
            jsr         puts2
            MOVR        r0, arg
            jsr         puts2
            LDR         r0, s_colon
            jsr         puts2
            LDR         r0, msg
            pla
            jsr         ERRSTR
            LDR         r0, msg
            jsr         puts2
            LDR         r0, s_nl
; The string at r0 on fd 2
puts2:
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            iny
            bne         :-
:
            sty         r1
            stz         r1 + 1
            lda         #2
            jmp         WRITE

.rodata
s_dot:      .byte       ".", 0, 0
s_ls:       .byte       "ls: ", 0
s_colon:    .byte       ": ", 0
s_nl:       .byte       LF, 0
s_werr:     .byte       "write error", 0
