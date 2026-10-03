; ****************************************************************************
; df - each disk started (#d's directories, the storage driver's): its name, its kind, its file system's size and
; what's free on it, and its label, from its ctl ("ram 256 KB 512 blocks", "hydrafs label=RAM", "free 192 KB of
; 252 KB"); a disk without a file system, its own size.  A line each, under a heading.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "df", main

.zeropage
rec:        .res        2                                   ; A disk's record (#d's)
to:         .res        2                                   ; Where a word goes

.bss
left:       .res        1                                   ; Disks still to show
path:       .res        16                                  ; "#d/N/ctl"
buf:        .res        257                                 ; Its ctl's text
kind:       .res        12                                  ; The parts of it shown
size:       .res        16
free:       .res        16
label:      .res        32
junk:       .res        16                                  ; (A word not shown)
start:      .res        1                                   ; (match's: where it started)
fd:         .res        1

.code
main:
            jsr         tl_start
            LDR         r0, s_head
            jsr         tl_puts
            LDR         r0, s_disks
            jsr         tl_readdir
            bcs         @failed
            sta         left
            MOVR        rec, r0
@disk:
            lda         left
            beq         @end
            jsr         one
            clc
            lda         rec
            adc         #SR_SIZE
            sta         rec
            bcc         :+
            inc         rec + 1
:
            dec         left
            bra         @disk

@failed:
            pha
            LDR         r0, s_disks
            pla
            jsr         tl_err
@end:
            jmp         tl_end

; The disk whose record is at rec: its line
one:
            ldx         #0                                  ; Its ctl: "#d/N/ctl"
:
            lda         s_disks,X
            sta         path,X
            inx
            cpx         #2
            bne         :-
            lda         #'/'
            sta         path,X
            inx
            ldy         #0
:
            lda         (rec),Y
            beq         :+
            sta         path,X
            inx
            iny
            cpy         #8
            bne         :-
:
            ldy         #0
:
            lda         s_ctl,Y
            sta         path,X
            inx
            iny
            cmp         #0
            bne         :-
            stz         kind                                ; (Nothing known yet)
            stz         size
            stz         free
            stz         label
            LDR         r0, path                            ; Its text
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            rts                                             ; (Gone meanwhile: no line)

:
            sta         fd
            LDR         r0, buf
            LDR         r1, 256
            lda         fd
            jsr         READ
            bcc         :+
            lda         #0
:
            tax
            stz         buf,X
            lda         fd
            jsr         CLOSE
            jsr         parse
            MOVR        r0, rec                             ; Its line: its name ...
            lda         #6
            jsr         tl_field
            LDR         r0, kind                            ;   its kind ...
            lda         #7
            jsr         tl_field
            LDR         r0, size                            ;   its size ...
            lda         #12
            jsr         tl_field
            LDR         r0, free                            ;   what's free ...
            lda         free
            bne         :+
            LDR         r0, s_none
:
            lda         #12
            jsr         tl_field
            LDR         r0, label                           ;   and its label
            jsr         tl_puts
            jmp         tl_nl

; buf's lines into kind, size, free and label
parse:
            ldy         #0
            LDR         to, kind                            ; The first: its kind and size ("ram 256 KB ...")
            jsr         word
            LDR         to, size
            jsr         word
            jsr         also
@line:
            jsr         eol                                 ; The next line
            lda         buf,Y
            beq         @done
            LDR         r0, s_label                         ; "hydrafs label=": its label, the rest of the line
            jsr         match
            bcs         @free
            ldx         #0
:
            lda         buf,Y
            beq         :+
            cmp         #LF
            beq         :+
            cpx         #31
            bcs         :+
            sta         label,X
            iny
            inx
            bra         :-
:
            stz         label,X
            bra         @line

@free:
            LDR         r0, s_free                          ; "free N U of M V": what's free, and the size
            jsr         match
            bcs         @line
            LDR         to, free
            jsr         word
            jsr         also
            LDR         to, junk                            ; ("of")
            jsr         word
            LDR         to, size
            jsr         word
            jsr         also
            bra         @line

@done:
            rts

; Does buf at .Y start with the string at r0?  OUT: C = 0, .Y past it; or C = 1, .Y as it was
match:
            sty         start
            tya
            tax                                             ; (.X: in buf; .Y: in the string)
            ldy         #0
:
            lda         (r0),Y
            beq         @yes
            cmp         buf,X
            bne         @no
            inx
            iny
            bra         :-

@yes:
            txa
            tay
            clc
            rts

@no:
            ldy         start
            sec
            rts

; .Y past the rest of the line and its end
eol:
            lda         buf,Y
            beq         @done
            iny
            cmp         #LF
            bne         eol
@done:
            rts

; The next word at buf,.Y (.Y past it, and the space after it): the string at to (word), or onto its end, a
; space between (also).  15 bytes at most
also:
            ldx         #$FF
:
            inx
            jsr         wget
            bne         :-
            lda         #' '
            jsr         wput
            bra         wbyte

word:
            ldx         #0
wbyte:
            lda         buf,Y
            beq         @end
            cmp         #LF
            beq         @end
            iny
            cmp         #' '
            beq         @end
            jsr         wput
            bra         wbyte

@end:
            lda         #0
            jsr         wput
            rts

; .A into the string at to, at .X (past 15: dropped, but its 0, at 15).  Keeps .Y
wput:
            cpx         #15
            bcc         :+
            cmp         #0
            bne         @done
            ldx         #15
:
            phy
            pha
            txa
            tay
            pla
            sta         (to),Y
            ply
            inx
@done:
            rts

; .A = byte .X of the string at to (Z = 1: its 0).  Keeps .Y
wget:
            phy
            txa
            tay
            lda         (to),Y
            ply
            cmp         #0
            rts

.rodata
s_head:     .byte       "disk  kind   size        free        label", LF, 0
s_disks:    .byte       "#d", 0
s_ctl:      .byte       "/ctl", 0
s_label:    .byte       "hydrafs label=", 0
s_free:     .byte       "free ", 0
s_none:     .byte       "-", 0
tl_name:    .byte       "df", 0
tl_flagset: .byte       0
tl_usage:   .byte       "df", 0

.include "toollib.s"
