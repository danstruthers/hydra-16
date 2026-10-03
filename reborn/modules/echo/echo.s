; ****************************************************************************
; echo [-n] [words] - the words on fd 1, a space between each, and a new line (-n: none).  A line is written
; whole (a buffer of them: 256 bytes at a time).  A write that fails is said on fd 2 ("echo: write error: why"),
; and echo ends with "write error" (code 1).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "echo", main

.zeropage
arg:        .res        2                                   ; An argument

.bss
buf:        .res        256
nl:         .res        1                                   ; <> 0: a new line at the end
first:      .res        1
msg:        .res        32

.code
main:
            MOVR        arg, r0
            lda         #1
            sta         nl
            sta         first
            ldx         #0
            lda         (arg)                               ; -n?
            cmp         #'-'
            bne         @word
            ldy         #1
            lda         (arg),Y
            cmp         #'n'
            bne         @word
            iny
            lda         (arg),Y
            bne         @word
            stz         nl
            jsr         next
@word:
            lda         (arg)                               ; (An empty one: the end)
            beq         @end
            lda         first
            bne         :+
            lda         #' '
            jsr         put
:
            stz         first
            ldy         #0
:
            lda         (arg),Y
            beq         :+
            jsr         put
            iny
            bne         :-
:
            jsr         next
            bra         @word

@end:
            lda         nl
            beq         :+
            lda         #LF
            jsr         put
:
            jsr         flush
            lda         #0
            rts

; arg past its argument
next:
            lda         (arg)
            beq         :+
            inc         arg
            bne         next
            inc         arg + 1
            bra         next
:
            inc         arg
            bne         :+
            inc         arg + 1
:
            rts

; .A into the buffer at .X (full: written first).  Keeps .Y
put:
            sta         buf,X
            inx
            bne         :+
            phy
            ldx         #0                                  ; (256: all of it)
            jsr         write
            ply
            ldx         #0
:
            rts

; The buffer's .X bytes written
flush:
            cpx         #0
            bne         write
            rts

; The same, .X not 0
write:
            LDR         r0, buf
            stx         r1
            stz         r1 + 1
            cpx         #0
            bne         :+
            inc         r1 + 1                              ; (0: 256)
:
            lda         #1
            jsr         WRITE
            bcs         failed
            ldx         #0
            rts

; A failed write (.A, the error): "echo: write error: why" on fd 2, and echo ends, "write error" (code 1)
failed:
            pha
            LDR         r0, s_echo
            jsr         puts2
            LDR         r0, msg
            pla
            jsr         ERRSTR
            LDR         r0, msg
            jsr         puts2
            LDR         r0, s_nl
            jsr         puts2
            LDR         r0, s_werr
            lda         #1
            jmp         EXITS

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
s_echo:     .byte       "echo: write error: ", 0
s_nl:       .byte       LF, 0
s_werr:     .byte       "write error", 0
