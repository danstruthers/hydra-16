; ****************************************************************************
; tail [-N] [file] - the file's last N lines (10; none: fd 0's).  It's read into memory past the break, 16K of it
; at most (the older half let go when it's full), and its last lines found there.  A file that can't be read is said
; ("tail: name: why"), and tail ends with code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "tail", main

WINDOW          = 16384         ; The bytes kept ...
HALF            = WINDOW / 2    ;   and how many go when it's full

.zeropage
base:       .res        2                                   ; Where the bytes are ...
p:          .res        2                                   ;   where the next goes

.bss
lines:      .res        2                                   ; The lines to show
len:        .res        2                                   ; The bytes kept
found:      .res        2                                   ; (The new lines found, from the end)

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
            bcs         @usage
            lda         tl_num + 2
            ora         tl_num + 3
            bne         @usage
            MOVR        lines, tl_num
            jsr         tl_next
            MOVR        r0, tl_arg
@start:
            jsr         tl_start
            jsr         tl_count                            ; (One file, or none)
            cmp         #2
            bcc         :+
@usage:
            jmp         tl_badusage

:
            LDR         tl_iname, tl_s_stdin                ; The input
            stz         r0
            stz         r0 + 1
            lda         (tl_arg)
            beq         :+
            MOVR        tl_iname, tl_arg
            MOVR        r0, tl_arg
:
            jsr         tl_inopen
            bcs         @failed
            stz         r0                                  ; The memory: WINDOW bytes past the break
            stz         r0 + 1
            jsr         BREAK
            MOVR        base, r0
            clc
            lda         r0 + 1
            adc         #>WINDOW
            sta         r0 + 1
            jsr         BREAK
            bcs         @failed
            MOVR        p, base
            stz         len
            stz         len + 1
@read:
            jsr         tl_getc
            bcs         @end
            sta         (p)
            inc         p
            bne         :+
            inc         p + 1
:
            inc         len
            bne         @read
            inc         len + 1
            lda         len + 1
            cmp         #>WINDOW
            bne         @read
            jsr         halve
            bra         @read

@end:
            jsr         tl_inclose
            jsr         last
            jmp         tl_end

@failed:
            pha
            MOVR        r0, tl_iname
            pla
            jsr         tl_err
            jmp         tl_end

; The older half let go: the newer moved down to base
halve:
            MOVR        r0, base
            clc
            lda         base
            sta         r1
            lda         base + 1
            adc         #>HALF
            sta         r1 + 1
            ldx         #>HALF
            ldy         #0
:
            lda         (r1),Y
            sta         (r0),Y
            iny
            bne         :-
            inc         r0 + 1
            inc         r1 + 1
            dex
            bne         :-
            LDR         len, HALF
            clc
            lda         base
            sta         p
            lda         base + 1
            adc         #>HALF
            sta         p + 1
            rts

; The last lines of what's kept, shown: back from its end (a new line at its very end not counted) to the new
; line before them
last:
            lda         len                                 ; (p: its end)
            ora         len + 1
            beq         @done
            stz         found
            stz         found + 1
            jsr         back                                ; (Its last byte: a new line doesn't count)
            lda         (p)
            cmp         #LF
            beq         @look
            jsr         fwd
@look:
            lda         p                                   ; At its start: all of it
            cmp         base
            bne         :+
            lda         p + 1
            cmp         base + 1
            beq         @show
:
            jsr         back
            lda         (p)
            cmp         #LF
            bne         @look
            inc         found
            bne         :+
            inc         found + 1
:
            lda         found
            cmp         lines
            bne         @look
            lda         found + 1
            cmp         lines + 1
            bne         @look
            jsr         fwd                                 ; (Past that new line)
@show:
            clc                                             ; From p to the end (r0: base + len)
            lda         base
            adc         len
            sta         r0
            lda         base + 1
            adc         len + 1
            sta         r0 + 1
@byte:
            lda         p
            cmp         r0
            bne         :+
            lda         p + 1
            cmp         r0 + 1
            beq         @done
:
            lda         (p)
            jsr         tl_putc
            jsr         fwd
            bra         @byte

@done:
            rts

; p one back, or one on
back:
            lda         p
            bne         :+
            dec         p + 1
:
            dec         p
            rts

fwd:
            inc         p
            bne         :+
            inc         p + 1
:
            rts

.rodata
tl_name:    .byte       "tail", 0
tl_flagset: .byte       0
tl_usage:   .byte       "tail [-N] [file]", 0

.include "toollib.s"
