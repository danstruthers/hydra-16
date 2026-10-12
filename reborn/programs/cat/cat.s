; ****************************************************************************
; cat [files] - each file to fd 1 (none: fd 0's bytes), 512 bytes at a time.  A file that can't be read is said on
; fd 2 ("cat: name: why"), and cat ends with code 1.  A write that fails is said ("cat: write error: why"), and
; cat ends with "write error" (code 1).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "cat", main

.zeropage
arg:        .res        2                                   ; An argument

.bss
buf:        .res        512
fd:         .res        1
code:       .res        1                                   ; Its exit code
msg:        .res        32

.code
main:
            MOVR        arg, r0
            stz         code
            lda         (arg)
            bne         @file
            lda         #0                                  ; None: fd 0
            jsr         copy
            bra         @end

@file:
            lda         (arg)
            beq         @end
            MOVR        r0, arg
            lda         #O_READ
            jsr         OPEN
            bcs         @failed
            sta         fd
            jsr         copy
            lda         fd
            jsr         CLOSE
@next:
            lda         (arg)                               ; The next argument
            beq         :+
            inc         arg
            bne         @next
            inc         arg + 1
            bra         @next
:
            inc         arg
            bne         @file
            inc         arg + 1
            bra         @file

@failed:
            jsr         say
            lda         #1
            sta         code
            bra         @next

@end:
            stz         r0                                  ; (Its code: EXITS, as returning is 0)
            stz         r0 + 1
            lda         code
            jmp         EXITS

; Fd .A to fd 1, to its end
copy:
            sta         fd
@read:
            LDR         r0, buf
            LDR         r1, 512
            lda         fd
            jsr         READ
            bcs         @done
            sta         r1
            stx         r1 + 1
            ora         r1 + 1
            beq         @done
            LDR         r0, buf
            lda         #1
            jsr         WRITE
            bcc         @read
            pha                                             ; A failed write: said, and cat ends
            LDR         arg, s_werr
            pla
            jsr         say
            LDR         r0, s_werr
            lda         #1
            jmp         EXITS

@done:
            rts

; Error .A on fd 2: "cat: name: why"
say:
            pha
            LDR         r0, s_cat
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
s_cat:      .byte       "cat: ", 0
s_colon:    .byte       ": ", 0
s_nl:       .byte       LF, 0
s_werr:     .byte       "write error", 0
